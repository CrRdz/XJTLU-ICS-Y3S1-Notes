from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, CONFIG_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet, ethernet, arp, ipv4, tcp
from ryu.lib.packet import ether_types
import logging

LOG = logging.getLogger("ryu.app.syn_redirector")


class SynRedirector(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    # Hardcoded server details based on topology
    SERVER1_IP = "10.0.1.2"
    SERVER1_MAC = "00:00:00:00:00:01"
    SERVER2_IP = "10.0.1.3"
    SERVER2_MAC = "00:00:00:00:00:02"

    def __init__(self, *args, **kwargs):
        super(SynRedirector, self).__init__(*args, **kwargs)
        self.mac_to_port = {}
        self.ip_to_mac = {}

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # install table-miss flow
        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        mod = parser.OFPFlowMod(datapath=datapath, priority=0,
                                match=match, instructions=inst)
        datapath.send_msg(mod)

        LOG.info("Installed table-miss flow for switch %s", datapath.id)

    def add_flow(self, datapath, priority, match, actions, idle_timeout=5):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                match=match, instructions=inst,
                                idle_timeout=idle_timeout)
        datapath.send_msg(mod)

        LOG.info("Flow added: match=%s actions=%s timeout=%s", match, actions, idle_timeout)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def packet_in_handler(self, ev):
        msg = ev.msg
        datapath = msg.datapath
        dpid = datapath.id
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto
        in_port = msg.match['in_port']

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocols(ethernet.ethernet)[0]

        # Ignore LLDP
        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            return

        self.mac_to_port.setdefault(dpid, {})
        self.mac_to_port[dpid][eth.src] = in_port  # learn MAC

        arp_pkt = pkt.get_protocol(arp.arp)
        if arp_pkt:
            self.ip_to_mac[arp_pkt.src_ip] = arp_pkt.src_mac

            # simple ARP forward
            if arp_pkt.dst_ip in self.ip_to_mac:
                dst_mac = self.ip_to_mac[arp_pkt.dst_ip]
                out_port = self.mac_to_port[dpid].get(dst_mac, ofproto.OFPP_FLOOD)
            else:
                out_port = ofproto.OFPP_FLOOD

            actions = [parser.OFPActionOutput(out_port)]
            out = parser.OFPPacketOut(datapath=datapath,
                                      buffer_id=ofproto.OFP_NO_BUFFER,
                                      in_port=in_port,
                                      actions=actions,
                                      data=msg.data)
            datapath.send_msg(out)
            return

        ip_pkt = pkt.get_protocol(ipv4.ipv4)
        tcp_pkt = pkt.get_protocol(tcp.tcp)

        if ip_pkt:
            self.ip_to_mac[ip_pkt.src] = eth.src  # learn IP to MAC

        # SYN redirection logic
        if tcp_pkt and ip_pkt:
            is_syn = (tcp_pkt.bits & 0x02) != 0

            if is_syn and ip_pkt.dst == self.SERVER1_IP:
                LOG.info("Detected SYN %s -> server1(%s). Redirecting to server2(%s).",
                         ip_pkt.src, self.SERVER1_IP, self.SERVER2_IP)

                # Ensure controller knows server2 MAC
                self.ip_to_mac[self.SERVER2_IP] = self.SERVER2_MAC
                server2_mac = self.SERVER2_MAC

                # Find switch port of server2 (from MAC learning)
                server2_port = self.mac_to_port[dpid].get(server2_mac)
                if server2_port is None:
                    LOG.info("Server2 MAC not learned yet. Cannot redirect.")
                    return

                # Ensure Server 1 MAC address is known
                server1_mac = self.ip_to_mac.get(self.SERVER1_IP)
                if server1_mac is None:
                    # If Server 1 MAC is not learned, use the hardcoded value
                    server1_mac = self.SERVER1_MAC
                    self.ip_to_mac[self.SERVER1_IP] = server1_mac

                LOG.info("Redirecting SYN: new dst_ip=%s dst_mac=%s out_port=%s",
                         self.SERVER2_IP, server2_mac, server2_port)

                # Install bidirectional flow entries

                # 1. Forward flow: Client -> Server 1 redirected to Server 2
                match_forward = parser.OFPMatch(
                    eth_type=0x0800,
                    ip_proto=6,
                    ipv4_src=ip_pkt.src,
                    ipv4_dst=self.SERVER1_IP,
                    tcp_src=tcp_pkt.src_port,
                    tcp_dst=tcp_pkt.dst_port
                )
                actions_forward = [
                    parser.OFPActionSetField(ipv4_dst=self.SERVER2_IP),
                    parser.OFPActionSetField(eth_dst=server2_mac),
                    parser.OFPActionOutput(server2_port)
                ]

                self.add_flow(datapath, priority=100, match=match_forward, actions=actions_forward)

                # Backward flow: Server 2 -> Client simulating as Server 1
                match_backward = parser.OFPMatch(
                    eth_type=0x0800,
                    ip_proto=6,
                    ipv4_src=self.SERVER2_IP,
                    ipv4_dst=ip_pkt.src,
                    tcp_src=tcp_pkt.dst_port,
                    tcp_dst=tcp_pkt.src_port
                )
                actions_backward = [
                    parser.OFPActionSetField(ipv4_src=self.SERVER1_IP),
                    parser.OFPActionSetField(eth_src=server1_mac),
                    parser.OFPActionOutput(in_port)
                ]

                self.add_flow(datapath, priority=100, match=match_backward, actions=actions_backward)

                # 2. Send PacketOut to server2
                out_actions = [
                    parser.OFPActionSetField(ipv4_dst=self.SERVER2_IP),
                    parser.OFPActionSetField(eth_dst=server2_mac),
                    parser.OFPActionOutput(server2_port)
                ]

                out = parser.OFPPacketOut(
                    datapath=datapath,
                    buffer_id=msg.buffer_id,
                    in_port=in_port,
                    actions=out_actions,
                    data=msg.data if msg.buffer_id == ofproto.OFP_NO_BUFFER else None
                )
                datapath.send_msg(out)
                LOG.info("Installed bidirectional flows for TCP connection")
                return

        # Default L2 learning switch with flow entry
        dst = eth.dst
        if dst in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        # Install a flow for this MAC pair (idle timeout = 5s)
        if dst in self.mac_to_port[dpid]:
            match = parser.OFPMatch(eth_dst=dst)
            self.add_flow(datapath, priority=1, match=match, actions=actions, idle_timeout=5)

        # Send PacketOut for the current packet
        out = parser.OFPPacketOut(
            datapath=datapath,
            buffer_id=msg.buffer_id,
            in_port=in_port,
            actions=actions,
            data=msg.data if msg.buffer_id == ofproto.OFP_NO_BUFFER else None
        )
        datapath.send_msg(out)