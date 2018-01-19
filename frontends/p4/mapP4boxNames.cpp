
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

/*const IR::Node* MapNames::postorder(IR::Path* origPath){
    printf("Path: %s\n", origPath->toString().c_str());

    return origPath;
}*/

/*const IR::Node* MapNames::postorder(IR::Type_Name* origTypeName){
    printf("Type name: %s\n", origTypeName->toString().c_str());

    return origTypeName;    
}*/

/*const IR::Node* MapNames::postorder(IR::PathExpression* origPathExpression){
    printf("Path expression: %s\n", origPathExpression->toString().c_str());

    return origPathExpression;
}*/


const IR::Node* MapP4boxNames::postorder(IR::Member* origMember){
    printf("Member: %s\n", origMember->toString().c_str());

    return origMember;    

}

}//End P4 namespace



















