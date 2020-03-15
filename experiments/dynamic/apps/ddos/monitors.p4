//

monitor getInitialState(inout p4boxState pstate) on ingress {
	before {
		/* Well-formedness: get header validity */
		pstate.ddosdPresent = false;

		if( hdr.ddosd.isValid() ){
			pstate.ddosdPresent = true;
		}

		/* Header protection, traffic locality: get header value */
		pstate.protected_ipv4 = hdr.ipv4;
	}
}

/* Waypoint */
monitor parseWaypointLabel(inout p4boxState pstate) on ParserImpl{
	after parse_ipv4 {
		state start {
        	    transition select(hdr.ipv4.protocol){
        	        8w0xFF : parse_wp_label;
        	        default : end;
            	    }
        	}

	        state parse_wp_label {
	            pkt.extract(pstate.waypointLabel);
	            transition end;
	        }

	        state end {
	            transition accept;
	        }
	}
}

monitor deparseWaypointLabel(inout p4boxState pstate) on emit<ipv4_t> {
	after {
		pkt.emit(pstate.waypointLabel);
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
            			pstate.protected_ipv4.src_addr : exact;
            			pstate.protected_ipv4.dst_addr : exact;
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
		if( pstate.ddosdPresent && !hdr.ddosd.isValid() ){
			//Process property violation
		}
	}
}

monitor header_protection(inout p4boxState pstate) on egress {
	after{
		if( pstate.protected_ipv4.src_addr != hdr.ipv4.src_addr ){
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
				pstate.protected_ipv4.src_addr : ternary;
				pstate.protected_ipv4.dst_addr : ternary;
				standard_metadata.egress_port : exact;
			}
			size = 512;
		}
	}

	after{
		tlocal.apply();
	}
}



















