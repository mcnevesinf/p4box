
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


int CreateModel::getFieldType(const IR::StructField* field){
    int fieldType = 0;

    if( field->type->is<IR::Type_Name>() ){
	fieldType = TYPE_NAME;
    }

    if( field->type->is<IR::Type_Bits>() ){
	fieldType = TYPE_BITS;
    }

    return fieldType;
}


std::string CreateModel::constantToC(const IR::Constant* intConst){
    std::string returnString = "";

    returnString += intConst->toString().c_str();

    return returnString;
}


std::string CreateModel::typeNameToC(const IR::Type_Name* tName){
    std::string returnString = "";

    returnString += tName->path->toString().c_str();

    return returnString;
}


std::string CreateModel::memberToC(const IR::Member* member){
    std::string returnString = "";

    //TODO: remove debug code
    //std::cout << "Member: " << member->member.toString() << std::endl;
    //std::cout << "Member expr: " << member->expr->toString().c_str() << "\n<<\n";
    std::string memberExpr = member->expr->toString().c_str();

    if( member->member.toString() != "apply" ){
        //TODO: Model "last"

        //Model hit method
        if( member->member.toString() == "hit" and memberExpr.length() > 5 ){
            if( memberExpr.substr( memberExpr.length()-5 ) == "apply" ){
                std::size_t nameDelim = memberExpr.find(".");
                std::string tableName = memberExpr.substr(0, nameDelim);
                //std::cout << "Hit table name: " << tableName << std::endl;
                returnString += tableName + "_" + networkMap->currentNodeName + "_" + std::to_string( tableIDs[tableName] ) + "_hit == 1";
            }
        }
        else{

            //std::cout << "lvalue: " << lvalueToC(member->expr) << std::endl; 
            //std::cout << "Member: " << member->member.toString() << std::endl;
            returnString += lvalueToC(member->expr) + "." + member->member.toString();

        }
    }
    else{
        //Apply method
        std::string nodeName = member->expr->toString().c_str();
        //std::cout << "Node name: " << nodeName << std::endl;
        returnString += nodeName;

        //Add table ID to function name
	returnString += "_" + networkMap->currentNodeName + "_" + std::to_string( tableIDs[nodeName] );
        returnString += "();";
    }

    return returnString;
}


std::string CreateModel::externMethodCall(std::string methodName, 
					  const IR::MethodCallExpression* methodCall,
					  bool* externMethod){
    std::string returnString = "";

    const IR::Member* member = methodCall->method->to<IR::Member>();

    if( methodName == "write" ){
	std::string registerName = member->expr->toString().c_str();

	const IR::Vector<IR::Expression>* args = methodCall->arguments;
	std::vector<std::string> modeledArgs;

	for( auto arg : *args ){
	    modeledArgs.push_back( exprToC(arg) );
	}

	//Add guards to symbolic indexes
	returnString += "klee_assume( " + modeledArgs[0] + " >= 0 );\n\t";
	returnString += "klee_assume( " + modeledArgs[0] + " < " + std::to_string( registerSize[registerName] ) + " );\n\t";

	returnString += registerName + "_" + networkMap->currentNodeName + "_" 
			+ std::to_string( registerIDs[registerName] );

	

	returnString += "[" + modeledArgs[0] + "] = " + modeledArgs[1] + ";";

	*externMethod = true;
    }
    else{
	if( methodName == "read" ){
	    std::string registerName = member->expr->toString().c_str();

	    const IR::Vector<IR::Expression>* args = methodCall->arguments;
	    std::vector<std::string> modeledArgs;

	    for( auto arg : *args ){
	        modeledArgs.push_back( exprToC(arg) );
	    }

	    //Add guards to symbolic indexes
	    returnString += "klee_assume( " + modeledArgs[1] + " >= 0 );\n\t";
	    returnString += "klee_assume( " + modeledArgs[1] + " < " + std::to_string( registerSize[registerName] ) + " );\n\t";

	    returnString += modeledArgs[0] + " = ";
	    returnString += registerName + "_" + networkMap->currentNodeName + "_" + std::to_string( registerIDs[registerName] );
	    returnString += "[" + modeledArgs[1] + "];"; 

	    *externMethod = true;
	}
    }

    return returnString;
}


std::string CreateModel::externFunctionCall( std::string functionName, 
					     const IR::MethodCallExpression* methodCall,
					     bool* externFunction ){
    std::string returnString = "";

    if( functionName == "hash" ){

        const IR::Vector<IR::Expression>* arguments = methodCall->arguments;
        std::string hashOutput = exprToC( arguments->front() );

        returnString += "klee_make_symbolic(&" + hashOutput + ", sizeof(" + hashOutput + "), \"" + hashOutput + "_" + std::to_string(hashCounter) + "\");\n\t";
        hashCounter++;
	*externFunction = true;

    } else
    if( functionName == "mark_to_drop" ){

        returnString += "mark_to_drop();\n";
        *externFunction = true;

    } else
    if( functionName == "NoAction" ){

        returnString += "NoAction();\n";
        *externFunction = true;

    }

    return returnString;
}


std::string CreateModel::methodCallExpressionToC(const IR::MethodCallExpression* methodCall){
    std::string returnString = "";
    bool externMethod = false;
    bool externFunction = false;

    if( methodCall->method->is<IR::Member>() ){
        const IR::Member* member = methodCall->method->to<IR::Member>();
        
        //TODO: Model externs here
        if( member->member.toString() == "setValid" ){
            returnString += member->expr->toString().c_str(); 
            returnString += ".isValid = 1;";
        }
        else{        
            if( member->member.toString() == "setInvalid" ){
                returnString += member->expr->toString().c_str(); 
                returnString += ".isValid = 0;";
            }
            else{
		if( member->member.toString() == "isValid" ){
		    returnString += member->expr->toString().c_str();
		    returnString += ".isValid == 1";
		}
		else{
		        //Check inside externMethodCall function whether modelling an extern call
		        returnString += externMethodCall( member->member.toString().c_str(), methodCall, &externMethod );		    

		        if(!externMethod){
                            returnString += memberToC(member);
                            //std::cout << methodCall->method->toString() << std::endl;
		        }

		}//End if isValid
            }//End if setInvalid
        }//End if setValid
    }
    else{
	if( methodCall->method->is<IR::PathExpression>() ){
	    const IR::PathExpression* pathExpr = methodCall->method->to<IR::PathExpression>();

	    returnString += externFunctionCall( pathExpr->toString().c_str(), methodCall, &externFunction );

            if(!externFunction){
                //TODO: check whether only actions activate this branch
                returnString += pathExpr->toString() + "_" + networkMap->currentNodeName + "_" + 
                                std::to_string( actionIDs[pathExpr->toString().c_str()] ) + "( ";

                const IR::Vector<IR::Expression>* args = methodCall->arguments;
                std::string modeledArgs = "";

	        for( auto arg : *args ){
	            modeledArgs += exprToC(arg) + ", ";
	        }
                //remove last comma
                modeledArgs = modeledArgs.substr(0, modeledArgs.size()-2);
                returnString += modeledArgs;

                returnString += " );\n";
            }	    

	    //std::cout << "Method call: " << pathExpr->toString() << std::endl;
	}
    }

    return returnString;
}


std::string CreateModel::pathToC(const IR::PathExpression* pathExpr){
    std::string returnString = ""; 
    
    returnString += pathExpr->path->toString().c_str();

    return returnString;
}


