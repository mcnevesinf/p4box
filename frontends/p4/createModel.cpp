
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

int CreateModel::getMonitorType(std::string monitoredBlockName){
    int monitorType = 0;

    if( monitorType == 0 ){
        for( auto controlMember : P4boxIR->controlMembers ){
            //std::cout << "Monitored block: " << monitoredBlock << "\n";

            if( controlMember == monitoredBlockName ){
                monitorType = CONTROL_BLOCK_MONITOR;
            }
        }
    }

    //TODO: Check if parser monitor

    //TODO: Check if extern monitor

    return monitorType;
}


std::string CreateModel::typeNameToC(const IR::Type_Name* tName){
    std::string returnString = "";

    return returnString;
}


std::string CreateModel::memberToC(const IR::Member* member){
    std::string returnString = "";

    //std::cout << member->member.toString() << std::endl;

    if( member->member.toString() != "apply" ){
        //TODO: Model "last"
        //std::cout << "lvalue: " << lvalueToC(member->expr) << std::endl; 
        returnString += lvalueToC(member->expr) + "." + member->member.toString();

    }
    else{
        //Apply method
        std::string nodeName = member->expr->toString().c_str();
        //std::cout << "Node name: " << nodeName << std::endl;
        returnString += nodeName;

        //TODO: Add table ID to function name

        returnString += "();";
    }

    return returnString;
}


std::string CreateModel::methodCallExpressionToC(const IR::MethodCallExpression* methodCall){
    std::string returnString = "";

    if( methodCall->method->is<IR::Member>() ){
        const IR::Member* member = methodCall->method->to<IR::Member>();
        returnString += memberToC(member);
        //std::cout << methodCall->method->toString() << std::endl;
    }

    return returnString;
}


std::string CreateModel::pathToC(const IR::PathExpression* pathExpr){
    std::string returnString = ""; 
    
    returnString += pathExpr->path->toString().c_str();

    return returnString;
}


//TODO: check if external function
bool CreateModel::isExternal(bool placeholder){
    return placeholder;
}


std::string CreateModel::lvalueToC(const IR::Expression* lvalue){

    std::string returnString = "";

    if( lvalue->is<IR::PathExpression>() ){
        const IR::PathExpression* pathExpr = lvalue->to<IR::PathExpression>();
        returnString += pathToC(pathExpr);
    }
    else{
        if( lvalue->is<IR::Member>() ){
            const IR::Member* member = lvalue->to<IR::Member>();
            returnString += memberToC(member);
        }
        //TODO: Model other types of lvalues
    }

    return returnString;
}


std::string CreateModel::assign(const IR::AssignmentStatement* assignStatem){
    std::string returnString = "";

    //Model left hand side
    returnString += lvalueToC( assignStatem->left );

    returnString += " = ";

    //TODO: Continue here (model right hand side)

    return returnString;
}


std::string CreateModel::assignmentStatemToC(const IR::AssignmentStatement* assignStatem){
    std::string returnString = "";

    if( isExternal(false) ){
        //TODO: if external function then make left value symbolic
    }
    else{
        returnString += assign( assignStatem );
    }

    return returnString;
}


//std::string CreateModel::controlBlockMonitorToC(IR::BlockStatement body){
std::string CreateModel::blockStatementToC(IR::BlockStatement body){
    std::string returnString = "";

    const IR::IndexedVector<IR::StatOrDecl> components = body.components;

    for( auto component : components ){

        if( component->is<IR::MethodCallStatement>() ){
            const IR::MethodCallStatement* methodCallStatem = component->to<IR::MethodCallStatement>();
            returnString += methodCallExpressionToC(methodCallStatem->methodCall) + "\n\t";
        }
        else{
            if( component->is<IR::AssignmentStatement>() ){
                const IR::AssignmentStatement* assignStatem = component->to<IR::AssignmentStatement>();
                returnString += assignmentStatemToC(assignStatem);
            }
        }//End IF MethodCallStatement

    }//End for each component

    return returnString; 
}


