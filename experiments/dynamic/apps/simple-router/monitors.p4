//

monitor getInitialState(inout p4boxState pstate) on ingress {
	before {
		/* Well-formedness: get header validity */
		pstate.udpPresent = false;

		if( hdr.udp.isValid() ){
			pstate.udpPresent = true;
		}

		/* Header protection, traffic locality: get header value */
		pstate.protected_ipv4 = hdr.ipv4;
	}
}

/* Waypoint */
monitor parseWaypointLabel(inout p4boxState pstate) on ParserImpl{
	after parse_udp {
		state start {
        	    transition select(hdr.udp.dstPort){
        	        16w0xFFFF : parse_wp_label;
        	        default : end;
            	    }
        	}

	        state parse_wp_label {
	            packet.extract(pstate.waypointLabel);
	            transition end;
	        }

	        state end {
	            transition accept;
	        }
	}
}

monitor deparseWaypointLabel(inout p4boxState pstate) on emit<udp_t> {
	after {
		packet.emit(pstate.waypointLabel);
	}
}

monitor checkWaypoint(inout p4boxState pstate) on ingress {
	local{
		action enforce_waypoint(){
            		//Process property violation
        	}

		table check_waypoint {
        		actions = { NoAction; enforce_waypoint; }
        		key = {
            			pstate.protected_ipv4.srcAddr : exact;
            			pstate.protected_ipv4.dstAddr : exact;
            			pstate.waypointLabel.isValid(): ternary;
            			pstate.waypointLabel.label : ternary;
 		        }
        		size = 512;
        	}
	}

	after {
		check_waypoint.apply();
	}
}


monitor well_formedness(inout p4boxState pstate) on egress {
	after{
		if( pstate.udpPresent && !hdr.udp.isValid() ){
			//Process property violation
		}
	}
}

monitor header_protection(inout p4boxState pstate) on egress {
	after{
		if( pstate.protected_ipv4.srcAddr != hdr.ipv4.srcAddr ){
			//Process property violation
		}
	}
}

monitor traffic_locality(inout p4boxState pstate) on egress {
	local{
		action enforce_locality(){ 
			//Process property violation
		}

		table tlocal{
			actions = { NoAction; enforce_locality; }
			key = {
				pstate.protected_ipv4.srcAddr : ternary;
				pstate.protected_ipv4.dstAddr : ternary;
				standard_metadata.egress_port : exact;
			}
			size = 512;
		}
	}

	after{
		tlocal.apply();
	}
}



