std::string CreateModel::stringLiteralToC(const IR::StringLiteral* strLit){
    std::string returnString = ""; 
    
    //TODO: Should we add quotation marks?
    returnString += strLit->value.c_str();

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


std::string CreateModel::addToC(const IR::Add* addExpr){
    std::string returnString = "";

    returnString += exprToC( addExpr->left ) + " + " + exprToC( addExpr->right );

    return returnString;
}


std::string CreateModel::subToC(const IR::Sub* subExpr){
    std::string returnString = "";

    returnString += exprToC( subExpr->left ) + " - " + exprToC( subExpr->right );

    return returnString;
}


std::string CreateModel::equToC( const IR::Equ* equExpr ){
    std::string returnString = "";

    returnString += exprToC( equExpr->left ) + " == " + exprToC( equExpr->right );

    return returnString;
}


std::string CreateModel::neqToC( const IR::Neq* neqExpr ){
    std::string returnString = "";

    returnString += exprToC( neqExpr->left ) + " != " + exprToC( neqExpr->right );

    return returnString;
}


std::string CreateModel::lorToC( const IR::LOr* lorExpr ){
    std::string returnString = "";

    returnString += exprToC( lorExpr->left ) + " || " + exprToC( lorExpr->right );

    return returnString;
}


std::string CreateModel::landToC( const IR::LAnd* landExpr ){
    std::string returnString = "";

    returnString += exprToC( landExpr->left ) + " && " + exprToC( landExpr->right );

    return returnString;
}


std::string CreateModel::exprToC(const IR::Expression* expr){
    std::string returnString = "";

    if( expr->is<IR::Constant>() ){
        const IR::Constant* intConstant = expr->to<IR::Constant>();
        returnString += constantToC(intConstant);
    }
    else{
	    if( expr->is<IR::StringLiteral>() ){
		const IR::StringLiteral* stringLiteral = expr->to<IR::StringLiteral>();
		returnString += stringLiteralToC(stringLiteral);
	    }
	    else{
		if( expr->is<IR::PathExpression>() ){
		    const IR::PathExpression* pathExpr = expr->to<IR::PathExpression>();
		    returnString += pathToC(pathExpr);
		}
		else{
		    if( expr->is<IR::Member>() ){
		        const IR::Member* member = expr->to<IR::Member>();
		        returnString += memberToC(member);
		    }
		    else{
			if( expr->is<IR::MethodCallExpression>() ){
			    const IR::MethodCallExpression* methodCall = expr->to<IR::MethodCallExpression>();
			    returnString += methodCallExpressionToC( methodCall );
			}
			else{
			    if( expr->is<IR::Add>() ){
				const IR::Add* addExpr = expr->to<IR::Add>();
				returnString += addToC( addExpr );
			    } else
			    if( expr->is<IR::Sub>() ){
				const IR::Sub* subExpr = expr->to<IR::Sub>();
				returnString += subToC( subExpr );
			    } else
			    if( expr->is<IR::Equ>() ){
				const IR::Equ* equExpr = expr->to<IR::Equ>();
				returnString += equToC( equExpr );
			    } else
			    if( expr->is<IR::Neq>() ){
				const IR::Neq* neqExpr = expr->to<IR::Neq>();
				returnString += neqToC( neqExpr );
			    } else
			    if( expr->is<IR::LOr>() ){
				const IR::LOr* lorExpr = expr->to<IR::LOr>();
				returnString += lorToC( lorExpr );
			    }  else
			    if( expr->is<IR::LAnd>() ){
				const IR::LAnd* landExpr = expr->to<IR::LAnd>();
				returnString += landToC( landExpr );
			    }
			    else{
		                //TODO: Model other types of expressions
				std::cout << "ERROR: Using not modeled expression.\n" << std::endl;
			    }
			}//End if Method call
		    }//End if Member
		}//End if PathExpression
	    }//End if StringLiteral
    }//End if integer constant

    return returnString;
}


std::string CreateModel::assign(const IR::AssignmentStatement* assignStatem){
    std::string returnString = "";
    std::string leftOp = "";
    std::string rightOp = "";
    size_t pos;

    leftOp = lvalueToC( assignStatem->left );

    for( int i=0; i < nodeMetaVars.size(); i++ ){
	pos = leftOp.find( nodeMetaVars[i]+"." );

	if( pos != std::string::npos ){
	    leftOp.replace(pos, nodeMetaVars[i].length(), nodeMetaVars[i] + "_" + networkMap->currentNodeName);
	    break;
	}
    }
    
    returnString += leftOp;
    returnString += " = ";

    rightOp = exprToC( assignStatem->right );

    for( int i=0; i < nodeMetaVars.size(); i++ ){
	pos = rightOp.find( nodeMetaVars[i]+"." );

	if( pos != std::string::npos ){
	    rightOp.replace(pos, nodeMetaVars[i].length(), nodeMetaVars[i] + "_" + networkMap->currentNodeName);
	    break;
	}
    }

    returnString += rightOp;
    returnString += ";";

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


std::string CreateModel::stringReverse( std::string str ){
    std::string revString = "";

    for(int i = str.length()-1; i>=0; i--){
        revString += str[i];
    }

    return revString;
}


std::vector<std::string> CreateModel::splitString( std::string str ){
    std::vector<std::string> splitStr;

    size_t found = 0;
    size_t delim;

    do{
        delim = str.find(" ", found);

        if( delim != std::string::npos ){
            splitStr.push_back( str.substr(found, delim-found) );
            found = delim+1;
        }
        else{
            found = delim;
        }
    }
    while(found != std::string::npos);

    return splitStr;
}



std::string CreateModel::ifStatementToC( const IR::IfStatement* ifStatem ){
    std::string returnString = "";

    const IR::BlockStatement* block;

    //Check whether condition involves applying a match-action table
    std::string condition = exprToC( ifStatem->condition );
    //std::cout << "Condition: " << condition << std::endl;

    std::vector<std::string> splitCond = splitString( condition );
    size_t hitFound = 0;

    for( auto part : splitCond ){
        //std::cout << "Part: " << part << std::endl;
        hitFound = part.find("_hit");
        if( hitFound != std::string::npos and (hitFound + 4 == part.length())){
            //std::cout << "Table call found" << std::endl;
            part.replace(hitFound, 4, "");
            //std::cout << "New part: " << part << std::endl;
            returnString += part + "();\n\t";
        }

        //TODO: model other flags (e.g., miss, action_run, etc)
    }

    returnString += "if( " + condition + " ){\n\t\t";

    if( ifStatem->ifTrue->is<IR::BlockStatement>() ){
	block = ifStatem->ifTrue->to<IR::BlockStatement>();
        returnString += blockStatementToC( block );
    }
    else{
	std::cout << "ERROR: Only block statements can be modeled inside conditional statements. Maybe you want to use brackets." << std::endl;
	exit(1);
    }

    returnString += "}\n\t";

    if( ifStatem->ifFalse != nullptr ){

	returnString += "else{\n\t\t";
	
	if( ifStatem->ifFalse->is<IR::BlockStatement>() ){
	    block = ifStatem->ifFalse->to<IR::BlockStatement>();
            returnString += blockStatementToC( block );
        }
        else{
	    std::cout << "ERROR: Only block statements can be modeled inside conditional statements. Maybe you want to use brackets." << std::endl;
	    exit(1);
        }

	returnString += "}\n\t";

    }

    return returnString;
}


//std::string CreateModel::controlBlockMonitorToC(IR::BlockStatement body){
std::string CreateModel::blockStatementToC(IR::BlockStatement body){

    std::string returnString = "";

    //Check whether there is an assertion associated to the block
    /*std::cout << "Number of annotations: " << body.annotations->size() << std::endl;
    if( body.annotations->size() != 0 ){
	for( auto annotation : body.annotations->annotations){
	    std::cout << "Process each annotation" << std::endl;	
	}
    }*/

    const IR::IndexedVector<IR::StatOrDecl> components = body.components;

    for( auto component : components ){

        if( component->is<IR::MethodCallStatement>() ){
            const IR::MethodCallStatement* methodCallStatem = component->to<IR::MethodCallStatement>();
            returnString += methodCallExpressionToC(methodCallStatem->methodCall) + "\n\t";
        }
        else{
            if( component->is<IR::AssignmentStatement>() ){
                const IR::AssignmentStatement* assignStatem = component->to<IR::AssignmentStatement>();
                returnString += assignmentStatemToC(assignStatem) + "\n\t";
            }
	    else{
		if( component->is<IR::IfStatement>() ){
		    const IR::IfStatement* ifStatem = component->to<IR::IfStatement>();
		    returnString += ifStatementToC( ifStatem ) + "\n\t";
		}
		else{
		    if( component->is<IR::BlockStatement>() ){
		        const IR::BlockStatement* blockStatem = component->to<IR::BlockStatement>();
		        returnString += blockStatementToC( blockStatem ) + "\n\t";
		    }
		}//End IF conditional statement
	    }//End IF assignment statement
        }//End IF MethodCallStatement

    }//End for each component

    return returnString; 
}


std::string CreateModel::replaceAllOccurrences(std::string oldString, char* oldChar, char* newChar){
    std::string newString = "";

    for( auto it=oldString.begin(); it != oldString.end(); ++it ){
	if( *it == *oldChar ){
	    if( newChar != nullptr ){
	        newString += newChar;
	    }
	}
	else{
	    newString += *it;
	}
    }

    return newString;
}


std::pair<std::string, std::string> CreateModel::assertionToC(std::string assertString){

    std::pair <std::string, std::string> assertionModel;
    assertionModel.first = ""; //Return string
    assertionModel.second = ""; //Logical expression

    std::string keywordConstant = "constant";
    std::string keywordIf = "if";
    std::string keywordEqual = "==";

    char dot = '.';
    char underline = '_';

    size_t metaPos;

    //Remove blank spaces and quotation marks from assertion string
    for(int i=0; i<assertString.length(); i++){
	if(assertString[i] == ' ' || assertString[i] == '\"'){
	    assertString.replace(i, 1, "");
	}
    }

    //TODO: model other elements of the assertion language
    if( assertString[0] == '!' ){
	assertString.replace(0, 1, "");

	std::pair <std::string, std::string> assertionNotModel;
	assertionNotModel = assertionToC(assertString);

	assertionModel.first += assertionNotModel.first;
	assertionModel.second += "!" + assertionNotModel.second;
    }
    else{
	    if( assertString.find(keywordIf.c_str(), 0, 2) != std::string::npos ){ //TODO: finish this
		size_t beginExp = assertString.find("(", 0);
		size_t endExp = assertString.find(",", beginExp);

		std::string condExpr = assertString.substr(beginExp+1, endExp-beginExp-1);
		std::pair <std::string, std::string> assertionIfLeftModel = assertionToC(condExpr);

		assertionModel.first += assertionIfLeftModel.first;
	    
		size_t endCondCommand = assertString.find(")", endExp+1);
		std::string condCommand = assertString.substr(endExp+1, endCondCommand-endExp-1);
		std::pair <std::string, std::string> assertionIfRightModel = assertionToC(condCommand);

		assertionModel.first += assertionIfRightModel.first;

		assertionModel.second += "!(" + assertionIfLeftModel.second + ") || (" + assertionIfRightModel.second + ")";
	    }
	    else{
		size_t eqPosition = assertString.find(keywordEqual, 0);

		if( eqPosition != std::string::npos ){

			//Remove any parentheses
			for(int i=0; i<assertString.length(); i++){
				if( (assertString[i] == '(') || (assertString[i] == ')') ){
	    				assertString.replace(i, 1, "");
				}
	    		}

			std::string left = assertString.substr(0, eqPosition-1);

			//Add node name - left
    			for( int i=0; i < nodeMetaVars.size(); i++ ){
			    metaPos = left.find( nodeMetaVars[i]+"." );

			    if( metaPos != std::string::npos ){
	    			left.replace(metaPos, nodeMetaVars[i].length(), nodeMetaVars[i] + "_" + networkMap->currentNodeName);
	    			break;
			    }
    			}

			std::string right = assertString.substr(eqPosition+1, assertString.length());

			//Add node name - right
    			for( int i=0; i < nodeMetaVars.size(); i++ ){
			    metaPos = right.find( nodeMetaVars[i]+"." );

			    if( metaPos != std::string::npos ){
	    			right.replace(metaPos, nodeMetaVars[i].length(), nodeMetaVars[i] + "_" + networkMap->currentNodeName);
	    			break;
			    }
    			}

			//Create global var
			std::string globalVarName = "";
			globalVarName += replaceAllOccurrences(left, &dot, &underline) + "_eq_" + 
					 replaceAllOccurrences(right, &dot, &underline) + "_" + 
					 std::to_string(assertionCounter);			
			networkMap->globalDeclarations += "int " + globalVarName + ";\n";
			assertionModel.second = globalVarName;
			assertionModel.first = globalVarName + " = (" + left + " == " + right + ");\n\t";
		}
		else{
			if( assertString.find(keywordConstant, 0) != std::string::npos ){
			    size_t begin = assertString.find("(", 0);
			    size_t end = assertString.find(")", begin);

	    		    std::string constantVariable = assertString.substr(begin+1, end-begin-1);

			    //Add node name
    			    for( int i=0; i < nodeMetaVars.size(); i++ ){
			        metaPos = constantVariable.find( nodeMetaVars[i]+"." );

			        if( metaPos != std::string::npos ){
	    			    constantVariable.replace(metaPos, nodeMetaVars[i].length(), nodeMetaVars[i] + "_" + networkMap->currentNodeName);
	    			    break;
			        }
    			    }

			    std::string globalVarName = "constant_" + replaceAllOccurrences(constantVariable, &dot, &underline) + "_" + std::to_string(assertionCounter);
			    std::string constantType = "uint64_t"; //TODO: get proper field type
			    networkMap->globalDeclarations += constantType + " " + globalVarName + ";\n";

			    assertionModel.first += globalVarName + " = " + constantVariable + ";";
			    assertionModel.second += globalVarName + " == " + constantVariable;
	    		}//End "constant"
		}//End "=="
	    }//End "if"
    }//End "!"

    return assertionModel;
}


std::string CreateModel::blockStatementToC(const IR::BlockStatement* body){

    std::string returnString = "";
    std::pair <std::string, std::string> assertionModel;

    //Check whether there is an assertion associated to the block
    if( body->annotations->size() != 0 ){
	for( auto annotation : body->annotations->annotations){
	    if( annotation->name == "assert" ){
		IR::Vector<IR::Expression> expr = annotation->expr;

		for( auto assertion : expr ){
		    std::string assertString = assertion->toString().c_str();
		
		    assertionModel = assertionToC(assertString); 
		    assertionCounter++;
		    returnString += assertionModel.first;
		    networkMap->logicalExpressionList.push_back( assertionModel.second );
		}
	    }	
	}
    }

    const IR::IndexedVector<IR::StatOrDecl> components = body->components;

    for( auto component : components ){

        if( component->is<IR::MethodCallStatement>() ){
            const IR::MethodCallStatement* methodCallStatem = component->to<IR::MethodCallStatement>();
            returnString += methodCallExpressionToC(methodCallStatem->methodCall) + "\n\t";
        }
        else{
            if( component->is<IR::AssignmentStatement>() ){
                const IR::AssignmentStatement* assignStatem = component->to<IR::AssignmentStatement>();
                returnString += assignmentStatemToC(assignStatem) + "\n\t";
            }
	    else{
		if( component->is<IR::IfStatement>() ){
		    const IR::IfStatement* ifStatem = component->to<IR::IfStatement>();
		    returnString += ifStatementToC( ifStatem ) + "\n\t";
		}
		else{
		    if( component->is<IR::BlockStatement>() ){
		        const IR::BlockStatement* blockStatem = component->to<IR::BlockStatement>();
		        returnString += blockStatementToC( blockStatem ) + "\n\t";
		    }
		    //TODO: continue here. Process assertion.
		}//End IF conditional statement
	    }//End IF assignment statement
        }//End IF MethodCallStatement

    }//End for each component

    return returnString; 
}



std::string CreateModel::klee_make_symbolic(std::string var){
    std::string returnString = "";

    //TODO: Process case when "var" contains a "."
    returnString += "\tklee_make_symbolic(&" + var + ", sizeof(" + var + "), \"" + var + "\");\n\t";

    return returnString;
}


std::string CreateModel::actionListNoRules(const IR::ActionList* actionList){
    std::string returnString = "";
    std::string actionName = "";

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
	    actionName = pathToC( pathExpr );

            //TODO: Deal with action IDs
            returnString += "\t\t\t" + actionName + "_" + networkMap->currentNodeName +
				"_" + std::to_string( actionIDs[actionName] ) + "();\n";
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

    actionIDs[action->name.toString()] = actionCounter;
    actionCounter++;

    std::string actionName = action->name.toString() + "_" + networkMap->currentNodeName +
				"_" + std::to_string( actionIDs[action->name.toString()] );
    nodeCounter++;

    //Model action parameters
    //TODO: How to deal with directions in action parameters?
    //      From P4SPEC: Directionless parameters are set by the control plane. When an action
    // is explicitly invoked (e.g., from a control block or from another action, all its parameters 
    // (including directionless ones) must be supplied.
    // In this case, directionless parameters behave like "in" parameters
    std::string actionData = "";
    std::string paramModel = "";
    std::string parameters = "";
    const IR::IndexedVector<IR::Parameter> paramList = action->parameters->parameters;

    for( auto parameter : paramList ){

        //Control plane (directionless) parameter
        if( parameter->direction == IR::Direction::None ){

            //TODO: If manipulating forwarding rules
	    if( forwardingRules.size() > 0 ){
		paramModel = "";
		
		if( parameter->type->is<IR::Type_Bits>() ){
                    const IR::Type_Bits* parameterType = parameter->type->to<IR::Type_Bits>();
                    paramModel += bitSizeToType( parameterType->size ) + " " + parameter->name;
                }
                else{
                    //TODO: Model any other type of parameter
                }//End IF Type_Bit parameter

		parameters += paramModel + ", ";
	    }
	    else{

                //IF Type_bit parameter (e.g., bit<32> x)
                if( parameter->type->is<IR::Type_Bits>() ){
                    const IR::Type_Bits* parameterType = parameter->type->to<IR::Type_Bits>();
                    actionData += bitSizeToType( parameterType->size ) + " " + parameter->name + ";\n";
                }
                else{
                    //TODO: Model any other type of parameter
                }//End IF Type_Bit parameter

                actionData += klee_make_symbolic( parameter->name.toString().c_str() );

	    }//End if manipulating forwarding rules
        }
        else{
            //TODO: Data plane (derected) parameter
        }
    }

    //Remove comma after last parameter
    if( parameters != "" ){
	parameters = parameters.substr(0, parameters.length()-2);
    }

    returnString += "//Action\n";
    returnString += "void " + actionName + "( " + parameters + " ){\n\t";
    returnString += actionData;
    returnString += blockStatementToC( action->body );
    returnString += "\n}\n\n";

    return returnString;
}


std::string CreateModel::p4ParserToC(const IR::P4Parser* parser){
    std::string returnString = "";

    return returnString;
}


std::string CreateModel::convertExactMatchValue(std::string value){
    std::string returnString = "";

    size_t matchDelimiter = value.find(':', 0);
    size_t begin, end, substrLen;

    //TODO: convert other match types (e.g., ?)
    if( matchDelimiter != std::string::npos ){
	//MAC value
	char macDelim = ':';
	std::string strMAC = replaceAllOccurrences(value, &macDelim, nullptr);
	uint64_t intMAC = std::stoul( strMAC, nullptr, 16 );
	returnString = std::to_string( intMAC );
    }
    else{
	matchDelimiter = value.find('.', 0);

	if( matchDelimiter != std::string::npos ){
	    //Convert IP value
	    begin = 0;

	    std::string octet;
	    int i_octet;
	    std::string bitIP = "";

	    for(int i=0; i<4; i++){
	        substrLen = matchDelimiter - begin;
	        octet = value.substr(begin, substrLen);

	        i_octet = std::stoi(octet, nullptr, 10);
	        bitIP += std::bitset<8>(i_octet).to_string();

	        begin = matchDelimiter+1;
	        matchDelimiter = value.find('.', begin);
	
	        if( matchDelimiter == std::string::npos ){
		    matchDelimiter = value.length();
	        }
            }

	    unsigned long intIP = std::bitset<32>(bitIP).to_ulong();
	    returnString = std::to_string(intIP);
        }
        else{
	    //Integer values
	    returnString += value;
        }//End if IP value
    }//End if MAC value

    return returnString;
}


std::string CreateModel::actionListWithRulesOptimized(cstring tableName, const IR::Key* keyList){
    std::string returnString = "";

    //Map containing action indexed rules
    //Key : (action name, action parameters) - string format
    //Value : match from rules invoking action key
    std::map< std::pair<std::string, std::string>, std::vector<std::string> > actionIndexedRules;

    std::string match = "";
    std::string keyName = "";
    std::string matchType = "";
    std::string arguments = "";
    std::string fullActionName = "";

    std::vector<std::string> matchValues;
    std::vector<std::string> actionParameters;

    bool tableAdd = false;
    std::string defaultAction;

    //Create action indexed rules
    for(auto rule : forwardingRules[tableName]){
        
        if( rule[0] == "table_add" ){
            tableAdd = true;
	    
	    //Extract matching values from forwarding rule
	    size_t matchBegin = 0;
	    size_t matchEnd = rule[2].find(' ', 0);
	    size_t substrLen;
	    std::string matchValue;

	    while( matchEnd != std::string::npos ){
		substrLen = matchEnd - matchBegin;
		matchValue = rule[2].substr(matchBegin, substrLen);
		matchValues.push_back(matchValue);

		matchBegin = matchEnd+1;
		matchEnd = rule[2].find(' ', matchBegin);
	    }

	    //Model rule match
	    int i=0, j=0;
	    size_t pos;

	    for( auto key : keyList->keyElements ){
		keyName = key->expression->toString();

		//Set match names specific to a device (i.e., metadata)
		for( j=0; j < nodeMetaVars.size(); j++ ){
		    pos = keyName.find( nodeMetaVars[j]+"." );

		    if( pos != std::string::npos ){
		        keyName.replace(pos, nodeMetaVars[j].length(), nodeMetaVars[j] + "_" + networkMap->currentNodeName);
	    		break;
		    }
    		}

		matchType = key->matchType->toString();

		if(matchType == "exact"){
		    match += keyName + " == " + convertExactMatchValue(matchValues[i]) + " && ";
		}
		else{
		    //TODO: Model other match types (ternary, lpm, etc) as well as rule priorities
		}

		i++;

	    }//End for each matching key

	    //Remove last connector
	    match = match.substr(0, match.length()-4);

	    //Model action arguments
	    
	    //Extract arguments from forwarding rule
	    if( rule.size() > 3 ){
		size_t argBegin = 1;
		size_t argEnd = rule[3].find(' ', argBegin);
		size_t argLen;
		std::string argValue;

		while( argEnd != std::string::npos ){
		    argLen = argEnd - argBegin;
		    argValue = rule[3].substr(argBegin, argLen);
		    actionParameters.push_back(argValue);

		    argBegin = argEnd+1;
		    argEnd = rule[3].find(' ', argBegin);
	        }

		argLen = argEnd - argBegin;
		argValue = rule[3].substr(argBegin, argLen);
		actionParameters.push_back(argValue);

	    }
	    
	    for( auto param : actionParameters ){
		arguments += convertExactMatchValue(param) + ", ";
	    }

	    arguments = arguments.substr(0, arguments.length()-2);

	    fullActionName = rule[1] + "_" + networkMap->currentNodeName + "_" + std::to_string(actionIDs[rule[1]]);

            std::pair<std::string, std::string> actionIndexedKey;
            actionIndexedKey.first = fullActionName;
            actionIndexedKey.second = arguments;
	    actionIndexedRules[actionIndexedKey].push_back(match);
	    
	}//End if table_add
        else{
	    if( rule[0] == "table_set_default" ){
		//FIXME: deal with case where default action has parameters
		defaultAction = rule[1] + "_" + networkMap->currentNodeName + "_" + std::to_string(actionIDs[rule[1]]);
	    }

	    //TODO: Model other commands
	}

        //Clear rule components
        match = "";
        arguments = "";
        matchValues.clear();
        actionParameters.clear();

    }//End creation of action indexed rules

    if(tableAdd){
        std::map< std::pair<std::string, std::string>, std::vector<std::string> >::iterator it = actionIndexedRules.begin();
 
        // Iterate over the map using Iterator till end.
        std::string combinedMatch = "";

        while (it != actionIndexedRules.end()){
            fullActionName = it->first.first;
            arguments = it->first.second;
            combinedMatch = "";

            for(auto match : actionIndexedRules[it->first]){
                combinedMatch += match + " ||\n\t";
            }

            //Remove last connector
            combinedMatch = combinedMatch.substr(0, combinedMatch.length()-5);

            //TODO: continue here. organize match
	    returnString += "\tif( " + combinedMatch + " ){\n";
	    returnString += "\t\t" + fullActionName + "( " + arguments + " );\n";
            returnString += "\t\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 1;\n";
	    returnString += "\t} else ";

            it++;
        }

	returnString += "{\n\t\t" + defaultAction + "(); //Default action\n";
        returnString += "\t\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 0;\n\t}";
    }
    else{
        returnString += "\t" + defaultAction + "(); //Default action\n";
        returnString += "\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 0;\n";
    }

    return returnString;
}


std::string CreateModel::actionListWithRules(cstring tableName, const IR::Key* keyList){
    std::string returnString = "";

    std::string match = "";
    std::string keyName = "";
    std::string matchType = "";
    std::string arguments = "";

    std::vector<std::string> matchValues;
    std::vector<std::string> actionParameters;

    bool tableAdd = false;
    std::string defaultAction;

    for(auto rule : forwardingRules[tableName]){

	//Model table entry
	if( rule[0] == "table_add" ){
	    tableAdd = true;

	    //Extract matching values from forwarding rule
	    size_t matchBegin = 0;
	    size_t matchEnd = rule[2].find(' ', 0);
	    size_t substrLen;
	    std::string matchValue;

	    while( matchEnd != std::string::npos ){
		substrLen = matchEnd - matchBegin;
		matchValue = rule[2].substr(matchBegin, substrLen);
		matchValues.push_back(matchValue);

		matchBegin = matchEnd+1;
		matchEnd = rule[2].find(' ', matchBegin);
	    }

	    //Model rule match
	    int i=0, j=0;
	    size_t pos;

	    for( auto key : keyList->keyElements ){
		keyName = key->expression->toString();

		//Set match names specific to a device (i.e., metadata)
		for( j=0; j < nodeMetaVars.size(); j++ ){
		    pos = keyName.find( nodeMetaVars[j]+"." );

		    if( pos != std::string::npos ){
		        keyName.replace(pos, nodeMetaVars[j].length(), nodeMetaVars[j] + "_" + networkMap->currentNodeName);
	    		break;
		    }
    		}

		matchType = key->matchType->toString();

		if(matchType == "exact"){
		    match += keyName + " == " + convertExactMatchValue(matchValues[i]) + " && ";
		}
		else{
		    //TODO: Model other match types (ternary, lpm, etc) as well as rule priorities
		}

		i++;

	    }//End for each matching key

	    //Remove last connector
	    match = match.substr(0, match.length()-4);

	    //Model action arguments
	    
	    //Extract arguments from forwarding rule
	    if( rule.size() > 3 ){
		size_t argBegin = 1;
		size_t argEnd = rule[3].find(' ', argBegin);
		size_t argLen;
		std::string argValue;

		while( argEnd != std::string::npos ){
		    argLen = argEnd - argBegin;
		    argValue = rule[3].substr(argBegin, argLen);
		    actionParameters.push_back(argValue);

		    argBegin = argEnd+1;
		    argEnd = rule[3].find(' ', argBegin);
	        }

		argLen = argEnd - argBegin;
		argValue = rule[3].substr(argBegin, argLen);
		actionParameters.push_back(argValue);

	    }
	    
	    for( auto param : actionParameters ){
		arguments += convertExactMatchValue(param) + ", ";
	    }

	    arguments = arguments.substr(0, arguments.length()-2);

	    std::string fullActionName = rule[1] + "_" + networkMap->currentNodeName + "_" + std::to_string(actionIDs[rule[1]]);

	    returnString += "\tif( " + match + "){\n";
	    returnString += "\t\t" + fullActionName + "( " + arguments + " );\n";
            returnString += "\t\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 1;\n";
	    returnString += "\t} else ";
	    
	}//End if table_add
	else{
	    if( rule[0] == "table_set_default" ){
		//FIXME: deal with case where default action has parameters
		defaultAction = rule[1] + "_" + networkMap->currentNodeName + "_" + std::to_string(actionIDs[rule[1]]);
	    }

	    //TODO: Model other commands
	}

        //Clear rule components
        match = "";
        arguments = "";
        matchValues.clear();
        actionParameters.clear();


    }//End for each forwarding rule

    //Model call for default action.
    if(tableAdd){
	returnString += "{\n\t\t" + defaultAction + "(); //Default action\n";
        returnString += "\t\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 0;\n\t}";
    }
    else{
        returnString += "\t" + defaultAction + "(); //Default action\n";
        returnString += "\t" + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string(tableIDs[tableName]) + "_hit = 0;\n";
    }

    return returnString;
}


std::string CreateModel::p4TableToC(const IR::P4Table* table){
    std::string returnString = "";

    cstring tableName = table->name.toString();
    tableIDs[tableName] = tableCounter;
    tableCounter++;

    returnString += "//Table\n";

    //Model hit/miss state
    returnString += "int " + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string( tableIDs[tableName] ) + "_hit;\n\n";

    returnString += "void " + tableName + "_" + networkMap->currentNodeName + "_" + std::to_string( tableIDs[tableName] ) + "(){\n";

    std::map<cstring, std::vector<std::vector<std::string>>>::iterator it;
    it = forwardingRules.find(tableName);

    //If manipulating forwarding rules
    if( it != forwardingRules.end() ){
	
	const IR::IndexedVector<IR::Property> tableProperties = table->properties->properties;
	const IR::Key* keyList = NULL;

	for( auto prop : tableProperties ){
	    if( prop->value->is<IR::Key>() ){
		keyList = prop->value->to<IR::Key>();
	    }
	}

	returnString += actionListWithRulesOptimized(tableName, keyList);
    }
    else{
	//IF the current table is NOT manipulating forwarding rules
	//THEN all tables must be symbolic
	if( forwardingRules.size() > 0 ){
	    std::cout << "ERROR: Match-action tables in a node must be all either symbolic or concrete." << std::endl;
	    exit(1);
	}
	else{
	    returnString += actionListNoRules( table->getActionList() );
	}
    }

    returnString += "\n}\n\n";

    return returnString;
}


std::string CreateModel::protectedStructToC(const IR::Type_ProtectedStruct* pstruct){

    std::string returnString = "typedef struct {\n";

    IR::IndexedVector<IR::StructField> fields = pstruct->fields;

    for(auto field : fields){
	switch( getFieldType(field) ){
	    case TYPE_NAME:
		returnString += "\t" + typeNameToC(field->type->to<IR::Type_Name>()) + " " + field->name + ";\n";
		break;
	    case TYPE_BITS:
		//TODO: model fields with more than 64 bits properly
		const IR::Type_Bits* fieldType = field->type->to<IR::Type_Bits>();
                returnString += "\t" + bitSizeToType( fieldType->size ) + " " + field->name + " : " + std::to_string( fieldType->size ) + ";\n";
		break;
	    //TODO: model Type_Stack
	}
    }

    returnString += "} " + pstruct->name + ";\n";

    return returnString;
}


std::string CreateModel::variableToC( const IR::Declaration_Variable* var ){
    std::string returnString = "";

    //std::cout << "Variable found" << std::endl;
    
    //TODO: model other variable types
    if( var->type->is<IR::Type_Bits>() ){
        //std::cout << "Var type bits" << std::endl;
        const IR::Type_Bits* varType = var->type->to<IR::Type_Bits>();
        returnString += bitSizeToType( varType->size );
    }

    returnString += " " + var->name.toString();  

    if( var->initializer != nullptr ){
        //TODO: model variable initializer
    }
    else{ 
        returnString += ";\n\n";
    }

    return returnString;
}


Visitor::profile_t CreateModel::init_apply(const IR::Node *root){

    //model += "void " + modelName + "(){\n";
    //std::cout << model;

    std::string monitorName;
    int monitorType = 0;
    
    MonitorModel newModel;

    //Extract forwarding rules
    if( commandsFile != "" ){
	size_t beginPos, endPos, substrLen;

	std::string commandName, tableName; 
	std::string actionName, actionParameters;

	std::vector<std::string> matchList;

	std::string command;
	std::string matchString, match;

	std::ifstream cFile(commandsFile);

	if( cFile.is_open() ){
	    while( getline( cFile, command ) ){
		std::vector<std::string> parsedRule;

		beginPos = command.find(' ', 0);

		commandName = command.substr(0, beginPos);

		//TODO: Extract other command types (e.g., ?)
		if( commandName == "table_add" ){
		    //Get table name
		    endPos = command.find(' ', beginPos+1);
		    substrLen = endPos - beginPos;
		    tableName = command.substr(beginPos+1, substrLen-1);
		    beginPos = endPos;

		    //Get action name
		    endPos = command.find(' ', beginPos+1);
		    substrLen = endPos - beginPos;
		    actionName = command.substr(beginPos+1, substrLen-1);
		    beginPos = endPos;

		    //Parse table match
		    endPos = command.find("=>", beginPos);
		    substrLen = endPos - beginPos;
		    matchString = command.substr(beginPos+1, substrLen-1);
		    beginPos = endPos + 2;

		    //Parse action parameters
		    substrLen = command.length() - beginPos;

		    bool actionParamsParsed = false;

		    if( substrLen > 0 ){
		        actionParameters = command.substr(beginPos, substrLen);
			actionParamsParsed = true;
		    }

		    parsedRule.push_back(commandName);
		    parsedRule.push_back(actionName);
		    parsedRule.push_back(matchString);

		    if( actionParamsParsed ){
		        parsedRule.push_back(actionParameters);
		    }

		    forwardingRules[tableName].push_back(parsedRule);
		}
		else{
		    if( commandName == "table_set_default" ){
			//Get table name
			endPos = command.find(' ', beginPos+1);
			substrLen = endPos - beginPos;
			tableName = command.substr(beginPos+1, substrLen-1);
			beginPos = endPos;

			//Get action name
			endPos = command.find(' ', beginPos+1);
			substrLen = endPos - beginPos;
			actionName = command.substr(beginPos+1, substrLen-1);
			beginPos = endPos;

			//TODO: parse action parameters

			parsedRule.push_back(commandName);
			parsedRule.push_back(actionName);

			forwardingRules[tableName].push_back(parsedRule);
		    }//End table_set_default
		}//End table_add
	    }//while not end of file

	    cFile.close();
	}
	else{
	    std::cout << "ERROR: Unable to open commands file." << std::endl;
	}
    }//End of forwarding rule extraction


    //Get types and names of input variables (i.e., headers and metadata)
    if( root->is<IR::P4Program>() ){
	const IR::P4Program* prog = root->to<IR::P4Program>();

	const IR::P4Parser* parser;

	for( auto decl : prog->declarations ){
   	    if( decl->is<IR::P4Parser>() ){
		parser = decl->to<IR::P4Parser>();
		break;
	    }
        }

        const IR::ParameterList* paramList = parser->type->getApplyParameters();

        for( auto param : paramList->parameters ){
            if( param->hasOut() ){	    
	        std::string paramName = param->name.toString().c_str();
	        std::string paramType = param->type->toString().c_str();

	        if(paramType != "headers" && paramType != "packet_in"){
		    nodeMetaVars.push_back(paramName);

		    if( paramType == "standard_metadata_t" ){
			networkMap->stdMeta[networkMap->currentNodeName] = paramName + "_" + networkMap->currentNodeName;
		    }
		    else{//FIXME: now are assuming the V1Model (i.e., device receives two structures containing metadata)
			networkMap->metaType[networkMap->currentNodeName] = paramType + "_" + networkMap->currentNodeName;
		        networkMap->metaName[networkMap->currentNodeName] = paramName + "_" + networkMap->currentNodeName;
		    }
	        }
            }
        }//End for each parameter
    }


    //For each monitor
    for( auto monitor : P4boxIR->monitorMap ){

	//Model protected state
	if(not pStateOn){
	    for( auto param : *monitor.second->type->monitorParams ){
	        //TODO: Symbolize/declare 'Parameter' and 'Type_Name' separately
                if( param->hasOut() ){
		    //FIXME: allow only a single parameter (with same name) to be passed to all monitors.
		    //The reasoning is this parameter represents global protected state. Local protected 
		    //state can be defined using local property.
	            std::string paramName = param->name.toString().c_str();
	            std::string paramType = param->type->toString().c_str();

    	            //Declare parameters
		    if(paramType == "p4boxState"){
			if(!networkMap->headersOn){
			    //inputSymbolization += klee_make_symbolic( paramName );
			    networkMap->p4boxState = paramName;
	            	    inputDeclaration += paramType + " " + paramName + ";\n";
		    	    pStateOn = true;
			}
		    }
		    else{
			//FIXME: should never pass here
			inputSymbolization += klee_make_symbolic( paramName );
			inputDeclaration += paramType + " " + paramName + ";\n";
		    	pStateOn = true;
		    }
                }
	    }
	}

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

                            newModel.model += "void " + newModel.monitorName + "_" + networkMap->currentNodeName + "_before(){\n\t";
                            //newModel.model += controlBlockMonitorToC( beforeBlock->body );
                            newModel.model += blockStatementToC( beforeBlock->body );
                            newModel.model += "\n}\n\n";
                        }
                        else{
                            newModel.before = false;
                            const IR::MonitorControlAfter* afterBlock = property->value->to<IR::MonitorControlAfter>();

                            newModel.model += "void " + newModel.monitorName + "_" + networkMap->currentNodeName + "_after(){\n\t";
                            //newModel.model += controlBlockMonitorToC( afterBlock->body );
                            newModel.model += blockStatementToC( afterBlock->body );
                            newModel.model += "\n}\n\n";
                        }//End if before|after property

                        controlBlockMonitorModels.push_back( newModel );

                        break;
                    case PARSER_MONITOR:
			newModel.model = "";

			//Get property type (before/after)
                        if( property->value->is<IR::MonitorParserBefore>() ){
                            newModel.before = true;
                            //const IR::MonitorControlBefore* beforeBlock = property->value->to<IR::MonitorControlBefore>();

                            //newModel.model += "void " + newModel.monitorName + "_before(){\n\t";
                            
                            //newModel.model += blockStatementToC( beforeBlock->body );
                            //newModel.model += "\n}\n\n";
                        }
                        else{
                            newModel.before = false;
                            //const IR::MonitorControlAfter* afterBlock = property->value->to<IR::MonitorControlAfter>();

                            //newModel.model += "void " + newModel.monitorName + "_after(){\n\t";
                            
                            //newModel.model += blockStatementToC( afterBlock->body );
                            //newModel.model += "\n}\n\n";
                        }//End if before|after property

                        break;
                }
            }
            else{
                //Model local declarations (e.g., actions, tables, registers)
                const IR::MonitorLocal* localBlock = property->value->to<IR::MonitorLocal>();

                for( auto localDeclaration : localBlock->monitorLocals ){

                    //Actions
                    if( localDeclaration->is<IR::P4Action>() ){
                        locals.push_back( p4ActionToC( localDeclaration->to<IR::P4Action>() ) );
			//TODO: remove debug code
                        //std::cout << locals.back() << std::endl;
                    }
                    
                    //Tables
                    if( localDeclaration->is<IR::P4Table>() ){
                        //std::cout << "Model Match-action table" << std::endl;
                        locals.push_back( p4TableToC( localDeclaration->to<IR::P4Table>() ) );

			//TODO: remove debug code
                        //std::cout << locals.back() << std::endl;
                    }

		    if( localDeclaration->is<IR::Declaration_Instance>() ){
			locals.push_back( instantiationToC( localDeclaration->to<IR::Declaration_Instance>() ) );
		    }

                    //Variable declarations
                    if( localDeclaration->is<IR::Declaration_Variable>() ){
			locals.push_back( variableToC( localDeclaration->to<IR::Declaration_Variable>() ) );
		    }

                }//End for each local declaration

            }//End if checking the type of property

        }//End for each property 
    }//End for each monitor
  
    return Inspector::init_apply(root);
}


