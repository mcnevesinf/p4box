#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* GetSupervisorNodes::postorder(IR::P4Program* program){
    
    printf("Root node\n");

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
    
    /*FOR each monitor
      DO get the associated block name */
    for (std::map<cstring,const IR::P4Monitor*>::iterator it=P4boxIR->monitorMap.begin(); it!=P4boxIR->monitorMap.end(); ++it){
        P4boxIR->blockMap[it->first] = it->second->type->controlBlockName;
    }//End for each monitor

}

}//End P4 namespace
