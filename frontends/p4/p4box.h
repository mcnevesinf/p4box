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
#include <bitset>

//#include "setup.h"
#include "../common/options.h"
#include "../common/netMap.h"

#include "createBuiltins.h"
#include "directCalls.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/common/constantFolding.h"
#include "ir/ir.h"
#include "typeChecking/typeChecker.h"

#define CONTROL_BLOCK_MONITOR 1
#define PARSER_MONITOR 2

#define TYPE_NAME 1
#define TYPE_BITS 2

namespace P4 {

struct SupervisorMap {
    std::map<cstring, const IR::Type_ProtectedStruct*> pStructMap;
    std::map<cstring, const IR::P4Monitor*> monitorMap; //<monitor-name, >
    std::map<cstring, IR::ID> monitorDeclarations; //<monitor-name, line/column-number>
    std::map<cstring, cstring> blockMap; //<monitor-name, block-name>
    std::map<cstring, IR::ID> programDeclarations; //<block-name, line/column-number>
    std::vector<cstring> parserMembers; //Parsers in a program
    std::vector<cstring> controlMembers; //Control blocks in a program
    std::vector<cstring> externMembers; //Extern functions in a program
    std::map<cstring, std::vector<cstring>> associatedTypes; //<extern-monitor-name, associated-types>
    std::map<cstring, cstring> associatedBlocks; //<extern-monitor-name, associated-block>
    std::map<cstring, int> monitorTypeMap; //<monitor-name, monitor-type>
    std::vector<cstring> candidateTypes;
    cstring hostStructType;
    std::map<cstring, cstring> nameMap; //<block-name, parameter-name>
    std::map<cstring, std::vector<cstring>> pStateNames; //<monitor-name, monitor-parameters>
    bool pStateInserted;
    int monitorStartStateCounter;
};


struct MonitorModel {
    std::string monitorName;
    std::string monitoredBlock;
    bool before; //After if false
    std::string qualifier;
    std::string model;
};


struct DataModel{
    std::vector<std::string> headers;
    std::vector<std::string> structs;
    std::vector<std::string> typedefs;
};


class GetProgramDeclarations final : public Inspector {

    SupervisorMap *P4boxIR;

  public:
    GetProgramDeclarations( SupervisorMap *map ) { 
	setName("GetProgramDeclarations"); 
        P4boxIR = map;
    }

    using Inspector::postorder;

    void postorder(const IR::P4Parser* parser) override;
    void postorder(const IR::P4Control* control) override;
    void postorder(const IR::Method* method) override;
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

  private:

    /* Functions related to the insertion of PARSER monitors */
    void getParserMonitorData( cstring monitorName,
                               IR::IndexedVector<IR::Declaration>* instrumentLocals,
                               IR::IndexedVector<IR::ParserState>* instrumentBefore,
                               IR::IndexedVector<IR::ParserState>* instrumentAfter,
                               cstring* beforeStateName,
                               cstring* afterStateName );

    IR::IndexedVector<IR::Declaration>* instrumentParserLocals( 
                               IR::IndexedVector<IR::Declaration> instrumentLocals,
                               IR::IndexedVector<IR::Declaration>* parserLocals );

    IR::IndexedVector<IR::ParserState>* instrumentParserBefore( 
                               cstring beforeStateName,
                               IR::IndexedVector<IR::ParserState> instrumentBefore,
                               IR::IndexedVector<IR::ParserState>* newParserStateList );

    IR::IndexedVector<IR::ParserState>* instrumentParserAfter(
                               cstring afterStateName,
                               IR::IndexedVector<IR::ParserState> instrumentAfter,
                               IR::IndexedVector<IR::ParserState>* newParserStateList );

    IR::ParserState* changeTransition( const IR::ParserState* parserState,
                                       cstring monitoredStateName,
                                       cstring monitorInitialState );

    IR::ParserState* changeAfterTransition( 
                               const IR::ParserState* parserState, 
                               cstring originalStateName, 
                               const IR::Expression* monitoredPathTransition );

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

    const IR::Node* postorder(IR::Path* origPath) override;
//    const IR::Node* postorder(IR::Type_Name* origTypeName) override;
//    const IR::Node* postorder(IR::PathExpression* origPathExpression) override;
//    const IR::Node* postorder(IR::Member* origMember) override;

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


class RemoveControlBlockNames : public Transform {

    SupervisorMap *P4boxIR;

