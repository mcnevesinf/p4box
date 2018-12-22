
/* P4BOX - PATH CONFORMANCE */
header wp_tag_t {
    bit<32> preamble;
    bit<8> tag; 
}

protected struct p4boxState {
    wp_tag_t wp_tag;
}
