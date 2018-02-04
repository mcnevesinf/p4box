#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

void GetCandidateStructs::postorder(const IR::P4Control* control){

    //Get control name
    std::string controlName = control->toString().c_str();
    controlName.replace(0, 8, "");

    bool monitoredBlock = false;

    /*FOR each monitor */
    for (std::map<cstring,cstring>::iterator it=P4boxIR->blockMap.begin(); it!=P4boxIR->blockMap.end(); ++it){

        //IF monitors this control block
        if( it->second == controlName ){
            monitoredBlock = true;
        }
    }

    /* Check blocks that are associated with extern monitors */
    for(std::map<cstring,cstring>::iterator it=P4boxIR->associatedBlocks.begin(); it!=P4boxIR->associatedBlocks.end(); it++){
        if( it->second == controlName ){
            monitoredBlock = true;
        }
    }

    /* Update list of structs that can be instrumented with P4box protected state */
    if( monitoredBlock ){
        IR::IndexedVector<IR::Parameter> blockParameters = control->type->applyParams->parameters;
 
        if( P4boxIR->candidateTypes.size() == 0 ){
            for( auto param : blockParameters ){
                P4boxIR->candidateTypes.push_back( param->type->toString() );
            }
        }
        else {
            /* Remove from list of candidate types (which will be used to instrument the program with the protected state) all types that are not input to this block */
            
            std::vector<cstring> temporaryCandidateTypes;

            //FOR each candidate type
            for( std::vector<cstring>::iterator it = P4boxIR->candidateTypes.begin() ; it != P4boxIR->candidateTypes.end(); ++it ){
                for( auto param : blockParameters ){
                    if( *it == param->type->toString() ){
                        temporaryCandidateTypes.push_back( *it );
                    }
                }
            }

            P4boxIR->candidateTypes.clear();
 
            for( std::vector<cstring>::iterator it = temporaryCandidateTypes.begin() ; it != temporaryCandidateTypes.end(); ++it ){
                P4boxIR->candidateTypes.push_back( *it );
            }
        }
 
    }//End if monitored block

}

}//End P4 namespace