  public:
    RemoveControlBlockNames( SupervisorMap *map ) { 
	setName("RemoveControlBlockNames"); 
        P4boxIR = map;
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::Member* origMember) override;
};


class InsertExternMonitors : public Transform {

    SupervisorMap *P4boxIR;
    TypeMap *auxTypeMap;

  public:
    InsertExternMonitors( SupervisorMap *map, TypeMap *typeMap ) { 
	setName("InsertExternMonitors"); 
        P4boxIR = map;
        auxTypeMap = typeMap;
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::MethodCallStatement* origCall) override;
};


class ResolveMonitorReferences final : public Inspector {

    SupervisorMap *P4boxIR;

  public:
    ResolveMonitorReferences( SupervisorMap *map ) { 
	setName("ResolveMonitorReferences"); 
        P4boxIR = map;
    }

    using Inspector::init_apply;
    using Inspector::postorder;

    Visitor::profile_t init_apply(const IR::Node *root) override;
    //void postorder(const IR::Member* origMember) override;
};


class ValidateSupervisor final : public Inspector {

    SupervisorMap *P4boxIR;

  public:
    ValidateSupervisor( SupervisorMap *map ) { 
	setName("ValidateSupervisor"); 
        P4boxIR = map;
    }

    using Inspector::postorder;

    void postorder(const IR::MonitoredBlock* mBlock) override;
    void postorder(const IR::Type_Monitor* monitor) override;
};


class MapProtectedInstantiations : public Transform {

    SupervisorMap *P4boxIR;

  public:
    MapProtectedInstantiations( SupervisorMap *map ) { 
	setName("MapProtectedInstantiations"); 
        P4boxIR = map;
    }

    using Transform::postorder;

    const IR::Node* postorder(IR::Protected_Declaration_Instance* pInst) override;
};


class P4boxTest final : public Inspector {

    SupervisorMap *P4boxIR;

  public:
    P4boxTest( SupervisorMap *map ) { 
	setName("P4boxTest"); 
        P4boxIR = map;
    }

    using Inspector::postorder;

    void postorder(const IR::P4Parser* parser) override;
    void postorder(const IR::ParserState* pstate) override;
    void postorder(const IR::Member* origMember) override;
    void postorder(const IR::P4Control* control) override;
};


class CreateModel final : public Inspector {

  private:
    SupervisorMap *P4boxIR;
    NetMap* networkMap;

    int nodeCounter;
    int tableCounter;
    int actionCounter;
    int assertionCounter;
    std::map<cstring, int> tableIDs;
    std::map<cstring, int> actionIDs;
    std::map<cstring, int> typedefs;
 
    //Map containing forwarding rules
    //Key : table name
    //Value : rule components (command, action, match, parameters)
    std::map<cstring, std::vector<std::vector<std::string>>> forwardingRules;

    cstring modelName;
    cstring commandsFile;
    std::string model;
    std::string mainFunctionModel;
    std::string inputDeclaration;
    std::string inputSymbolization;
    std::string globalDeclarations;

    std::vector<std::string> logicalExpressionList;

    bool pStateOn;

    //std::vector<std::string> monitorCalls;

    //Need to keep: 	1) monitor name; 2) monitored block; 3) before/after; 
    // 			4) qualifier (e.g., parser state, extern type); 5) C model
    std::vector<MonitorModel> controlBlockMonitorModels;
    std::vector<MonitorModel> parserMonitorModels;
    std::vector<std::string> locals;

    DataModel modeledStructures;

    int getMonitorType(std::string monitoredBlockName);
    int getFieldType(const IR::StructField* field);

    std::string actionListNoRules(const IR::ActionList* actionList);
    std::string actionListWithRules(cstring tableName, const IR::Key* keyList);
    std::pair<std::string, std::string> assertionToC(std::string assertString);
    std::string blockStatementToC(IR::BlockStatement body);
    std::string blockStatementToC(const IR::BlockStatement* body);
    std::string constantToC(const IR::Constant* intConst);
    std::string convertExactMatchValue(std::string value);
    std::string exprToC(const IR::Expression* expr);
    std::string insertAssertionChecks(void);
    std::string insertPreamble(void);
    std::string methodCallExpressionToC(const IR::MethodCallExpression* methodCall);
    std::string protectedStructToC(const IR::Type_ProtectedStruct* pstruct);
    std::string replaceAllOccurrences(std::string oldString, char oldChar, char newChar);
    std::string stringLiteralToC(const IR::StringLiteral* strLit);
    std::string assignmentStatemToC(const IR::AssignmentStatement* assignStatem);
    std::string lvalueToC(const IR::Expression* lvalue);
    std::string assign(const IR::AssignmentStatement* assignStatem);
    std::string p4ActionToC(const IR::P4Action* action);
    std::string p4ParserToC(const IR::P4Parser* parser);
    std::string p4TableToC(const IR::P4Table* table);
    std::string pathToC(const IR::PathExpression* pathExpr);
    std::string memberToC(const IR::Member* member);
    std::string typeNameToC(const IR::Type_Name* tName);
    