std::string CreateModel::assembleMonitors( std::string progBlock ){
    std::string returnString = "";
    std::string assembleBefore = "";
    std::string assembleAfter = "";

    unsigned lineNumber;
    std::map<unsigned, std::string> orderedBeforeMonitors;
    std::map<unsigned, std::string> orderedAfterMonitors;

    for( auto model : controlBlockMonitorModels ){
        if( model.monitoredBlock == progBlock ){
            //Assemble before and after monitors separately
            if( model.before == true ){
		lineNumber = P4boxIR->monitorDeclarations[model.monitorName].srcInfo.getStart().getLineNumber();
		orderedBeforeMonitors[lineNumber] = model.monitorName + "_" + networkMap->currentNodeName + "_before();\n\t";
            }
            else{
		lineNumber = P4boxIR->monitorDeclarations[model.monitorName].srcInfo.getStart().getLineNumber();
		orderedAfterMonitors[lineNumber] = model.monitorName + "_" + networkMap->currentNodeName + "_after();\n\t";
            }
        }
    }

    for( auto monitor : orderedBeforeMonitors ){
	returnString += monitor.second;
    }

    for( auto monitor : orderedAfterMonitors ){
	returnString += monitor.second;
    }

    return returnString;
}


void CreateModel::postorder(const IR::Declaration_Instance* instance){
    
    //Assemble monitors into the "main" function
    //Should order of programmable blocks NOT be hardcoded?

    //std::cout << "Dec instance type: " << instance->type->toString() << std::endl;

    if( instance->type->toString() == "V1Switch" ){
        mainFunctionModel += "void nodeModel_" + networkMap->currentNodeName + "(){\n\t";
	//mainFunctionModel += "symbolizeInputs_" + std::to_string(networkMap->deviceId) + "();\n\t";
        mainFunctionModel += assembleMonitors( "ingress" );
        mainFunctionModel += assembleMonitors( "egress" );
	//mainFunctionModel += "end_assertions();\n\t";
        mainFunctionModel += "\n}\n";

        //TODO: remove debug code
        //std::cout << mainFunctionModel << std::endl;
    }
}


