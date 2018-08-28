
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

void P4boxTest::postorder(const IR::P4Parser* parser){
    const IR::IndexedVector<IR::Declaration> parserLocals = parser->parserLocals;

    //printf("Number of locals in parser: %lu\n", parserLocals.size());
}

void P4boxTest::postorder(const IR::Member* origMember){
    //printf("(P4box) Member name: %s\n", origMember->toString().c_str());
}

void P4boxTest::postorder(const IR::ParserState* pstate){
    //printf("State name: %s\n", pstate->name.name.c_str());
}


void P4boxTest::postorder(const IR::P4Control* control){
    std::string rawControlName = control->toString().c_str();
    rawControlName.replace(0, 8, "");
    cstring controlName = rawControlName;

    //printf("Control name: %s\n", controlName.c_str());

    /*if( controlName == "MyDeparser" ){
        const IR::IndexedVector<IR::StatOrDecl> components = control->body->components;

        for( auto component : components ){            
            if(component->is<IR::MethodCallStatement>()){
                const IR::MethodCallStatement* methodCall = component->to<IR::MethodCallStatement>();
                const IR::Vector<IR::Expression>* arguments = methodCall->methodCall->arguments;

                for( auto argument : *arguments ){
                    printf("Method call param: %s\n", argument->toString().c_str());
                }
                
            }
        }
    }*/
}


}//End P4 namespace





















