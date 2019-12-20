
#ifndef _NETMAP_H_
#define _NETMAP_H_

#include "ir/ir.h"

class NetMap {

  public:
    NetMap(){
	deviceId = 1;
	headersOn = false;
    }

    int deviceId;
    bool headersOn;

    std::map<std::string, std::string> nodeModels;
    std::map<std::string, std::string> stdMeta;
    std::string currentNodeName;
};

#endif /* _NETMAP_H_ */
