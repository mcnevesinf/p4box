
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

Visitor::profile_t ResolveMonitorReferences::init_apply(const IR::Node *root){

    //std::map<cstring, Util::SourceInfo> monitorDeclarations; //<monitor-name, line/column-number>
    //std::map<cstring, cstring> blockMap; //<monitor-name, block-name>
    //std::map<cstring, Util::SourceInfo> programDeclarations; //<block-name, line/column-number>

    //FOR each monitor
    for (std::map<cstring,cstring>::iterator monitorIt=P4boxIR->blockMap.begin(); monitorIt!=P4boxIR->blockMap.end(); ++monitorIt){

        /* Check if the monitor is monitoring a declared (and allowed - i.e., top-level) block */
        std::map<cstring,IR::ID>::iterator blockIt;

        //Generate struct containing error information to help debugging
        IR::ID monitorDecl = P4boxIR->monitorDeclarations[monitorIt->first];
        IR::ID blockDecl = P4boxIR->programDeclarations[monitorIt->second];

        IR::ID* errorDecl = new IR::ID();
        errorDecl->name = blockDecl.name;
        errorDecl->originalName = blockDecl.name;
        errorDecl->srcInfo = monitorDecl.srcInfo;

        blockIt = P4boxIR->programDeclarations.find( monitorIt->second );
        if (blockIt != P4boxIR->programDeclarations.end()){
            /* Check if the monitor is declared after the monitored block */
            bool before = blockDecl.srcInfo <= monitorDecl.srcInfo;

            if( !before ){
                ::error("Could not find declaration for monitored block %1% at monitor %2%", errorDecl, monitorDecl.name);
            }
        }
        else{
            ::error("Could not find declaration for monitored block %1% at monitor %2%", errorDecl, monitorDecl.name);
        }

    }//End for each monitor

    return Inspector::init_apply(root);
}

}//End P4 namespace











