
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* InsertExternMonitors::postorder(IR::MethodCallStatement* origCall){

    cstring callName = origCall->methodCall->toString();
    
    //Prune call name if it is a header emission
    std::string rawCallName = callName.c_str();
    std::size_t found = rawCallName.find(".emit");

    if( found != std::string::npos ){
        rawCallName.replace(0, found+1, "");
        callName = rawCallName;
    }

    bool blockFlag = false;
    bool typeFlag = false;

    //Instruments that will be added to the program block
    IR::IndexedVector<IR::StatOrDecl> instrumentBefore;
    IR::IndexedVector<IR::StatOrDecl> instrumentAfter;

    /*FOR each monitor */
    for (std::map<cstring,cstring>::iterator it=P4boxIR->blockMap.begin(); it!=P4boxIR->blockMap.end(); ++it){

        blockFlag = false;
        typeFlag = false;

        //IF monitor this call
        if( it->second == callName ){

            /* Get program block in which this call appear and
                 figure out which parameter contains the protected state */
            auto controlContext = findContext<IR::P4Control>();

            if( controlContext != nullptr ){
                std::string rawControlBlockName = controlContext->toString().c_str();
                rawControlBlockName.replace(0, 8, "");
                cstring controlName = rawControlBlockName;

                IR::IndexedVector<IR::Parameter> parameters = controlContext->type->applyParams->parameters;

                for( auto parameter : parameters ){
                    if( parameter->type->toString() == P4boxIR->hostStructType ){
                        //printf("Type hosting protected state: %s\n", parameter->type->toString().c_str());
                        //printf("Parameter name: %s\n", parameter->name.name.c_str());
                        P4boxIR->nameMap[controlName] = parameter->name.name.c_str();
                    }
                }
            }

            /* Get program block in which this call appear and
                 figure out which parameter contains the protected state */
            auto parserContext = findContext<IR::P4Parser>();

            if( parserContext != nullptr ){
                std::string rawParserBlockName = parserContext->toString().c_str();
                rawParserBlockName.replace(0, 7, "");
                cstring parserName = rawParserBlockName;

                IR::IndexedVector<IR::Parameter> parameters = parserContext->type->applyParams->parameters;

                for( auto parameter : parameters ){
                    if( parameter->type->toString() == P4boxIR->hostStructType ){
                        //printf("Type hosting protected state: %s\n", parameter->type->toString().c_str());
                        //printf("Parameter name: %s\n", parameter->name.name.c_str());
                        P4boxIR->nameMap[parserName] = parameter->name.name.c_str();
                    }
                }
            }
        
            /* IF the call has an associated block
               THEN check if this call is in that block */
            if( P4boxIR->associatedBlocks[it->first] != "M_NO_SPEC" ){
                auto controlContext = findContext<IR::P4Control>();

                if( controlContext != nullptr ){
                    std::string rawControlBlockName = controlContext->toString().c_str();
                    rawControlBlockName.replace(0, 8, "");
                    cstring controlBlockName = rawControlBlockName;

                    if( P4boxIR->associatedBlocks[it->first] == controlBlockName ){
                        blockFlag = true;
                    }
                }

                auto parserContext = findContext<IR::P4Parser>();             
 
                if( parserContext != nullptr ){
                    std::string rawParserName = parserContext->toString().c_str();
                    rawParserName.replace(0, 7, "");
                    cstring parserName = rawParserName;

                    if( P4boxIR->associatedBlocks[it->first] == parserName ){
                        blockFlag = true;
                    }
                }
            }
            else{
                blockFlag = true;
            }//End if the call has an associated block

            /* IF the call has an associated type signature
               THEN check if this call matches it */
            if( P4boxIR->associatedTypes[it->first].size() > 0 ){
                
                //Get types for the parameters in this call
                const IR::MethodCallExpression* callExpr = origCall->methodCall;
                const IR::Vector<IR::Expression>* callArguments = callExpr->arguments;

                std::string rawType;
                std::size_t spaceFound;
                std::vector<cstring>* typeSignature = new std::vector<cstring>();

                for( auto callArgument : *callArguments ){
                    //Format types from parameters in this call
                    auto type = auxTypeMap->getType(callArgument);

                    if(type != nullptr){
                        rawType = type->toString().c_str();
                        spaceFound = rawType.find_last_of(" ");

                        if( spaceFound != std::string::npos ){
                            rawType.replace(0, spaceFound+1, "");
                        }
 
                        typeSignature->push_back( rawType );
                        //printf("Type: %s\n", rawType.c_str());
                    }
                }//End for each parameter

                //Check if types from this call match monitor types 
                if( P4boxIR->associatedTypes[it->first] == *typeSignature ){
                    typeFlag = true;
                }
                
            }
            else{
                typeFlag = true;
            }//End if the call has an associated type signature

            /* Will instrument extern call. Neet to get monitor 
                properties (i.e., before and after) first. */
            if( blockFlag and typeFlag ){

                const IR::IndexedVector<IR::MonitorProperty>* monitorProperties = P4boxIR->monitorMap[it->first]->properties;

                //FOR each property
                for ( auto property : *monitorProperties ){

                    std::string propertyName = property->toString().c_str();
                    propertyName.replace(0, 16, "");

                    if (propertyName == "before"){

                        const IR::MonitorControlBefore* monitorBefore = property->value->to<IR::MonitorControlBefore>();
                        const IR::IndexedVector<IR::StatOrDecl> beforeComponents = monitorBefore->body.components;

                        for( auto component : beforeComponents ){
                            instrumentBefore.push_back( component );
                        }
                    }
                    else { 
                        if (propertyName == "after"){

                            const IR::MonitorControlAfter* monitorAfter = property->value->to<IR::MonitorControlAfter>();
                            const IR::IndexedVector<IR::StatOrDecl> afterComponents = monitorAfter->body.components;
                            int m=0;
                            for( auto component : afterComponents ){
                                instrumentAfter.push_back( component );
                                m++;
                            }
                            
                        }
                    }

                }//End for each property

            }//End if block and type flags are true

        }//End IF monitor this call

    }//End for each monitor


    /* Instrument extern call */
    IR::IndexedVector<IR::StatOrDecl>* result = new IR::IndexedVector<IR::StatOrDecl>();

    //Insert monitoring code before the extern
    for( auto component : instrumentBefore ){
        result->push_back( component );
    }

    //Insert original extern call
    result->push_back( origCall );

    //Insert monitoring code after the extern
    for( auto component : instrumentAfter ){
        result->push_back( component );
    }
    
    return result;

}


}//End P4 namespace





















