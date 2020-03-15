//

header wp_label_t {
    bit<32> label;
}

protected struct p4boxState {
    bool 	ddosdPresent;
    wp_label_t	waypointLabel;
    ipv4_t 	protected_ipv4;
}
