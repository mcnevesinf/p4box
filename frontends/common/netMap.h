
#ifndef _NETMAP_H_
#define _NETMAP_H_

#include "ir/ir.h"

class NetMap {

  public:
    NetMap(){
	deviceId = 1;
	headersOn = false;
	headersInclude = "";
	std::string netwideStructs = "";

	p4boxState = "";
	registerInitializations = "";
	globalDeclarations = "";
	forwardDeclarations = "";
    }

    int deviceId;
    bool headersOn;
    std::string headersInclude;
    std::string netwideStructs;

    std::vector<std::string> archTypes;
    std::map<std::string, std::string> nodeIncludeModels;
    std::map<std::string, std::string> nodeModels;
    std::map<std::string, std::string> stdMeta;
    std::map<std::string, std::string> metaName;
    std::map<std::string, std::string> metaType;
    std::string currentNodeName;
    std::string p4boxState;
    std::string headers;
    std::string registerInitializations;
    std::string globalDeclarations;
    std::string forwardDeclarations;
    std::vector<std::string> logicalExpressionList;
};

#endif /* _NETMAP_H_ */
