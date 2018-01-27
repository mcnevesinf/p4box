
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

void InsertDataPlaneMonitors::getParserMonitorData( 
                                     cstring monitorName,
                                     IR::IndexedVector<IR::Declaration>* instrumentLocals,
                                     IR::IndexedVector<IR::ParserState>* instrumentBefore,
                                     IR::IndexedVector<IR::ParserState>* instrumentAfter,
                                     cstring* beforeStateName,
                                     cstring* afterStateName ){

    const IR::IndexedVector<IR::MonitorProperty>* monitorProperties = P4boxIR->monitorMap[monitorName]->properties;

    //FOR each property
    for ( auto property : *monitorProperties ){
        std::string propertyName = property->toString().c_str();
        propertyName.replace(0, 15, "");

        if(propertyName == " local"){
            //printf("Property parser local\n");

            const IR::MonitorLocal* monitorLocal = property->value->to<IR::MonitorLocal>();
            const IR::IndexedVector<IR::Declaration> locals = monitorLocal->monitorLocals;

            for( auto local : locals ){
                instrumentLocals->push_back( local );
            }                   
        } 
        else { 
            if (propertyName == "before"){
                //printf("Property parser before\n");
                
                const IR::MonitorParserBefore* monitorBefore = property->value->to<IR::MonitorParserBefore>();
                *beforeStateName = monitorBefore->stateName;
                const IR::IndexedVector<IR::ParserState> beforeComponents = monitorBefore->parserStates;

                for( auto component : beforeComponents ){
                    instrumentBefore->push_back( component );
                }
                //TODO: check if we need to deal with annotations
            } 
            else { 
                if (propertyName == "after"){
                    //printf("Property parser after\n");
                 
                    const IR::MonitorParserAfter* monitorAfter = property->value->to<IR::MonitorParserAfter>();
                    *afterStateName = monitorAfter->stateName;
                    const IR::IndexedVector<IR::ParserState> afterComponents = monitorAfter->parserStates;

                    for( auto component : afterComponents ){
                        instrumentAfter->push_back( component );
                    }                              
                }
            }
        }   
                                    
    }//End for each property

}



IR::ParserState* InsertDataPlaneMonitors::changeTransition( 
                                   const IR::ParserState* parserState,
                                   cstring originalStateName,
                                   cstring newStateName ){

    IR::ParserState* newState;

    //IF state contains direct transition
    if( parserState->selectExpression->is<IR::PathExpression>() ){

        const IR::PathExpression* transitionNameNode = parserState->selectExpression->to<IR::PathExpression>();

        //IF state transitions to original state name
        if( transitionNameNode->path->name.name == originalStateName ){

            //Create new transition node
            IR::ID newTransitionName = transitionNameNode->path->name;
            newTransitionName.name = newStateName;
            IR::Path* newPath = new IR::Path( newTransitionName );
            IR::PathExpression* newTransitionExpr = new IR::PathExpression( newPath );

            //Create new parser state linking to monitor start state 
            newState = new IR::ParserState( parserState->srcInfo, parserState->name, parserState->annotations, parserState->components, newTransitionExpr );

        }
        else{
            newState = new IR::ParserState( parserState->srcInfo, parserState->name, parserState->annotations, parserState->components, parserState->selectExpression );
        }
    }
    else{
        //State contains select statement
        const IR::SelectExpression* selectList = parserState->selectExpression->to<IR::SelectExpression>();
        const IR::Vector<IR::SelectCase> selectCases = selectList->selectCases;

        IR::Vector<IR::SelectCase> newSelectCases;

        //FOR each case
        for( auto selectCase : selectCases ){
               
            //IF state transitions to original state name
            if( selectCase->state->path->name.name == originalStateName ){
                IR::ID newSelectCaseName = selectCase->state->path->name;
                newSelectCaseName.name = newStateName;
                IR::Path* newPath = new IR::Path( newSelectCaseName );
                IR::PathExpression* newSelectCaseNameNode = new IR::PathExpression( newPath );
                IR::SelectCase* newSelectCase = new IR::SelectCase( selectCase->keyset, newSelectCaseNameNode );
                newSelectCases.push_back( newSelectCase );
            }//End if transitions to monitored state
            else{
                newSelectCases.push_back( selectCase );
            }                    
        }//End for each select case

        //Create new parser state linking to monitor start state
        IR::SelectExpression* newSelectExpr = new IR::SelectExpression( selectList->srcInfo, selectList->select, newSelectCases );
        newState = new IR::ParserState( parserState->srcInfo, parserState->name, parserState->annotations, parserState->components, newSelectExpr );

    }//End if transition is a select or path expression

    return newState;

}