void CreateModel::postorder(const IR::P4Parser* parser){

    //Symbolize parameters
    const IR::ParameterList* paramList = parser->type->getApplyParameters();
	
    for( auto param : paramList->parameters ){
	//TODO: Symbolize/declare 'Parameter' and 'Type_Name' separately
        if( param->hasOut() ){	    
	    std::string paramName = param->name.toString().c_str();
	    std::string paramType = param->type->toString().c_str();

    	    //Declare parameters
	    if(paramType == "headers"){
		if(!networkMap->headersOn){
		    //inputSymbolization += klee_make_symbolic( paramName );
		    networkMap->headers = paramName;
		    inputDeclaration += paramType + " " + paramName + ";\n";
		}
	    }
	    else{
		//Each node has its own metadata
		if( paramType == "standard_metadata_t" ){
		    inputDeclaration += "standard_metadata_t " + networkMap->stdMeta[networkMap->currentNodeName] + ";\n";
		}
		else{
		    //inputDeclaration += paramType + " " + paramName + ";\n";
		    inputDeclaration += networkMap->metaType[networkMap->currentNodeName] + " " + 
			     networkMap->metaName[networkMap->currentNodeName] + ";\n"; 
		}
	    }
        }

    }
}


void CreateModel::postorder(const IR::Type_Header* header){

    std::string returnString = "typedef struct {\n";
    returnString += "\tuint8_t isValid : 1;\n";

    IR::IndexedVector<IR::StructField> fields = header->fields;

    for(auto field : fields){
        if( field->type->is<IR::Type_Bits>() ){
	    //TODO: model fields with more than 64 bits properly
	    if( field->name != "$valid$" ){
		const IR::Type_Bits* fieldType = field->type->to<IR::Type_Bits>();
                returnString += "\t" + bitSizeToType( fieldType->size ) + " " + field->name + " : " + std::to_string( fieldType->size ) + ";\n";
	    }
	}
	//TODO: Model other field types
	switch( getFieldType(field) ){
	    case TYPE_NAME:
		std::string typeName = typeNameToC(field->type->to<IR::Type_Name>());
		returnString += "\t" + typeName + " " + field->name + " : " + std::to_string( typedefs[typeName] ) + ";\n";
		break;
        }
    }

    returnString += "} " + header->name + ";\n";
    modeledStructures.headers.push_back( returnString );

}


