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
#include <boost/graph/graphviz.hpp>
#include <boost/graph/adjacency_list.hpp>


using namespace boost;

namespace boost {
    enum vertex_program_t { vertex_program };
    enum vertex_commands_t { vertex_commands };

    BOOST_INSTALL_PROPERTY( vertex, program );
    BOOST_INSTALL_PROPERTY( vertex, commands );
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

            //Vertex properties
            typedef boost::property< vertex_program_t, std::string > DataPlaneProgram;
	    typedef boost::property< vertex_commands_t, std::string, DataPlaneProgram > ForwardingRules;
            typedef boost::property< vertex_name_t, std::string, ForwardingRules > vertex_p;

            //Graph properties
            typedef boost::property< graph_name_t, std::string > graph_p;

            //Adjacency list containing the graph that represents the network topology
            typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS,
                                       vertex_p, no_property, graph_p > graph_t;

            //Construct an empty graph and prepare the dynamic_property_maps
            graph_t graph(0);
            boost::dynamic_properties dp{ boost::ignore_other_properties };

            boost::property_map< graph_t, vertex_name_t >::type name = get( vertex_name, graph );
            dp.property( "node_id", name );

            boost::property_map< graph_t, vertex_program_t >::type p4program = get( vertex_program, graph );
            dp.property( "program", p4program );

	    boost::property_map< graph_t, vertex_commands_t >::type p4commands = get( vertex_commands, graph );
	    dp.property( "commands", p4commands );

            // Sample graph as an std::istream;
            //std::istringstream gvgraph("digraph { graph [name=\"graphname\"]  a  c e g h }");
            std::ifstream inputTopology("example-p4box-topology.txt");

            bool status = boost::read_graphviz( inputTopology, graph, dp, "node_id" );
            //End of topology reading

            //Iterate over vertices to read and store data plane programs and forwarding rules
            typedef boost::graph_traits< graph_t >::vertex_descriptor Vertex;
            typedef boost::graph_traits< graph_t >::vertex_iterator vertex_iter;
            std::pair<vertex_iter, vertex_iter> vp;

            for( vp = boost::vertices(graph); vp.first != vp.second; ++vp.first ){
                Vertex v = *vp.first;
                //TODO: remove debug code
                std::cout << p4program[v] << std::endl;
		//std::cout << "p4commands: " << p4commands[v] << std::endl;

                options.preprocessor_options += " -D__TARGET_BMV2__";
                options.file = p4program[v];
		options.commandsFile = p4commands[v];
                auto program = P4::parseP4File(options);

                if (program == nullptr || ::errorCount() > 0)
                    return 1;

                try {
                    P4::P4COptionPragmaParser optionsPragmaParser;
                    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));

                    P4::FrontEnd frontend;
                    frontend.addDebugHook(hook);
                    program = frontend.extractModel(options, program);
                } catch (const Util::P4CExceptionBase &bug) {
                    std::cerr << bug.what() << std::endl;
                    return 1;
                }
            } //End of vertice iteration
        
            //std::cout << num_vertices(graph) << std::endl;

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
