#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* GetSupervisorNodes::postorder(IR::P4Program* program){

    //printf("Root node\n");
    //printf("Program node: %s\n", program->toString().c_str());

    const IR::IndexedVector<IR::Node> declarations = program->declarations;
    IR::IndexedVector<IR::Node> newDeclList;

    std::string declName;
    std::string pStructName;
    std::string monitorName;

    bool removeNode = false;

    for( auto declaration : declarations ){
        removeNode = false;
        declName = declaration->toString();

        /*IF is a protected struct 
          THEN save it in the supervisor map
               remove it from the original program */ 
        std::size_t pStructFound = declName.find("protected struct");

        if( pStructFound != std::string::npos ){
            pStructName = declName.replace(0, 17, "");
            const IR::Type_ProtectedStruct* pStruct = declaration->to<IR::Type_ProtectedStruct>();
            P4boxIR->pStructMap[pStructName] = pStruct;
            removeNode = true;
        }

        //Repeat the same process for monitors
        std::size_t monitorFound = declName.find("monitor");

        if( monitorFound != std::string::npos ){
            if(declaration->is<IR::P4Monitor>()){
                monitorName = declName.replace(0, 8, "");
                const IR::P4Monitor* monitor = declaration->to<IR::P4Monitor>();
                P4boxIR->monitorMap[monitorName] = monitor;
                removeNode = true;
            }
        }


        if( not removeNode ){
            newDeclList.push_back( declaration );
        }
    }//End for each declaration

    auto newProgram = new IR::P4Program( program->srcInfo, newDeclList );
        
    return newProgram;
}


void GetSupervisorNodes::end_apply(const IR::Node* root){

    /* Get block names, protected state names and source information 
       associated with each monitor */    

    //FOR each monitor
    for (std::map<cstring,const IR::P4Monitor*>::iterator it=P4boxIR->monitorMap.begin(); it!=P4boxIR->monitorMap.end(); ++it){

        //Get block name
        //TODO: remove debug code
        //P4boxIR->blockMap[it->first] = it->second->type->controlBlockName;
        P4boxIR->blockMap[it->first] = it->second->type->monitoredBlock->blockName->monitoredBlockName;

        //IF is an extern monitor THEN get associated types and/or block
        std::vector<cstring>::iterator itExtern;

        itExtern = std::find(P4boxIR->externMembers.begin(), P4boxIR->externMembers.end(), it->first);

        if( P4boxIR->blockMap[it->first] == "emit" or 
            itExtern != P4boxIR->externMembers.end() ){

            //Get associated types
            const IR::Vector<IR::Type>* externMonitorTypes = it->second->type->monitoredBlock->blockName->typeArguments;

            for( auto type : *externMonitorTypes ){
                //printf("Extern monitor type: %s\n", type->toString());
                P4boxIR->associatedTypes[it->first].push_back( type->toString() );
            }

            //Get associated block
            P4boxIR->associatedBlocks[it->first] = it->second->type->monitoredBlock->specialization;

        }//End if is an extern monitor

        //Get protected state names
        IR::IndexedVector<IR::Parameter> monitorParameters = it->second->type->monitorParams->parameters;

        for( auto parameter : monitorParameters ){
            //printf("Monitor parameter: %s\n", parameter->toString().c_str());
            P4boxIR->pStateNames[it->first].push_back( parameter->toString().c_str() );
        }

        //Get source information
        P4boxIR->monitorDeclarations[it->first] = it->second->name;
    }//End for each monitor

}

}//End P4 namespace
