    bool isExternal(bool placeholder);
    std::string bitSizeToType( int size );
    std::string klee_make_symbolic(std::string var);

  public:
    CreateModel( const CompilerOptions& options, SupervisorMap *map, NetMap& networkModelMap ){
        modelName = options.file;
	commandsFile = options.commandsFile;
        P4boxIR = map;
	networkMap = &networkModelMap;

        nodeCounter = 1;
	tableCounter = 1;
        actionCounter = 1;
	assertionCounter = 1;
        mainFunctionModel = "";
        model = "";
	inputDeclaration = "";
	inputSymbolization = "void symbolizeInputs(){\n";
	globalDeclarations = "";

	pStateOn = false;

        std::cout << "Testing model creation\n";
        setName("CreateModel");
    }

    using Inspector::init_apply;
    using Inspector::postorder;

    Visitor::profile_t init_apply(const IR::Node *root) override;
    void postorder(const IR::Declaration_Instance* instance) override;
    void postorder(const IR::P4Parser* parser) override;
    void postorder(const IR::Type_Header* header) override;
    void postorder(const IR::Type_Struct* tstruct) override;
    void postorder(const IR::Type_Typedef* tdef) override;
    std::string assembleMonitors( std::string progBlock );
    void end_apply(const IR::Node* node) override;

};


class ElementModelSetup : public PassManager {

  private:
    SupervisorMap P4box;

  public:

    ElementModelSetup( const CompilerOptions& options, const IR::P4Program auxProgram, NetMap& networkModelMap ){
        passes.push_back(new P4::GetProgramDeclarations( &P4box ));
        passes.push_back(new P4::ValidateSupervisor( &P4box ));
        passes.push_back(new P4::GetSupervisorNodes( &P4box ));
        passes.push_back(new P4::CreateModel( options, &P4box, networkModelMap ));

        std::cout << "Creating element model" << std::endl;
        setName("ElementModelSetup");
    }

}; //End element model setup



class P4boxSetup : public PassManager {

 private:
    SupervisorMap P4box;
    SupervisorMap auxP4box;
    ReferenceMap  auxRefMap;
    TypeMap       auxTypeMap;

 public:
    
    P4boxSetup( const IR::P4Program auxProgram ) {
        P4box.pStateInserted = false;
        P4box.monitorStartStateCounter = 0;
  
        PassManager auxPasses = {
            new GetSupervisorNodes( &auxP4box ),
            new CreateBuiltins(),
            new ResolveReferences( &auxRefMap ),
            new ConstantFolding( &auxRefMap, nullptr ),
            new InstantiateDirectCalls( &auxRefMap ),
            new ResolveReferences( &auxRefMap ),
            new TypeInference(&auxRefMap, &auxTypeMap, false)
        };
  
        auxProgram.apply(auxPasses);

        passes.push_back(new P4::GetProgramDeclarations( &P4box ));
        passes.push_back(new P4::ValidateSupervisor( &P4box ));
        passes.push_back(new P4::GetSupervisorNodes( &P4box ));
        passes.push_back(new P4::ResolveMonitorReferences( &P4box ));
        /* Must keep this order because name maps are built when 
           instrumenting programs with data plane monitors and protected state */
        passes.push_back(new P4::GetCandidateStructs( &P4box ));
        passes.push_back(new P4::InsertProtectedState( &P4box ));

        passes.push_back(new P4::InsertExternMonitors( &P4box, &auxTypeMap ));
        passes.push_back(new P4::InsertDataPlaneMonitors( &P4box ));
        passes.push_back(new P4::MapP4boxNames( &P4box ));
        passes.push_back(new P4::MapProtectedInstantiations( &P4box ));
    
        passes.push_back(new P4::P4boxTest( &P4box ));

        setName("P4boxSetup");
    }

}; //End P4boxSetup


} //End P4 namespace


#endif /* _P4_BOX_H_ */