void CreateModel::postorder(const IR::Type_Struct* tstruct){
    std::string returnString = "typedef struct {\n";

    std::string nodeStruct = tstruct->name + "_" + networkMap->currentNodeName;
    IR::IndexedVector<IR::StructField> fields = tstruct->fields;

    for(auto field : fields){
	switch( getFieldType(field) ){
	    case TYPE_NAME:
		if( nodeStruct == networkMap->metaType[networkMap->currentNodeName] ){
		    returnString += "\t" + typeNameToC(field->type->to<IR::Type_Name>()) + "_" + networkMap->currentNodeName +
					" " + field->name + ";\n";
		}
		else{
		    returnString += "\t" + typeNameToC(field->type->to<IR::Type_Name>()) + " " + field->name + ";\n";
		}
		break;
	    case TYPE_BITS:
		//TODO: model fields with more than 64 bits properly
		const IR::Type_Bits* fieldType = field->type->to<IR::Type_Bits>();
                returnString += "\t" + bitSizeToType( fieldType->size ) + " " + field->name + " : " + std::to_string( fieldType->size ) + ";\n";
		break;
	    //TODO: model Type_Stack
	}
    }

    //Model headers and p4box state only once
    if(tstruct->name == "headers" || tstruct->name == "p4boxState" || tstruct->name == "standard_metadata_t"){
	if(!networkMap->headersOn){
	    returnString += "} " + tstruct->name + ";\n";
    	    modeledStructures.structs.push_back( returnString );
	}
    }
    else{
	returnString += "} " + tstruct->name + "_" + networkMap->currentNodeName + ";\n";
	modeledStructures.structs.push_back( returnString );
    }
}


