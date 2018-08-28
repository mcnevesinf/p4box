
#include "p4box.h"
#include "ir/ir.h"


namespace P4 {

void ValidateSupervisor::postorder(const IR::Type_Monitor* monitor){

}



void ValidateSupervisor::postorder(const IR::MonitoredBlock* mBlock){

    cstring monitoredBlockName = mBlock->blockName->monitoredBlockName;

    std::vector<cstring>::iterator itExtern;
    std::vector<cstring>::iterator itControl;
    std::vector<cstring>::iterator itParser;

    itExtern = std::find(P4boxIR->externMembers.begin(), P4boxIR->externMembers.end(), monitoredBlockName);
    itControl = std::find(P4boxIR->controlMembers.begin(), P4boxIR->controlMembers.end(), monitoredBlockName);
    itParser = std::find(P4boxIR->parserMembers.begin(), P4boxIR->parserMembers.end(), monitoredBlockName);

    auto monitor = findContext<IR::P4Monitor>();
    const IR::IndexedVector<IR::MonitorProperty>* properties = monitor->properties;

    /* Before and after parser states can appear only in parser monitors */
    if( itParser == P4boxIR->parserMembers.end() ){
        for( auto property : *properties ){
            if( property->value->is<IR::MonitorParserBefore>() ){
                const IR::MonitorParserBefore* parserBefore = property->value->to<IR::MonitorParserBefore>();

                ::error("Property before %1% cannot appear in monitor %2%", parserBefore->stateName, monitor->name.name);
            }

            if( property->value->is<IR::MonitorParserAfter>() ){
                const IR::MonitorParserAfter* parserAfter = property->value->to<IR::MonitorParserAfter>();

                ::error("Property after %1% cannot appear in monitor %2%", parserAfter->stateName, monitor->name.name);
            }
        }
    }

    /* Check if monitoring a valid program block (i.e., parser, control 
        or extern) */
    if( monitoredBlockName != "emit" and
        itExtern == P4boxIR->externMembers.end() and
        itControl == P4boxIR->controlMembers.end() and
        itParser == P4boxIR->parserMembers.end() ){
        ::error("Monitor %1% is attached to an invalid block: %2%", monitor->name.name, monitoredBlockName);
    }

    /* Control and parser monitors cannot be associated with another 
        block (operator "at") */
    if ( monitoredBlockName != "emit" and itExtern == P4boxIR->externMembers.end() ){
        if( itControl != P4boxIR->controlMembers.end() and mBlock->specialization != "M_NO_SPEC" ){
            ::error("Control monitor %1% cannot be associated with another program block", monitor->name.name);
        }

        if( itParser != P4boxIR->parserMembers.end() and mBlock->specialization != "M_NO_SPEC" ){
            ::error("Parser monitor %1% cannot be associated with another program block", monitor->name.name);
        }
    }

    /* Extern monitors cannot have local declarations */
    if( monitoredBlockName == "emit" or itExtern != P4boxIR->externMembers.end() ){
        for( auto property : *properties ){
            if( property->value->is<IR::MonitorLocal>() ){
                ::error("Extern monitor %1% cannot have local declarations", monitor->name.name);
            }
        }
    }

}

}//End P4 namespace














