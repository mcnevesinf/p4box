
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

const IR::Node* MapProtectedInstantiations::postorder(IR::Protected_Declaration_Instance* pInst){
    auto mappedInst = new IR::Declaration_Instance(pInst->name, pInst->type, pInst->arguments);

    return mappedInst;
}

}