void CreateModel::postorder(const IR::Type_Typedef* tdef){
    std::string nodeName = tdef->name.toString().c_str();
    typedefs[nodeName] = tdef->type->width_bits();

    std::string returnString = "typedef " + bitSizeToType( tdef->type->width_bits() ) + " " + nodeName + ";\n";
    modeledStructures.typedefs.push_back( returnString );
}


std::string CreateModel::instantiationToC(const IR::Declaration_Instance* inst){
    std::string returnString = "";

    //TODO: model other instantiation types (e.g., Type_Name)
    if(inst->type->is<IR::Type_Specialized>()){

	const IR::Type_Specialized* instType = inst->type->to<IR::Type_Specialized>();
	std::string baseType = instType->baseType->toString().c_str();

	if( baseType == "register" ){

	    //Get size of register array
	    const IR::Vector<IR::Expression>* args = inst->arguments;
	    const IR::Constant* regSize;

	    for( auto arg : *args ){
		if( arg->is<IR::Constant>() ){
		    regSize = arg->to<IR::Constant>();
		    break;
		}
	    }

	    registerSize[inst->Name()] = std::stoi( constantToC(regSize) );

	    //Get register width
	    int registerWidth = 0;

	    for( auto typeArg : *instType->arguments ){
		if( typeArg->is<IR::Type_Bits>() ){
		    const IR::Type_Bits* widthType = typeArg->to<IR::Type_Bits>();

		    registerWidth = widthType->width_bits();
		}
	    }

	    registerIDs[inst->Name()] = registerCounter;
    	    registerCounter++;

	    returnString += bitSizeToType( registerWidth ) + "* " + inst->Name() + "_" + networkMap->currentNodeName 
				+ "_" + std::to_string( registerIDs[inst->Name()] ) + ";\n\n";

	    networkMap->registerInitializations += "\t" + inst->Name() + "_" + networkMap->currentNodeName 
				+ "_" + std::to_string( registerIDs[inst->Name()] ) + " = v1model_declare_register(" + 
				std::to_string(registerWidth) + ", " + constantToC(regSize) + ");\n";

	}//End register instantiation

    }//End if Type_Specialized

    return returnString;
}