std::string CreateModel::blockStatementToC(const IR::BlockStatement* body){
    std::string returnString = "";

    const IR::IndexedVector<IR::StatOrDecl> components = body->components;

    for( auto component : components ){

        if( component->is<IR::MethodCallStatement>() ){
            const IR::MethodCallStatement* methodCallStatem = component->to<IR::MethodCallStatement>();
            returnString += methodCallExpressionToC(methodCallStatem->methodCall) + "\n\t";
        }
        else{
            if( component->is<IR::AssignmentStatement>() ){
                const IR::AssignmentStatement* assignStatem = component->to<IR::AssignmentStatement>();
                returnString += assignmentStatemToC(assignStatem);
            }
        }//End IF MethodCallStatement

    }//End for each component

    return returnString; 
}



std::string CreateModel::klee_make_symbolic(std::string var){
    std::string returnString = "";

    //TODO: Process case when "var" contains a "."
    returnString += "\tklee_make_symbolic(&" + var + ", sizeof(" + var + "), \"" + var + "\");\n";

    return returnString;
}


std::string CreateModel::actionListNoRules(const IR::ActionList* actionList){
    std::string returnString = "";

    returnString += "\tint symbol;\n";
    returnString += klee_make_symbolic("symbol");
    returnString += "\tswitch(symbol){\n";

    //Go through the action list vector
    int listSize = actionList->size();
    //std::cout << "Action list size: " << listSize << std::endl;

    int iterator = 0;

    for( auto actionElement : actionList->actionList ){
        //actionElement->expression //Can be a path expression or a method call expression
        if( iterator == listSize-1 ){
            returnString += "\t\tdefault:\n";
        }
        else{
            returnString += "\t\tcase " + std::to_string(iterator) + ":\n";
        }

        if( actionElement->expression->is<IR::PathExpression>() ){
            const IR::PathExpression* pathExpr = actionElement->expression->to<IR::PathExpression>();

            //std::cout << "Action name: " << pathExpr->path->name << std::endl;

            //TODO: Deal with action IDs
            returnString += "\t\t\t" + pathToC( pathExpr ) + "();\n";
        }
        else{
            if( actionElement->expression->is<IR::MethodCallExpression>() ){
                //TODO: Model action calls when using method syntax
            }
            else{
                returnString += "ERROR: UNKNOWN ACTION LIST TYPE\n";
            }
        }

        returnString += "\t\t\tbreak;\n";

        iterator++;
    }

    //Close "switch(symbol)"
    returnString += "\t}";

    return returnString;
}


std::string CreateModel::bitSizeToType( int size ){
    std::string returnString = "";

    if( size <= 8 ){
        returnString += "uint8_t";
    }
    else{
        if( size <= 32 ){
            returnString += "uint32_t";
        }
        else{
            returnString += "uint64_t";
        }
    }

    return returnString;
}


std::string CreateModel::p4ActionToC(const IR::P4Action* action){
    std::string returnString = "";

    std::string actionName = action->name.toString() + "_" + std::to_string(nodeCounter);
    nodeCounter++;

    //Model action parameters
    //TODO: How to deal with directions in action parameters?
    //      From P4SPEC: Directionless parameters are set by the control plane. When an action
    // is explicitly invoked (e.g., from a control block or from another action, all its parameters 
    // (including directionless ones) must be supplied.
    // In this case, directionless parameters behave like "in" parameters
    std::string actionData = "";
    const IR::IndexedVector<IR::Parameter> paramList = action->parameters->parameters;

    for( auto parameter : paramList ){

        //Control plane (directionless) parameter
        if( parameter->direction == IR::Direction::None ){
            std::cout << "Control plane parameter" << std::endl;

            //TODO: If manipulating forwarding rules

            //If NOT manipulating forwarding rules

            //IF Type_bit parameter (e.g., bit<32> x)
            if( parameter->type->is<IR::Type_Bits>() ){
                std::cout << "Type bit parameter" << std::endl;
                const IR::Type_Bits* parameterType = parameter->type->to<IR::Type_Bits>();
                actionData += bitSizeToType( parameterType->size ) + " " + parameter->name + ";\n";
            }
            else{
                //TODO: Model any other type of parameter
            }//End IF Type_Bit parameter

            actionData += klee_make_symbolic( parameter->name.toString().c_str() );
        }
        else{
            //TODO: Data plane (derected) parameter
        }
    }

    returnString += "//Action\n";
    returnString += "void " + actionName + "(" + "){\n\t";
    returnString += actionData;
    returnString += blockStatementToC( action->body );
    returnString += "\n}\n\n";

    return returnString;
}


