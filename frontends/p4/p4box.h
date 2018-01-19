#ifndef _P4_BOX_H_
#define _P4_BOX_H_

#include <utility>
#include <tuple>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <map>
#include <algorithm>

//#include "setup.h"
#include "ir/ir.h"

#define BLOCK_MONITOR 1

namespace P4 {

struct SupervisorMap {
    std::map<cstring, const IR::Type_ProtectedStruct*> pStructMap;
    std::map<cstring, const IR::P4Monitor*> monitorMap;
    std::map<cstring, cstring> blockMap; //<monitor-name, block-name>
    std::map<cstring, int> monitorTypeMap; //<monitor-name, monitor-type>
    std::vector<cstring> candidateTypes;
    cstring hostStructType;
    std::map<cstring, cstring> nameMap; //<block-name, parameter-name>
    bool pStateInserted;
};

class GetSupervisorNodes final : public Transform {

    SupervisorMap *P4boxIR;

  public:
    GetSupervisorNodes( SupervisorMap *map ) { 
	setName("GetSupervisorNodes"); 
        P4boxIR = map;
    }

    using Transform::postorder;
    void end_apply(const IR::Node* node) override;

    const IR::Node* postorder(IR::P4Program* program) override;

};


class InsertDataPlaneMonitors final : public Transform {

    SupervisorMap *P4boxIR;

  public:
    InsertDataPlaneMonitors( SupervisorMap *map ) { 
	setName("InstrumentProgramBlocks"); 
        P4boxIR = map;
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::MethodCallStatement* methodCall) override; 

};



class InsertProtectedState final : public Transform {

    SupervisorMap *P4boxIR;

  public:
    InsertProtectedState( SupervisorMap *map ) { 
	setName("InsertProtectedState"); 
        P4boxIR = map;
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::Type_Struct* origStruct) override;

};


class MapP4boxNames final : public Transform {

    SupervisorMap *P4boxIR;

  public:
    MapP4boxNames( SupervisorMap *map ) { 
	setName("MapP4boxNames"); 
        P4boxIR = map;
    }

    using Transform::postorder;

//    const IR::Node* postorder(IR::Path* origPath) override;
//    const IR::Node* postorder(IR::Type_Name* origTypeName) override;
//    const IR::Node* postorder(IR::PathExpression* origPathExpression) override;
    const IR::Node* postorder(IR::Member* origMember) override;

};


class GetCandidateStructs final : public Inspector {

    SupervisorMap *P4boxIR;

  public:
    GetCandidateStructs( SupervisorMap *map ) { 
	setName("GetCandidateStructs"); 
        P4boxIR = map;
    }

    using Inspector::postorder;

    void postorder(const IR::P4Control* control) override;
};


class P4boxSetup : public PassManager {

 private:
    SupervisorMap P4box;

 public:
    
    P4boxSetup( ) {
        P4box.pStateInserted = false;

        passes.push_back(new P4::GetSupervisorNodes( &P4box ));
        /* Must keep this order because name maps are built when 
           instrumenting programs with data plane monitors and protected state */
        passes.push_back(new P4::GetCandidateStructs( &P4box ));
        passes.push_back(new P4::InsertProtectedState( &P4box ));
        passes.push_back(new P4::InsertDataPlaneMonitors( &P4box ));
        passes.push_back(new P4::MapP4boxNames( &P4box ));
        setName("P4boxSetup");
    }

}; //End P4boxSetup


} //End P4 namespace


#endif /* _P4_SUPERVISORSETUP_H_ */