std::string CreateModel::insertPreamble(void){
    std::string returnString = "";

    returnString += "#include <stdio.h>\n";
    returnString += "#include <stdint.h>\n";
    returnString += "#include \"klee/klee.h\"\n";

    returnString += "\n";

    return returnString;
}


/*std::string CreateModel::insertAssertionChecks(void){
    std::string returnString = "";

    returnString += "void assert_error(char* msg){\n";
    returnString += "\tprintf(\"%s\", msg);\n";
    returnString += "}\n\n";

    returnString += "void end_assertions_" + networkMap->currentNodeName + "(){\n";
    
    for( auto logicExpr : logicalExpressionList ){
	returnString += "\tif( !(" + logicExpr + ") ) assert_error(\"" + logicExpr + "\");\n";
    }

    returnString += "}\n";

    return returnString;
}*/


void CreateModel::end_apply(const IR::Node* node){

    /* Create model for a complete program */

    //Check whether device contains a new architecture model
    bool archFound = false;

    for( auto archElem : networkMap->archTypes ){
	if( archType == archElem ){
	    archFound = true;
	    break;
	}
    }

    if( !archFound ){
	networkMap->archTypes.push_back( archType.c_str() );
    }

    //model += insertPreamble();

    //model += globalDeclarations;
    //model += "\n";

    //if(!networkMap->headersOn){
    //    model += "void end_assertions();\n\n";
    //}

    //Forward declare tables and actions
    //TODO: forward declare parser states
    //FIXME: check whether it is necessary to forward declare tables and actions

    /*if( tableIDs.size() > 0 || actionIDs.size() > 0 ){
        model += "// Forward declarations for table and action calls";
    }

    for( auto table : tableIDs ){
	model += "\nvoid " + table.first + "_" + std::to_string(table.second) + "();";
    }

    for( auto action : actionIDs ){
	model += "\nvoid " + action.first + "_" + std::to_string(action.second) + "();";
    }
    
    model += "\n\n";*/

    model += "// Headers and metadata\n";
    //Add only once into the model
    if(!networkMap->headersOn){

        for( auto tdef : modeledStructures.typedefs ){
	    model += tdef;
    	    model += "\n";
        }

        for( auto headerModel : modeledStructures.headers ){
            model += headerModel;
	    model += "\n";
         }

        //Model protected state
        for( auto pstruct : P4boxIR->pStructMap ){
	    modeledStructures.structs.push_back( protectedStructToC(pstruct.second) );
        }
    }

    for( auto structModel : modeledStructures.structs ){
	model += structModel;
	model += "\n";
    }

    model += inputDeclaration;
    model += "\n";

    for( auto local : locals ){
        model += local;
    }

    for( auto monitor : controlBlockMonitorModels ){
        model += monitor.model;
    }

    //model += inputSymbolization;
    //model += "\n";

    //Model assertion checks
    //model += insertAssertionChecks();
    model += "\n";
    model += mainFunctionModel;
    
    networkMap->nodeModels[networkMap->currentNodeName] = model;
    networkMap->headersOn = true;
}

}//End P4 namespace



