IR::IndexedVector<IR::ParserState>* InsertDataPlaneMonitors::instrumentParserBefore( 
                               cstring monitoredStateName,
                               IR::IndexedVector<IR::ParserState> instrumentBefore,
                               IR::IndexedVector<IR::ParserState>* parserStateList ){

    cstring monitorInitialState = "M_START";
    monitorInitialState += std::to_string( P4boxIR->monitorStartStateCounter );
    P4boxIR->monitorStartStateCounter++;

    IR::IndexedVector<IR::ParserState>* newParserStateList = new IR::IndexedVector<IR::ParserState>();

    /* Link states to monitor start state */

    //FOR each parser state
    for( auto parserState : *parserStateList ){

        IR::ParserState* newState = changeTransition( parserState, monitoredStateName, monitorInitialState );
        newParserStateList->push_back( newState );        

    }//End for each parser state

    /* Instrument code with monitor states */
    for( auto parserState : instrumentBefore ){

        //Link final monitor states back to the monitored program state
        IR::ParserState* newState = changeTransition( parserState, "accept", monitoredStateName );
       
        //IF is start state THEN set monitor start name
        if( newState->name.name == "start" ){
            newState->name.name = monitorInitialState;
        }

        newParserStateList->push_back( newState );
        
    }

    return newParserStateList;
}



IR::ParserState* InsertDataPlaneMonitors::changeAfterTransition( 
                               const IR::ParserState* parserState, 
                               cstring originalStateName, 
                               const IR::Expression* monitoredTransition ){

    IR::ParserState* newState = new IR::ParserState( parserState->srcInfo, parserState->name, parserState->annotations, parserState->components, parserState->selectExpression );

    //IF state contains direct transition
    if( parserState->selectExpression->is<IR::PathExpression>() ){

        const IR::PathExpression* transitionNameNode = parserState->selectExpression->to<IR::PathExpression>();

        //IF state transitions to original state name
        if( transitionNameNode->path->name.name == originalStateName ){
            newState->selectExpression = monitoredTransition;
        }
    }
    else{
        //State contains select statement

        //IF both the monitored and the monitor state transition using select expressions        
        if( monitoredTransition->is<IR::SelectExpression>() ){
        }
        else{

            const IR::SelectExpression* selectList = parserState->selectExpression->to<IR::SelectExpression>();
            const IR::Vector<IR::SelectCase> selectCases = selectList->selectCases;

            IR::Vector<IR::SelectCase> newSelectCases;

            //FOR each case
            for( auto selectCase : selectCases ){
               
                //IF state transitions to original state name
                if( selectCase->state->path->name.name == originalStateName ){                    
                    IR::SelectCase* newSelectCase = new IR::SelectCase( selectCase->keyset, monitoredTransition->to<IR::PathExpression>() );
                    newSelectCases.push_back( newSelectCase );
                }//End if transitions to monitored state
                else{
                    newSelectCases.push_back( selectCase );
                }                    
            }//End for each select case

            //Create new parser state linking to monitor start state
            IR::SelectExpression* newSelectExpr = new IR::SelectExpression( selectList->srcInfo, selectList->select, newSelectCases );
            newState->selectExpression = newSelectExpr;
        }//End if monitored transition is select or path

    }//End if state transition is a select or path expression

    return newState;
}



IR::IndexedVector<IR::ParserState>* InsertDataPlaneMonitors::instrumentParserAfter( 
                               cstring monitoredStateName,
                               IR::IndexedVector<IR::ParserState> instrumentAfter,
                               IR::IndexedVector<IR::ParserState>* parserStateList ){

    cstring monitorInitialState = "M_START";
    monitorInitialState += std::to_string( P4boxIR->monitorStartStateCounter );
    P4boxIR->monitorStartStateCounter++;

    IR::IndexedVector<IR::ParserState>* newParserStateList = new IR::IndexedVector<IR::ParserState>();
    IR::ParserState* newState;

    /* Get transition from monitored state */
    bool selectExpression = false;
    const IR::PathExpression* monitoredPathTransition;
    const IR::SelectExpression* monitoredSelectTransition;

    for( auto parserState : *parserStateList ){
        if( parserState->name.name == monitoredStateName ){
            if( parserState->selectExpression->is<IR::PathExpression>() ){
                monitoredPathTransition = parserState->selectExpression->to<IR::PathExpression>();
            }
            else{
                monitoredSelectTransition = parserState->selectExpression->to<IR::SelectExpression>();
                selectExpression = true;
            }
        }
    }

    /* Instrument code with monitor states */
    for( auto parserState : instrumentAfter ){

        //Link final monitor states back to the program state after the monitored one
        if( not selectExpression ){
            newState = changeAfterTransition( parserState, "accept", monitoredPathTransition );
        }
        else{
            newState = changeAfterTransition( parserState, "accept", monitoredSelectTransition );
        }
       
        //IF is start state THEN set monitor start name
        if( newState->name.name == "start" ){
            newState->name.name = monitorInitialState;
        }

        newParserStateList->push_back( newState );
    }


    /* Link states to monitor start state */
    for( auto parserState : *parserStateList ){
        if( parserState->name.name == monitoredStateName ){
            //Create new transition node
            IR::ID* newTransitionName = new IR::ID( parserState->name.srcInfo, monitorInitialState );
            IR::Path* newPath = new IR::Path( *newTransitionName );
            IR::PathExpression* newTransitionExpr = new IR::PathExpression( newPath );            

            newState = new IR::ParserState( parserState->srcInfo, parserState->name, parserState->annotations, parserState->components, newTransitionExpr );

            newParserStateList->push_back( newState );
        }
        else{
            newParserStateList->push_back( parserState );
        }

    }//End for each parser state


    return newParserStateList;
}



