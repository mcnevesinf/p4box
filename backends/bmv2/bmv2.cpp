/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdio.h>
#include <string>
#include <iostream>

#include "ir/ir.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "backend.h"
#include "midend.h"
#include "options.h"
#include "JsonObjects.h"

//P4BOX: manage topology information
#include "/usr/include/boost/graph/graphviz.hpp"
#include "/usr/include/boost/graph/adjacency_list.hpp"
#include "/usr/include/boost/graph/depth_first_search.hpp"
#include "/usr/include/boost/graph/breadth_first_search.hpp"
#include "/usr/include/boost/version.hpp"


using namespace boost;

namespace boost {
    enum vertex_program_t { vertex_program };
    enum vertex_commands_t { vertex_commands };
    enum vertex_arch_t { vertex_arch };

    enum edge_sport_t { edge_sport };
    enum edge_dport_t { edge_dport };

    BOOST_INSTALL_PROPERTY( vertex, program );
    BOOST_INSTALL_PROPERTY( vertex, commands );
    BOOST_INSTALL_PROPERTY( vertex, arch );
    BOOST_INSTALL_PROPERTY( edge, sport );
    BOOST_INSTALL_PROPERTY( edge, dport );
}

            //Vertex properties
            typedef boost::property< vertex_program_t, std::string > DataPlaneProgram;
	    typedef boost::property< vertex_commands_t, std::string, DataPlaneProgram > ForwardingRules;
	    typedef boost::property< vertex_arch_t, std::string, ForwardingRules > Arch;
            typedef boost::property< vertex_name_t, std::string, Arch > vertex_p;

	    //Edge properties
	    typedef boost::property< edge_sport_t, int > SrcPort;
	    typedef boost::property< edge_dport_t, int, SrcPort > DstPort;
	    typedef boost::property< edge_name_t, std::string, DstPort > edge_p;

            //Graph properties
            typedef boost::property< graph_name_t, std::string > graph_p;

            //Adjacency list containing the graph that represents the network topology
            typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS,
                                       vertex_p, edge_p, graph_p > graph_t;

            typedef boost::graph_traits< graph_t >::vertex_descriptor Vertex;
	    typedef boost::graph_traits< graph_t >::edge_descriptor Edge;
            typedef boost::graph_traits< graph_t >::vertex_iterator vertex_iter;
            typedef boost::graph_traits< graph_t >::edge_iterator edge_iter;

class custom_bfs_visitor : public boost::default_bfs_visitor{

    boost::property_map< graph_t, vertex_name_t >::type nodeName;
    boost::property_map< graph_t, edge_sport_t >::type srcPort;
    boost::property_map< graph_t, edge_dport_t >::type dstPort;

    NetMap networkModelMap;
    std::string* model;
    int level;

    std::string tabSpacing(int level){
	std::string returnString = "";

	for(int i = 0; i < level; i++){
	    returnString += "\t";
	}

	return returnString;
    }

    public:

	custom_bfs_visitor(graph_t& g, std::string* netModel, NetMap netModelMap){
	    nodeName = get( vertex_name, g);
	    srcPort = get( edge_sport, g );
	    dstPort = get( edge_dport, g );
	    model = netModel;
	    networkModelMap = netModelMap;

    	    level = 1;
	}

	void discover_vertex(Vertex u, const graph_t& g){
	    //std::cout << "Visiting node " << nodeName[u] << std::endl;
	    *model += "void walk_" + nodeName[u] + "(){\n";
	    *model += "\t//Process node model\n";
	    *model += "\tnodeModel_" + nodeName[u] + "();\n\n";

	    std::pair<edge_iter, edge_iter> es;
	    Vertex srcVertex, dstVertex;

	    //For each edge
	    for( es = boost::edges(g); es.first != es.second; ++es.first ){
		Edge e = *es.first;
		srcVertex = boost::source(e, g);
		dstVertex = boost::target(e, g);
		
		//Model edges that leave current node
		if( u == srcVertex ){
		    //std::cout << "Src vertex: " << srcVertex << std::endl;
		    //std::cout << "Edge src port: " << srcPort[e] << std::endl;
		    *model += tabSpacing(level) + "if( " + networkModelMap.stdMeta[nodeName[u]] + ".egress_spec == " + std::to_string( srcPort[e] ) + " ){\n";
		    *model += tabSpacing(level+1) + networkModelMap.stdMeta[nodeName[dstVertex]] + ".ingress_port = " + std::to_string( dstPort[e] ) + ";\n";
		    *model += tabSpacing(level+1) + "walk_" + nodeName[dstVertex] + "();\n";
		    *model += tabSpacing(level) + "}\n";
		    *model += tabSpacing(level) + "else {\n";
		    level++;
		}
	    }

	    for(int i = level-1; i >= 1; i--){
		*model += tabSpacing(i) + "}\n";
	    }	    
	    level = 1;
	    *model += "}\n\n";
	}

};

