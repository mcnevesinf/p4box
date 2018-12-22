
/* P4BOX - PATH CONFORMANCE */
header pc_tag_t {
    bit<32> preamble;
    bit<8> tag; 
    //bit<16> nextProto;
}

protected struct p4boxState {
    pc_tag_t pc_tag;
}