IR::IndexedVector<IR::Declaration>* InsertDataPlaneMonitors::instrumentParserLocals( 
                               IR::IndexedVector<IR::Declaration> instrumentLocals,
                               IR::IndexedVector<IR::Declaration>* parserLocals ){

    IR::IndexedVector<IR::Declaration>* newParserLocals = new IR::IndexedVector<IR::Declaration>();

    //Add current locals
    for( auto local : *parserLocals ){
        newParserLocals->push_back( local );
    }

    //Instrument code with monitor (local) declarations
    for( auto local : instrumentLocals ){
        newParserLocals->push_back( local );
    }

    return newParserLocals;
}



const IR::Node* InsertDataPlaneMonitors::postorder(IR::P4Parser* parser){

    //Get parser name
    std::string parserName = parser->toString().c_str();
    parserName.replace(0, 7, "");
    //printf("Parser: %s\n", parserName.c_str());

    //Get parser states and local elements (will be instrumented)
    IR::IndexedVector<IR::ParserState>* newParserStateList = new IR::IndexedVector<IR::ParserState>();
    IR::IndexedVector<IR::Declaration>* newParserLocals = new IR::IndexedVector<IR::Declaration>();

    const IR::IndexedVector<IR::ParserState> parserStates = parser->states;
    const IR::IndexedVector<IR::Declaration> parserLocals = parser->parserLocals;

    for( auto state : parserStates ){
        newParserStateList->push_back( state );
    }

    for( auto local : parserLocals ){
        newParserLocals->push_back( local );
    }

    //Instruments that will be added to the program block
    IR::IndexedVector<IR::Declaration> instrumentLocals;
    IR::IndexedVector<IR::ParserState> instrumentBefore;
    IR::IndexedVector<IR::ParserState> instrumentAfter;

    //Parser states that will be linked to monitors
    cstring beforeStateName = "";
    cstring afterStateName = "";

    bool monitoredBlock = false;

    /*FOR each monitor */
    for (std::map<cstring,cstring>::iterator it=P4boxIR->blockMap.begin(); it!=P4boxIR->blockMap.end(); ++it){

        //IF monitor this parser
        if( it->second == parserName ){
            monitoredBlock = true;
            P4boxIR->monitorTypeMap[it->first] = PARSER_MONITOR;

            /* Get monitor properties (local, before and after) */
            getParserMonitorData( it->first,
                                  &instrumentLocals,
                                  &instrumentBefore,
                                  &instrumentAfter,
                                  &beforeStateName,
                                  &afterStateName );        

            /* Instrument parser with monitor data */

            /* Instrument code with monitor (local) declarations */
            if( instrumentLocals.size() > 0 ){
                newParserLocals = instrumentParserLocals( instrumentLocals,
                                                          newParserLocals );
            }
            

            /* Instrument code before the designated state */
            if( beforeStateName != ""){
                newParserStateList = instrumentParserBefore( beforeStateName,
                                                         instrumentBefore,
                                                         newParserStateList );
            }    
                

            /* Instrument code after the designated state */
            if( afterStateName != ""){
                newParserStateList = instrumentParserAfter( afterStateName,
                                                        instrumentAfter,
                                                        newParserStateList );
            }

            instrumentLocals.clear();
            instrumentBefore.clear();
            instrumentAfter.clear();

            beforeStateName = "";
            afterStateName = "";

        }//End if monitor this parser

    }//End for each monitor


    /* Get parameter containing protected state */
    if( monitoredBlock ){
        IR::IndexedVector<IR::Parameter> parameters = parser->type->applyParams->parameters;

        for( auto parameter : parameters ){
            //TODO: remove debug code
            //printf("Parameter type: %s\n", parameter->type->toString().c_str());
            //printf("Host struct type: %s\n", P4boxIR->hostStructType.c_str());
            if( parameter->type->toString() == P4boxIR->hostStructType ){
                //printf("Type hosting protected state: %s\n", parameter->type->toString().c_str());
                //printf("Parameter name: %s\n", parameter->name.name.c_str());
                P4boxIR->nameMap[parserName] = parameter->name.name.c_str();
            }
        }
    }//End if monitored block

    //Update the parser node
    IR::P4Parser* newParser = new IR::P4Parser( parser->srcInfo, parser->name, parser->type, parser->constructorParams, *newParserLocals, *newParserStateList );

    return newParser;
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
            P4boxIR->monitorTypeMap[it->first] = CONTROL_MONITOR;

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
                        if (propertyName == "after"){
                            //printf("Property after\n");
                            const IR::MonitorControlAfter* monitorAfter = property->value->to<IR::MonitorControlAfter>();
                            const IR::IndexedVector<IR::StatOrDecl> afterComponents = monitorAfter->body.components;
                            for( auto component : afterComponents ){
                                instrumentAfter.push_back( component );
                            }                           
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

        //Insert monitoring code after the block
        for( auto component : instrumentAfter ){
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

