class custom_dfs_visitor : public boost::default_dfs_visitor{


    boost::property_map< graph_t, vertex_name_t >::type nodeName;

    std::string* model;
    int level;

    std::string tabSpacing(int level){
	std::string returnString = "";

	for(int i = 0; i < level; i++){
	    returnString += "\t";
	}

	return returnString;
    }

    public:

	custom_dfs_visitor(graph_t& g, std::string* netModel){
	    nodeName = get( vertex_name, g);
	    model = netModel;

	    level = 0;
	}

	void discover_vertex(Vertex u, const graph_t& g){
	    level++;
	    std::cout << "Visiting node " << nodeName[u] << std::endl;
	    *model += tabSpacing(level);
	    *model += "call_node_" + nodeName[u] + "();\n";
	}

	void tree_edge(Edge e, const graph_t& g){
	    *model += tabSpacing(level);
	    *model += "if(){\n";
	}
	
	void finish_edge(Edge e, const graph_t& g){
	    std::cout << "Pass here" << std::endl;
	    *model += tabSpacing(level);
	    *model += "}\n"; 
	    *model += "else {\n";
	}

	void finish_vertex(Vertex u, const graph_t& g){
	    *model += tabSpacing(level);
	    *model += "}\n";
	    level--;
	}	
};


std::string insertAssertionChecks( NetMap networkModelMap ){
    std::string returnString = "";

    returnString += "void assert_error(char* msg){\n";
    returnString += "\tprintf(\"%s\\n\", msg);\n";
    returnString += "\tklee_abort();\n";
    returnString += "}\n\n";

    returnString += "void end_assertions(){\n";
    
    for( auto logicExpr : networkModelMap.logicalExpressionList ){
	returnString += "\tif( !(" + logicExpr + ") ) assert_error(\"" + logicExpr + "\");\n";
    }

    returnString += "}\n";

    return returnString;
}


std::string insertRegisterInitializations( NetMap networkModelMap ){
    std::string returnString = "";

    returnString += "void initializeRegisters(){\n";
    returnString += networkModelMap.registerInitializations;
    returnString += "}\n\n";

    return returnString;
}


