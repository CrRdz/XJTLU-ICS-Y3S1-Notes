from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, CONFIG_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.lib.packet import packet, ethernet, arp, ipv4, tcp, icmp
from ryu.ofproto import ofproto_v1_3
from ryu.lib import mac

# Ryu controller implementing reactive forwarding and TCP SYN traffic handling
class ReactiveController(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # dpid -> {mac: port}
        self.mac_to_port = {}

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures,
                CONFIG_DISPATCHER)  # Install table-miss flow entry when switch connects
    def switch_features_handler(self, ev):
        """Install table-miss flow entry to send unknown traffic to controller"""
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        # Install table-miss flow entry
        match = parser.OFPMatch()

        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        # Create and install table-miss flow entry
        self.add_flow(datapath=datapath,
                      priority=0,
                      match=match,
                      actions=actions)
        self.logger.info("Installed table-miss flow on dpid=%s", datapath.id)

    def add_flow(self, datapath, priority, match, actions, buffer_id=None,
                 idle_timeout=0):  # Function to add flow entry
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        if buffer_id:
            if idle_timeout:
                mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                        priority=priority, match=match, idle_timeout=idle_timeout,
                                        instructions=inst)
            else:
                mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                        priority=priority, match=match,
                                        instructions=inst)
        else:
            if idle_timeout:
                mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                        match=match, idle_timeout=idle_timeout,
                                        instructions=inst)
            else:
                mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                        match=match, instructions=inst)
        datapath.send_msg(mod)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        """Handle Packet-In messages, implement reactive forwarding and TCP SYN traffic handling"""
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        dpid = datapath.id
        self.mac_to_port.setdefault(dpid, {})
        # Parse received packet
        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)
        if eth is None:
            return
        src = eth.src
        dst = eth.dst

        in_port = msg.match.get('in_port')

        self.logger.info(f'Packet in: dpid={dpid} src={src} dst={dst} in_port={in_port}')

        # 1. Learn source MAC address and port mapping
        self.mac_to_port[dpid][src] = in_port
        self.logger.info(f'Learned MAC to port: dpid={dpid} {src} -> {in_port}')

        # 2. Handle ARP packets: flood (convert IP address → MAC address)
        arp_pkt = pkt.get_protocol(arp.arp)
        if arp_pkt:
            # Flood ARP packets
            actions = [parser.OFPActionOutput(ofproto.OFPP_FLOOD)]
            out = parser.OFPPacketOut(datapath=datapath,
                                      buffer_id=ofproto.OFP_NO_BUFFER,
                                      in_port=in_port,
                                      actions=actions,
                                      data=msg.data)
            datapath.send_msg(out)
            return

        ################################## Now the client knows the MAC address and can send the actual TCP SYN handshake packet #######################################
        # 3. Handle IPv4 packets and other logic
        ipv4_pkt = pkt.get_protocol(ipv4.ipv4)

        # --- Step 1: Determine forwarding port (check mac_to_port table) ---
        if dst in self.mac_to_port[dpid]:
            out_port = self.mac_to_port[dpid][dst]
        else:
            out_port = ofproto.OFPP_FLOOD

        actions = [parser.OFPActionOutput(out_port)]

        # --- Step 2: Install flow entry (only when port is known and not flooding, and it's IPv4) ---
        if out_port != ofproto.OFPP_FLOOD and ipv4_pkt:
            ip_proto = ipv4_pkt.proto
            tcp_pkt = pkt.get_protocol(tcp.tcp)

            # === General TCP SYN handling ===
            # Condition: Protocol is TCP + contains SYN flag
            if (ip_proto == 6 and tcp_pkt and (tcp_pkt.bits & 0x02)):

                # Match IP and TCP
                match = parser.OFPMatch(
                    in_port=in_port,
                    eth_dst=dst,
                    eth_type=0x0800,
                    ip_proto=6,
                    ipv4_src=ipv4_pkt.src,
                    ipv4_dst=ipv4_pkt.dst,
                    tcp_src=tcp_pkt.src_port,
                    tcp_dst=tcp_pkt.dst_port
                )
                # Priority 100 (TCP) idle_timeout 5s
                self.add_flow(datapath, 100, match, actions, buffer_id=None, idle_timeout=5)
                self.logger.info("Installed TCP SYN flow: %s -> %s", ipv4_pkt.src, ipv4_pkt.dst)

            # === Normal IPv4 forwarding (ICMP ping or non-SYN TCP) ===
            else:
                match = parser.OFPMatch(
                    in_port=in_port,
                    eth_dst=dst,
                    eth_type=0x0800,
                    ip_proto=ip_proto,
                    ipv4_src=ipv4_pkt.src,
                    ipv4_dst=ipv4_pkt.dst
                )
                # Priority 50, idle_timeout 5s
                self.add_flow(datapath, 50, match, actions, buffer_id=None, idle_timeout=5)

        # --- Step 3: Send Packet-Out ---
        data = None
        if msg.buffer_id == ofproto.OFP_NO_BUFFER:
            data = msg.data

        out = parser.OFPPacketOut(datapath=datapath,
                                  buffer_id=msg.buffer_id,
                                  in_port=in_port,
                                  actions=actions,
                                  data=data)
        datapath.send_msg(out)