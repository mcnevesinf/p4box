
/* P4BOX - PATH CONFORMANCE */
monitor pathConformanceMonitor(inout p4boxState pstate) on ingress {
    local {
        action wp_nop() {
        }

        action enforce_waypoint(){
            standard_metadata.egress_spec = 3;
        }

        action wp_drop() {
            mark_to_drop();
        }

        @name(".check_waypoint") table check_waypoint {
        actions = {
            wp_nop;
            wp_drop;
            enforce_waypoint;
        }
        key = {
            hdr.ipv4.srcAddr : exact;
            hdr.ipv4.dstAddr : exact;
            pstate.pc_tag.isValid(): ternary;
            pstate.pc_tag.tag : ternary;
        }
        size = 512;
        }

        action insert_tag(bit<8> tag){
            //push pc tag
            pstate.pc_tag.setValid();
            pstate.pc_tag.preamble = 32w0xFFFFFFFF;
            pstate.pc_tag.tag = tag;
            //pstate.pc_tag.nextProto = hdr.ethernet.etherType;
            //hdr.ethernet.etherType = 16w0x1234;
        }

        @name(".insert_pc_tag") table insert_pc_tag {
        actions = {
            wp_nop;
            insert_tag;
        }
        size = 1;
        }

        action remove_tag(){
            pstate.pc_tag.setInvalid();
            //hdr.ethernet.etherType = 16w0x800;
        }

        @name(".remove_pc_tag") table remove_pc_tag {
        actions = {
            wp_nop;
            remove_tag;
        }
        size = 1;
        }
    }

    after {
       //Insert tag
       insert_pc_tag.apply();

       //check path conformance
       check_waypoint.apply();

       //Remove tag
       remove_pc_tag.apply();
    }
}


monitor pathConformanceMonitorParser(inout p4boxState pstate) on ParserImpl {
    after parse_ethernet {
        /*state start {
            transition select(hdr.ethernet.etherType){
                16w0x1234: parse_pc_header;
                default: parse_aux_state;
            }
        }*/

        /*state parse_aux_state {
            transition accept;
        }*/

        /* TODO: solve BUG 1        
        state parse_pc_header {
            packet.extract(hdr.pc_tag);
            transition select(hdr.pc_tag.nextProto){
                16w0x800: parse_aux_state;
            }
        }*/
        /*state parse_pc_header {
            packet.extract(pstate.pc_tag);
            transition select(pstate.pc_tag.nextProto){
                16w0x800: parse_ipv4;
            }
        }*/

        state start {
            transition select(packet.lookahead<bit<32>>()){
                32w0xFFFFFFFF : parse_wp_header;
                default : end;
            }
        }

        state parse_wp_header {
            packet.extract(pstate.pc_tag);
            transition end;
        }

        state end {
            transition accept;
        }
    }
}


monitor pathConformanceMonitorDeparser(inout p4boxState pstate) on emit<ethernet_t>{
    after {
        packet.emit(hdr.pc_tag);
    }
}