int main(int argc, char *const argv[]) {
    setup_gc_logging();

    AutoCompileContext autoBMV2Context(new BMV2::BMV2Context);
    auto& options = BMV2::BMV2Context::get().options();
    options.langVersion = BMV2::BMV2Options::FrontendVersion::P4_16;
    options.compilerVersion = "0.0.5";

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto hook = options.getDebugHook();

    if(options.staticEnforce){

        //P4BOX STATIC ENFORCEMENT BEGIN
        try {
            //Read topology information


            //Construct an empty graph and prepare the dynamic_property_maps
            graph_t graph(0);
            boost::dynamic_properties dp{ boost::ignore_other_properties };

            boost::property_map< graph_t, vertex_name_t >::type name = get( vertex_name, graph );
            dp.property( "node_id", name );

            boost::property_map< graph_t, vertex_program_t >::type p4program = get( vertex_program, graph );
            dp.property( "program", p4program );

	    boost::property_map< graph_t, vertex_commands_t >::type p4commands = get( vertex_commands, graph );
	    dp.property( "commands", p4commands );

	    boost::property_map< graph_t, vertex_arch_t >::type p4arch = get( vertex_arch, graph );
	    dp.property( "arch", p4arch );

	    boost::property_map< graph_t, edge_sport_t >::type src_ports = get( edge_sport, graph );
	    dp.property( "sport", src_ports );

	    boost::property_map< graph_t, edge_dport_t >::type dst_ports = get( edge_dport, graph );
	    dp.property( "dport", dst_ports );

            // Sample graph as an std::istream;
            //std::ifstream inputTopology("example-p4box-topology.txt");
	    std::ifstream inputTopology( options.topoFile );

            bool status = boost::read_graphviz( inputTopology, graph, dp, "node_id" );
            //End of topology reading

            //Iterate over vertices to read and store data plane programs and forwarding rules
            std::pair<vertex_iter, vertex_iter> vp;

	    //Store pieces of network-wide model
	    NetMap networkModelMap;
	    std::string netModel = "";

            for( vp = boost::vertices(graph); vp.first != vp.second; ++vp.first ){
                Vertex v = *vp.first;
                //TODO: remove debug code
		std::cout << name[v] << std::endl;
		networkModelMap.currentNodeName = name[v];
		
		//Forward declare walk functions
		networkModelMap.forwardDeclarations += "void walk_" + name[v] + "();\n";

                std::cout << p4program[v] << std::endl;
		//std::cout << "p4commands: " << p4commands[v] << std::endl;

                options.preprocessor_options += " -D__TARGET_BMV2__";
                options.file = p4program[v];
		options.commandsFile = p4commands[v];
		options.archModel = p4arch[v];
                auto program = P4::parseP4File(options);

                if (program == nullptr || ::errorCount() > 0)
                    return 1;

                try {
                    P4::P4COptionPragmaParser optionsPragmaParser;
                    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

                    P4::FrontEnd frontend;
                    frontend.addDebugHook(hook);
                    program = frontend.extractModel(options, program, networkModelMap);
		    //netModel += networkModelMap.nodeModels[name[v]];
		    
		    /*if(networkModelMap.headersOn){
			std::cout << "Headers successfuly inserted" << std::endl;
                    }
		    else{
			std::cout << "Values not updated" << std::endl;
		    }*/

                } catch (const Util::P4CExceptionBase &bug) {
                    std::cerr << bug.what() << std::endl;
                    return 1;
                }
            } //End of vertice iteration
        
            //std::cout << num_vertices(graph) << std::endl;

	    //Graph traversal
	    //TODO: set start node
	    Vertex v;

	    for( vp = boost::vertices(graph); vp.first != vp.second; ++vp.first ){
		v = *vp.first;
		//std::cout << "Node name: " << name[v] << std::endl;
		if( name[v] == options.ingressNode ){
		    //std::cout << "Set start node" << std::endl;
		    break;
		}
	    }

	    //custom_dfs_visitor vis(graph, &netModel);
	    //boost::depth_first_search( graph, boost::visitor(vis).root_vertex(v) );

	    //Insert preamble
	    netModel += "#include <stdio.h>\n"; 
	    netModel += "#include <stdint.h>\n";
    	    netModel += "#include \"klee/klee.h\"\n\n";

	    //Insert libraries describing architecture models
	    for( auto arch : networkModelMap.archTypes ){
		netModel += "#include \"" + arch + ".h\"\n";
	    }

	    netModel += "\n";

	    //Insert assertion variables
	    netModel += networkModelMap.globalDeclarations + "\n";

	    //Forward declare walk functions
	    netModel += networkModelMap.forwardDeclarations + "\n";

	    //Insert node models
	    std::map<std::string, std::string>::iterator nodeIter;

	    for (nodeIter = networkModelMap.nodeModels.begin(); nodeIter != networkModelMap.nodeModels.end(); ++nodeIter){
		netModel += nodeIter->second + "\n"; 
	    }
	    netModel += "\n";

	    custom_bfs_visitor vis(graph, &netModel, networkModelMap);
	    boost::breadth_first_search( graph, v, boost::visitor(vis) );

	    //Input symbolization
	    std::map<std::string, std::string>::iterator metaIter;
	    std::map<std::string, std::string>::iterator stdMetaIter;

	    netModel += insertRegisterInitializations( networkModelMap );

	    netModel += "void symbolizeInputs(){\n";

	    if( networkModelMap.p4boxState != "" ){
		netModel += "\tklee_make_symbolic(&" + networkModelMap.p4boxState + ", sizeof(" + 
			    networkModelMap.p4boxState + "), \"" + networkModelMap.p4boxState + "\");\n";
	    }

	    netModel += "\tklee_make_symbolic(&" + networkModelMap.headers + ", sizeof(" + 
			    networkModelMap.headers + "), \"" + networkModelMap.headers + "\");\n";

	    for (metaIter = networkModelMap.metaName.begin(); metaIter != networkModelMap.metaName.end(); ++metaIter){
		netModel += "\tklee_make_symbolic(&" + metaIter->second + ", sizeof(" + metaIter->second + "), \"" + metaIter->second + "\");\n";
	    }

	    for (stdMetaIter = networkModelMap.stdMeta.begin(); stdMetaIter != networkModelMap.stdMeta.end(); ++stdMetaIter){
		netModel += "\tklee_make_symbolic(&" + stdMetaIter->second + ", sizeof(" + stdMetaIter->second + "), \"" + stdMetaIter->second + "\");\n";
	    }

	    netModel += "}\n\n";

	    //Model assertion checks
	    netModel += insertAssertionChecks( networkModelMap ) + "\n";
	    
	    //Model main function
	    //TODO: set start port
	    netModel += "int main(){\n";
	    netModel += "\tinitializeRegisters();\n";
	    netModel += "\tsymbolizeInputs();\n\n";
	    netModel += "\t" + networkModelMap.stdMeta[name[v]] + ".ingress_port = " + options.ingressPort + ";\n";
	    netModel += "\twalk_" + name[v] + "();\n";
	    netModel += "\tend_assertions();\n";
	    netModel += "\treturn 0;\n";
	    netModel += "}\n";

	    std::cout << "------- Net model --------" << std::endl;
	    //std::cout << netModel << std::endl;

std::cout << "Using Boost "     
          << BOOST_VERSION / 100000     << "."  // major version
          << BOOST_VERSION / 100 % 1000 << "."  // minor version
          << BOOST_VERSION % 100                // patch level
          << std::endl;

	    std::string outFileName = "netwide-model.c";
    	    std::ofstream outFile;
    	    outFile.open(outFileName);
    	    outFile << netModel;
    	    outFile.close();

        } catch (const Util::P4CExceptionBase &bug) {
            std::cerr << bug.what() << std::endl;
            return 1;
        }

        return ::errorCount() > 0;
        //P4BOX STATIC ENFORCEMENT END

    } else {

    // BMV2 is required for compatibility with the previous compiler.
    options.preprocessor_options += " -D__TARGET_BMV2__";
    auto program = P4::parseP4File(options);
    if (program == nullptr || ::errorCount() > 0)
        return 1;
    try {
        P4::P4COptionPragmaParser optionsPragmaParser;
        program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

        P4::FrontEnd frontend;
        frontend.addDebugHook(hook);

        //P4BOX BEGIN
        if(options.emitMonitoredP4){
            frontend.emitMonitoredP4(options, program);  
            return 0;
        }
        else{
            program = frontend.run(options, program);
        }
        //P4BOX END

    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    const IR::ToplevelBlock* toplevel = nullptr;
    BMV2::MidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    try {
        toplevel = midEnd.process(program);
        if (::errorCount() > 1 || toplevel == nullptr ||
            toplevel->getMain() == nullptr)
            return 1;
        if (options.dumpJsonFile)
            JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0)
        return 1;

    // backend depends on the modified refMap and typeMap from midEnd.
    BMV2::Backend backend(options.isv1(), &midEnd.refMap,
            &midEnd.typeMap, &midEnd.enumMap);
    try {
        backend.addDebugHook(hook);
        backend.process(toplevel, options);
        backend.convert(options);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0)
        return 1;

    if (!options.outputFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.outputFile, false);
        if (out != nullptr) {
            backend.serialize(*out);
            out->flush();
        }
    }

    P4::serializeP4RuntimeIfRequired(program, options);

    return ::errorCount() > 0;

    }//Enf of check whether static enforcement is active
}
