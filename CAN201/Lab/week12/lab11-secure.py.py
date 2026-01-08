# Copyright (C) 2011 Nippon Telegraph and Telephone Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet
from ryu.lib.packet import ethernet
from ryu.lib.packet import ether_types

from ryu.lib.packet import in_proto
from ryu.lib.packet import ipv4
from ryu.lib.packet import icmp
from ryu.lib.packet import tcp
from ryu.lib.packet import udp
import time
import collections


class SimpleSwitch13(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(SimpleSwitch13, self).__init__(*args, **kwargs)
        self.mac_to_port = {}
        # DoS protection variables
        self.flow_count = {}
        self.packet_rates = {}
        self.attack_detected = False
        self.suspicious_sources = set()

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # install table-miss flow entry
        #
        # We specify NO BUFFER to max_len of the output action due to
        # OVS bug. At this moment, if we specify a lesser number, e.g.,
        # 128, OVS will send Packet-In with invalid buffer_id and
        # truncated packet data. In that case, we cannot output packets
        # correctly.  The bug has been fixed in OVS v2.1.0.
        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions)

        # Initialize flow count for this switch
        self.flow_count[datapath.id] = 0

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]
        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id, priority=priority, match=match,
                                    instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                    match=match, instructions=inst)
        datapath.send_msg(mod)

    def add_flow1(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]

        # Use shorter timeouts during attack to prevent flow table exhaustion
        if self.attack_detected:
            idle_timeout = 2
            hard_timeout = 10
        else:
            idle_timeout = 5
            hard_timeout = 30

        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id, priority=priority,
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    match=match, instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                    idle_timeout=idle_timeout, hard_timeout=hard_timeout,
                                    match=match, instructions=inst)
        datapath.send_msg(mod)

        # Track flow count
        if datapath.id in self.flow_count:
            self.flow_count[datapath.id] += 1

    def is_under_attack(self, datapath, src_mac):
        """Detect if we're under DoS attack"""
        current_time = time.time()

        # Track packet rate per source
        if src_mac not in self.packet_rates:
            self.packet_rates[src_mac] = collections.deque()

        # Add current timestamp and remove old ones
        self.packet_rates[src_mac].append(current_time)
        # Keep only packets from last 2 seconds
        while self.packet_rates[src_mac] and current_time - self.packet_rates[src_mac][0] > 2:
            self.packet_rates[src_mac].popleft()

        # Check packet rate (more than 50 packets in 2 seconds is suspicious)
        if len(self.packet_rates[src_mac]) > 50:
            self.suspicious_sources.add(src_mac)
            return True

        # Check flow table capacity (more than 500 flows indicates potential attack)
        if datapath.id in self.flow_count and self.flow_count[datapath.id] > 500:
            return True

        return False

    def should_install_flow(self, pkt, src_mac):
        """Determine if we should install a flow based on current conditions"""
        if not self.attack_detected:
            return True

        # During attack, be selective about flow installation

        # Always allow ARP
        eth = pkt.get_protocols(ethernet.ethernet)[0]
        if eth.ethertype == ether_types.ETH_TYPE_ARP:
            return True

        # Always allow ICMP (ping)
        if eth.ethertype == ether_types.ETH_TYPE_IP:
            ip_pkt = pkt.get_protocol(ipv4.ipv4)
            if ip_pkt and ip_pkt.proto == in_proto.IPPROTO_ICMP:
                return True

        # Block suspicious sources
        if src_mac in self.suspicious_sources:
            return False

        # For TCP/UDP during attack, use broader matches
        return True

    def create_flow_match(self, parser, eth, in_port, dst, src, pkt):
        """Create flow match with attack-aware granularity"""
        if eth.ethertype == ether_types.ETH_TYPE_IP:
            ip = pkt.get_protocol(ipv4.ipv4)
            srcip = ip.src
            dstip = ip.dst
            protocol = ip.proto

            # During attack, use broader matches to reduce flow table usage
            if self.attack_detected:
                # Use IP-only matches instead of IP+port during attack
                if protocol in [in_proto.IPPROTO_ICMP, in_proto.IPPROTO_TCP, in_proto.IPPROTO_UDP]:
                    return parser.OFPMatch(eth_type=ether_types.ETH_TYPE_IP,
                                           ipv4_src=srcip, ipv4_dst=dstip,
                                           ip_proto=protocol)
            else:
                # Normal mode: use detailed matches
                if protocol == in_proto.IPPROTO_ICMP:
                    return parser.OFPMatch(eth_type=ether_types.ETH_TYPE_IP,
                                           in_port=in_port, ipv4_src=srcip,
                                           ipv4_dst=dstip, ip_proto=protocol)
                elif protocol == in_proto.IPPROTO_TCP:
                    t = pkt.get_protocol(tcp.tcp)
                    return parser.OFPMatch(eth_type=ether_types.ETH_TYPE_IP,
                                           in_port=in_port, ipv4_src=srcip,
                                           ipv4_dst=dstip, ip_proto=protocol,
                                           tcp_src=t.src_port, tcp_dst=t.dst_port)
                elif protocol == in_proto.IPPROTO_UDP:
                    u = pkt.get_protocol(udp.udp)
                    return parser.OFPMatch(eth_type=ether_types.ETH_TYPE_IP,
                                           in_port=in_port, ipv4_src=srcip,
                                           ipv4_dst=dstip, ip_proto=protocol,
                                           udp_src=u.src_port, udp_dst=u.dst_port)

        elif eth.ethertype == ether_types.ETH_TYPE_ARP:
            return parser.OFPMatch(eth_type=ether_types.ETH_TYPE_ARP,
                                   in_port=in_port, eth_dst=dst, eth_src=src)

        return None

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        # If you hit this you might want to increase
        # the "miss_send_length" of your switch
        if ev.msg.msg_len < ev.msg.total_len:
            self.logger.debug("packet truncated: only %s of %s bytes",
                              ev.msg.msg_len, ev.msg.total_len)
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocols(ethernet.ethernet)[0]

        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            # ignore lldp packet
            return
        dst = eth.dst
        src = eth.src

        dpid = format(datapath.id, "d").zfill(16)
        self.mac_to_port.setdefault(dpid, {})

        self.logger.info("packet in %s %s %s %s", dpid, src, dst, in_port)

        # Check for DoS attack
        current_attack_status = self.is_under_attack(datapath, src)
        if current_attack_status and not self.attack_detected:
            self.attack_detected = True
            self.logger.warning("DOS ATTACK DETECTED! Activating mitigation measures.")
        elif not current_attack_status and self.attack_detected:
            self.attack_detected = False
            self.logger.warning("Attack mitigated. Returning to normal operation.")

        # learn a mac address to avoid FLOOD next time.
        self.mac_to_port[dpid][src] = in_port

        if dst in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        # install a flow to avoid packet_in next time
        if out_port != ofproto.OFPP_FLOOD:
            # Check if we should install flow based on current conditions
            if self.should_install_flow(pkt, src):
                match = self.create_flow_match(parser, eth, in_port, dst, src, pkt)

                if match:
                    # flow_mod & packet_out
                    if msg.buffer_id != ofproto.OFP_NO_BUFFER:
                        self.add_flow1(datapath, 1, match, actions, msg.buffer_id)
                        return
                    else:
                        self.add_flow1(datapath, 1, match, actions)
            else:
                self.logger.warning("Flow installation blocked for suspicious source: %s", src)

        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id,
                                  in_port=in_port, actions=actions, data=data)
        datapath.send_msg(out)

    @set_ev_cls(ofp_event.EventOFPFlowRemoved, MAIN_DISPATCHER)
    def flow_removed_handler(self, ev):
        """Track flow removals to maintain accurate count"""
        msg = ev.msg
        datapath = msg.datapath

        if datapath.id in self.flow_count and self.flow_count[datapath.id] > 0:
            self.flow_count[datapath.id] -= 1