std::string CreateModel::p4TableToC(const IR::P4Table* table){
    std::string returnString = "";

    returnString += "//Table\n";
    returnString += "void " + table->name.toString() + "(){\n";

    //TODO: If manipulating forwarding rules

    //If NOT manipulating forwarding rules
    returnString += actionListNoRules( table->getActionList() );

    returnString += "\n}\n\n";

    return returnString;
}


Visitor::profile_t CreateModel::init_apply(const IR::Node *root){

    //model += "void " + modelName + "(){\n";
    //std::cout << model;

    std::string monitorName;
    int monitorType = 0;
    
    MonitorModel newModel;

    //For each monitor
    for( auto monitor : P4boxIR->monitorMap ){

        //For each property (local, before, after)
        for( auto property : *monitor.second->properties ){

            //Do not need to create a model for locals
            if( not property->value->is<IR::MonitorLocal>() ){

                //Get monitor name
                newModel.monitorName = monitor.first;
                //std::cout << "Monitor: " << newModel.monitorName << std::endl;

                //Get monitored block
                newModel.monitoredBlock = monitor.second->type->monitoredBlock->blockName->monitoredBlockName.c_str();

                //Get monitor type
                monitorType = getMonitorType( newModel.monitoredBlock );
                //std::cout << "Monitor type: " << monitorType << std::endl;

                //TODO: Create an equivalent model according to the monitor type
                switch (monitorType) {
                    case CONTROL_BLOCK_MONITOR:
                        //std::cout << "Control block monitor" << std::endl;

                        newModel.model = "";

                        //Get property type (before/after)
                        if( property->value->is<IR::MonitorControlBefore>() ){
                            newModel.before = true;
                            const IR::MonitorControlBefore* beforeBlock = property->value->to<IR::MonitorControlBefore>();

                            newModel.model += "void " + newModel.monitorName + "_before(){\n\t";
                            //newModel.model += controlBlockMonitorToC( beforeBlock->body );
                            newModel.model += blockStatementToC( beforeBlock->body );
                        }
                        else{
                            newModel.before = false;
                            const IR::MonitorControlAfter* afterBlock = property->value->to<IR::MonitorControlAfter>();

                            newModel.model += "void " + newModel.monitorName + "_after(){\n\t";
                            //newModel.model += controlBlockMonitorToC( afterBlock->body );
                            newModel.model += blockStatementToC( afterBlock->body );
                            newModel.model += "\n}\n\n";
                        }//End if before|after property

                        std::cout << newModel.model << std::endl;

                        break;
                    case PARSER_MONITOR:
                        break;
                }
            }
            else{
                //Model local declarations (e.g., actions, tables)
                const IR::MonitorLocal* localBlock = property->value->to<IR::MonitorLocal>();

                for( auto localDeclaration : localBlock->monitorLocals ){

                    //Actions
                    if( localDeclaration->is<IR::P4Action>() ){
                        locals.push_back( p4ActionToC( localDeclaration->to<IR::P4Action>() ) );
                        std::cout << locals.back() << std::endl;
                    }
                    
                    //Tables
                    if( localDeclaration->is<IR::P4Table>() ){
                        //std::cout << "Model Match-action table" << std::endl;
                        locals.push_back( p4TableToC( localDeclaration->to<IR::P4Table>() ) );
                        //std::cout << locals.back() << std::endl;
                    }

                }//End for each local declaration

            }//End if checking the type of property

        }//End for each property 
    }//End for each monitor
  
    return Inspector::init_apply(root);
}


void CreateModel::postorder(const IR::Declaration_Instance* instance){
    //std::cout << instance->type->toString() << std::endl;
}


void CreateModel::end_apply(const IR::Node* node){
//    std::cout << model;
}

}//End P4 namespace



















