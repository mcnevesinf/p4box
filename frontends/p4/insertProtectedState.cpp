
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* InsertProtectedState::postorder(IR::Type_Struct* origStruct){

    //Check if the struct is a candidate type to receive P4box protected state
    std::string structName = origStruct->toString().c_str();
    structName.replace(0, 7, "");
    bool isCandidateType = false;

    for( std::vector<cstring>::iterator it = P4boxIR->candidateTypes.begin() ; it != P4boxIR->candidateTypes.end(); ++it ){
        if ( *it == structName ){
            isCandidateType = true;
        }
    }

    if( isCandidateType ){
        if( not P4boxIR->pStateInserted ){
            printf("Candidate struct: %s\n", structName.c_str());

            IR::IndexedVector<IR::StructField> newFields;

            //Insert original struct fields
            for( auto field : origStruct->fields ){
                newFields.push_back( field );
            }

            /* Insert P4box protected state */

            //FOR each protected struct
            for(std::map<cstring,const IR::Type_ProtectedStruct*>::iterator it=P4boxIR->pStructMap.begin(); it!=P4boxIR->pStructMap.end(); ++it){

                //FOR each field in the protected struct
                for( auto pField : it->second->fields ){
                    printf("Protected field: %s\n", pField->toString().c_str());
                    newFields.push_back( pField );
                }

            }//End for each protected struct

            /* Instrument original program with P4box protected state */
            auto newStruct = new IR::Type_Struct( origStruct->srcInfo, origStruct->name, origStruct->annotations, newFields );

            P4boxIR->pStateInserted = true;
            P4boxIR->hostStructType = structName;
            return newStruct;
        }//End if protected state not inserted yet
    }//End if is candidate type

    return origStruct;
}

}//End P4 namespace














