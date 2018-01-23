
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* InsertDataPlaneMonitors::postorder(IR::P4Parser* parser){
    return parser;
}


const IR::Node* InsertDataPlaneMonitors::postorder(IR::P4Control* control){

    //Get control name
    std::string controlName = control->toString().c_str();
    controlName.replace(0, 8, "");
    //printf("Control: %s\n", controlName.c_str());

    //Instruments that will be added to the program block
    IR::IndexedVector<IR::Declaration> instrumentLocals;
    IR::IndexedVector<IR::StatOrDecl> instrumentBefore;
    IR::IndexedVector<IR::StatOrDecl> instrumentAfter;

    bool monitoredBlock = false;

    /*FOR each monitor */
    for (std::map<cstring,cstring>::iterator it=P4boxIR->blockMap.begin(); it!=P4boxIR->blockMap.end(); ++it){

        //IF monitors this control block
        if( it->second == controlName ){
            monitoredBlock = true;
            P4boxIR->monitorTypeMap[it->first] = BLOCK_MONITOR;

            /* Get monitor properties (local, before and after) */
            const IR::IndexedVector<IR::MonitorProperty>* monitorProperties = P4boxIR->monitorMap[it->first]->properties;

            //FOR each property
            for ( auto property : *monitorProperties ){
                std::string propertyName = property->toString().c_str();
                propertyName.replace(0, 16, "");

                //printf("Property: %s\n", propertyName.c_str());

                if(propertyName == "local"){
                    //printf("Property local\n");
                    const IR::MonitorLocal* monitorLocal = property->value->to<IR::MonitorLocal>();
                    const IR::IndexedVector<IR::Declaration> locals = monitorLocal->monitorLocals;

                    for( auto local : locals ){
                        instrumentLocals.push_back( local );
                    }
                } 
                else { 
                    if (propertyName == "before"){
                        //printf("Property before\n");
                        const IR::MonitorControlBefore* monitorBefore = property->value->to<IR::MonitorControlBefore>();
                        const IR::IndexedVector<IR::StatOrDecl> beforeComponents = monitorBefore->body.components;

                        for( auto component : beforeComponents ){
                            instrumentBefore.push_back( component );
                        }

                        //TODO: check if we need to deal with annotations
                    } 
                    else { 
                        if (propertyName == "After"){
                            printf("Property after\n");
                            //TODO: get after property
                        }
                    }
                }   
                                            
            }//End for each property

            //const IR::IndexedVector<IR::StatOrDecl> monitorBefore = ->value->body->components;
        }//End if is a monitored block

    }//End for each monitor


    /* Get parameter containing protected state */
    if( monitoredBlock ){
        IR::IndexedVector<IR::Parameter> parameters = control->type->applyParams->parameters;

        for( auto parameter : parameters ){
            //TODO: remove debug code
            //printf("Parameter type: %s\n", parameter->type->toString().c_str());
            //printf("Host struct type: %s\n", P4boxIR->hostStructType.c_str());
            if( parameter->type->toString() == P4boxIR->hostStructType ){
                //printf("Type hosting protected state: %s\n", parameter->type->toString().c_str());
                //printf("Parameter name: %s\n", parameter->name.name.c_str());
                P4boxIR->nameMap[controlName] = parameter->name.name.c_str();
            }
        }
    }//End if monitored block


    /* Instrument control block */
    if( monitoredBlock ){    

        /* Instrument code with monitor declarations (i.e., locals) */
        IR::IndexedVector<IR::Declaration>  newControlLocals;

        //Insert original (local) declarations
        for( auto local : control->controlLocals ){
            newControlLocals.push_back( local );
        }

        //Insert monitor (local) declarations
        for( auto local : instrumentLocals ){
            newControlLocals.push_back( local );
        }

        /* Instrument the control block with monitors before and after it */
        IR::IndexedVector<IR::StatOrDecl> newComponents;

        //Insert monitoring code before the block
        for( auto component : instrumentBefore ){
            newComponents.push_back( component );
        }  

        //Insert original (block) components
        for( auto component : control->body->components ){
            newComponents.push_back( component );
        }

        auto newBody = new IR::BlockStatement( control->body->srcInfo, control->body->annotations, newComponents );

        auto result = new IR::P4Control( control->srcInfo, control->name, control->type, control->constructorParams, newControlLocals, newBody );

        return result;
    }//End if monitored block

    return control;
}





const IR::Node* InsertDataPlaneMonitors::postorder(IR::MethodCallStatement* methodCall){
    return methodCall;
}

}//End P4 namespace

















