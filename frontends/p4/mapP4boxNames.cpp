
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* MapP4boxNames::postorder(IR::Path* origPath){
    //printf("Path: %s\n", origPath->toString().c_str());

    /* Check if it is a P4box (protected state) name */
    bool isP4boxName = false;
    std::string pathName = origPath->toString().c_str();

    //FOR each monitor
    for (std::map<cstring, std::vector<cstring>>::iterator it=P4boxIR->pStateNames.begin(); it!=P4boxIR->pStateNames.end(); ++it){
        std::vector<cstring> monitorParameters = it->second;

        //For each monitor parameter
        for (auto paramIt = monitorParameters.cbegin(); paramIt != monitorParameters.cend(); ++paramIt){
            if( *paramIt == pathName ){
                isP4boxName = true;
                //TODO: remove debug code
                //printf("P4box name: %s\n", pathName.c_str());
            }    
        }
    }//End for each monitor

   
    /* Map name to its equivalent in the original program */
    if( isP4boxName ){

        //Figure out which control block is this reference from
        auto parentContext = getContext()->parent;
        std::string parentName = getContext()->node->toString().c_str();
        //TODO: remove debug code
        //printf("Parent name: %s\n", parentName.c_str());
    
        bool blockFound = false;
        std::size_t controlFound, parserFound, rootFound;

        while( not blockFound ){
            controlFound = parentName.find("control");
            parserFound = parentName.find("parser");
            rootFound = parentName.find("P4Program");

            if( controlFound != std::string::npos or
                parserFound != std::string::npos or
                rootFound != std::string::npos ){
                blockFound = true;
            }
            else {
                parentName = parentContext->node->toString().c_str();
                parentContext = parentContext->parent;
            }
        }

        //Get mapping name from original program
        cstring mappingName;

        if( controlFound != std::string::npos or
            parserFound != std::string::npos ){

            //Get block name
            cstring blockName;

            if(controlFound != std::string::npos){
                blockName = parentName.replace(0, 8, "");
            }
            else{
                blockName = parentName.replace(0, 7, "");
            }

            //TODO: remove debug code
            //printf("Block name: %s\n", blockName.c_str());
            mappingName = P4boxIR->nameMap[blockName];
            //printf("Mapping name: %s\n", mappingName.c_str());
 
        }
        else {
            printf("P4box error.\n");
        }//End if found block name

        //Map P4box name
        //TODO: remove debug code
        //printf("Original path name: %s\n", origPath->toString().c_str());

        auto newPath = new IR::Path( IR::ID( origPath->name.srcInfo , mappingName ) );
        return newPath;

    }//End if is a P4box name

    return origPath;
}

/*const IR::Node* MapNames::postorder(IR::Type_Name* origTypeName){
    printf("Type name: %s\n", origTypeName->toString().c_str());

    return origTypeName;    
}*/

/*const IR::Node* MapNames::postorder(IR::PathExpression* origPathExpression){
    printf("Path expression: %s\n", origPathExpression->toString().c_str());

    return origPathExpression;
}*/


const IR::Node* MapP4boxNames::postorder(IR::Member* origMember){
    //printf("Member: %s\n", origMember->toString().c_str());

    /* Check if it is a P4box (protected state) name */
    bool isP4boxName = false;
    std::string memberName = origMember->toString().c_str();

    // Get struct from member name
    std::size_t found = memberName.find(".");
    std::string structName;  

    if (found != std::string::npos){
        structName = memberName.substr(0, found);
        //printf("Struct name: %s\n", structName.c_str());
    }
    else{
        structName = memberName;
    }

    //FOR each monitor
    for (std::map<cstring, std::vector<cstring>>::iterator it=P4boxIR->pStateNames.begin(); it!=P4boxIR->pStateNames.end(); ++it){
        std::vector<cstring> monitorParameters = it->second;

        //For each monitor parameter
        for (auto paramIt = monitorParameters.cbegin(); paramIt != monitorParameters.cend(); ++paramIt){
            if( *paramIt == structName ){
                isP4boxName = true;
                //printf("P4box name: %s\n", structName.c_str());
            }    
        }
    }//End for each monitor

   
    /* Map name to its equivalent in the original program */
    if( isP4boxName ){

        //Figure out which control block is this reference from
        auto parentContext = getContext()->parent;
        std::string parentName = getContext()->node->toString().c_str();
        //printf("Parent name: %s\n", parentName.c_str());
    
        bool blockFound = false;
        std::size_t controlFound, parserFound, rootFound;

        while( not blockFound ){
            controlFound = parentName.find("control");
            parserFound = parentName.find("parser");
            rootFound = parentName.find("P4Program");

            if( controlFound != std::string::npos or
                parserFound != std::string::npos or
                rootFound != std::string::npos ){
                blockFound = true;
            }
            else {
                parentName = parentContext->node->toString().c_str();
                parentContext = parentContext->parent;
            }
        }

        //Get mapping name from original program
        if( controlFound != std::string::npos or
            parserFound != std::string::npos ){

            //Get block name
            cstring blockName;

            if(controlFound != std::string::npos){
                blockName = parentName.replace(0, 8, "");
            }
            else{
                blockName = parentName.replace(0, 7, "");
            }

            //printf("Block name: %s\n", blockName.c_str());
            cstring mappingName = P4boxIR->nameMap[blockName];
            //printf("Mapping name: %s\n", mappingName.c_str());
 
        }
        else {
            printf("P4box error.\n");
        }//End if found block name

        //Map P4box name
        //printf("Original member name: %s\n", origMember->member.name.c_str());
        //origMember->expr->path->name.c_str();
        //TODO: print all Paths

    }//End if is a P4box name

    return origMember;    

}

}//End P4 namespace



















