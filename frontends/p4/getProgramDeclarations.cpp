
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

void GetProgramDeclarations::postorder(const IR::P4Parser* parser){

    /* Check if this is a top-level parser */
    bool topLevelParser = false;
    const IR::Node* parentNode = getContext()->node;

    if( parentNode->is<IR::P4Program>() ){
        topLevelParser = true;
    }

    /* Get parser decl information */
    if( topLevelParser ){
        std::string parserName = parser->toString().c_str();
        parserName.replace(0, 7, "");

        P4boxIR->programDeclarations[parserName] = parser->name;
        P4boxIR->parserMembers.push_back( parser->name );
    }

}


void GetProgramDeclarations::postorder(const IR::P4Control* control){

    /* Check if this is a top-level control block */
    bool topLevelControl = false;
    const IR::Node* parentNode = getContext()->node;

    if( parentNode->is<IR::P4Program>() ){
        topLevelControl = true;
    }

    /* Get control decl information */
    if( topLevelControl ){
        std::string controlName = control->toString().c_str();
        controlName.replace(0, 8, "");

        P4boxIR->programDeclarations[controlName] = control->name;
        P4boxIR->controlMembers.push_back( control->name );
    }

}


void GetProgramDeclarations::postorder(const IR::Method* method){

    cstring methodName = method->name.name;

    P4boxIR->externMembers.push_back( methodName );
}

}//End P4 namespace


























