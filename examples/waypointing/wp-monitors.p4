
/* P4BOX - WAYPOINTING */
monitor waypointMonitor(inout p4boxState pstate) on ingress {
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
            pstate.wp_tag.isValid(): ternary;
            pstate.wp_tag.tag : ternary;
        }
        size = 512;
        }

        action insert_tag(bit<8> tag){
            //push wp tag
            pstate.wp_tag.setValid();
            pstate.wp_tag.preamble = 32w0xFFFFFFFF;
            pstate.wp_tag.tag = tag;
        }

        @name(".insert_wp_tag") table insert_wp_tag {
        actions = {
            wp_nop;
            insert_tag;
        }
        size = 1;
        }

        action remove_tag(){
            pstate.wp_tag.setInvalid();
        }

        @name(".remove_wp_tag") table remove_wp_tag {
        actions = {
            wp_nop;
            remove_tag;
        }
        size = 1;
        }
    }

    after {
       //Insert tag
       insert_wp_tag.apply();

       //check path conformance
       check_waypoint.apply();

       //Remove tag
       remove_wp_tag.apply();
    }
}


monitor waypointMonitorParser(inout p4boxState pstate) on ParserImpl {
    after parse_ethernet {
        state start {
            transition select(packet.lookahead<bit<32>>()){
                32w0xFFFFFFFF : parse_wp_header;
                default : end;
            }
        }

        state parse_wp_header {
            packet.extract(pstate.wp_tag);
            transition end;
        }

        state end {
            transition accept;
        }
    }
}


monitor waypointMonitorDeparser(inout p4boxState pstate) on emit<ethernet_t>{
    after {
        packet.emit(hdr.wp_tag);
    }
}





