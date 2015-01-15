//
//  main.cpp
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 27.11.14.
//  Copyright (c) 2014 Leonhard Spiegelberg. All rights reserved.
//

#include "RanGenFile.h"
#include "Graph.h"
#include "helper.h"

#include <iostream>
#include <cassert>
#include <vector>
#include <stack>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>

#include <cstring>

#define MODE_CHECK 0x2
#define MODE_REGULAR 0x4

// make life easier
using namespace std;

string program_name;

// program short options
const char * const short_options = "hc:vgdt:";
// the programs options
const struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"check-input", 1, NULL, 'c'},
    {"verbose", 0, NULL, 'v'},
    {"graphml", 0, NULL, 'g'},
    {"dummy", 0, NULL, 'd'},
	{ "timelimit", 1, NULL, 't' },
    {NULL, 0, NULL, 0}
};

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

/**
 * @brief diplays help message
 * @details displays all short and long options of the converter. Note that the syntax to use the converter requires to specify first all options and two files (input, output) except if the check mode is used (-c file)
 * 
 * @param stream where to output the usage 
 * @param exit_code exit code with which the program shall finished after usage has been printed
 * 
 */
// print usage function including detailed help for all opts
void print_usage(FILE * stream, int exit_code) {
    
    fprintf(stream, "usage: %s options [intputfile] [outputfile]", program_name.substr(program_name.rfind(PATH_SEPARATOR) + 1).c_str());
    fprintf(stream,
            "   -h --help                   display help message\n"
            "   -c --check-input filename   check if a given input file obeys the RanGen format\n"
            "   -v --verbose                perform in verbose mode\n"
            "   -g --graphml                output additionally GraphML file\n"
            "   -d --dummy                  output dummy nodes at start and end\n"
			"   -t --timelimit  value       limit X, Y range. Higher values lead to more flexibility but a higher time horizon");
    exit(exit_code);
}


// 
/**
 * @brief helper func to check if file exists
 * @details checks if file exists by using C++ STL fstream functions
 * 
 * @param name path to file to check for
 * @return true if file exists, false otherwise
 */
inline bool exists_file (const std::string& name) {
    ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }
}

/**
 * @brief helper func to check if file can be written
 * @details checks if program is exceuted with permission to write to disk. 
 * 
 * @param name path to check for writing permission
 * @return true if converter could write to given path (name), false otherwise
 */
inline bool writable_file (const std::string& name) {
    ofstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }
}

/**
 * @brief generates for given RanGenFile graphml output
 * @details generates for given RanGenFile graphml output for use i.e. in GePhi. GraphML output contains node and edge struture labeling nodes/edges n0, n1, .../ e0, e2, ... . Note that labels start only with 0 iff dummynodes are selected. Furthermore, duration, release, deadline and time between deadline and release are written to the GraphML file.
 * 
 * 
 * @param verbose set to true if messages shall be displayed
 * @param file reference to RanGenFile for which contents shall be written to GraphML
 * @param ofilename path to file to write to
 * @param dummynodes set to true to output dummynodes at start and end of graph (default false)
 * @return returns true if no errors occured
 */
bool generate_graphml(const bool verbose, RanGenFile& file, const char *ofilename, const bool dummynodes = false) {
    
    int offset = dummynodes ? 0 : 1;
    
    if(verbose)cout<<"writing GraphML file..."<<endl;
    // open output stream
    ofstream ofs(ofilename);
    
    if(ofs.bad() || ofs.fail()) {
        cout<<"error: output file could not been opened"<<endl;
        exit(1);
    }
    
    //print header
    ofs<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl<<
    "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"" \
    " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" \
    " xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns" \
    "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"<<endl;
    
    // print attribute definition (for activity duration, release, deadline, window[deadline - release])
    // actually, window is redundant info, but included for convenience reasons...
    ofs<<"<key id=\"d0\" for=\"node\" attr.name=\"activity_duration\" attr.type=\"int\">"<<endl;
    ofs<<"<default>0</default>"<<endl;
    ofs<<"</key>"<<endl;
    ofs<<"<key id=\"d1\" for=\"node\" attr.name=\"release\" attr.type=\"int\">"<<endl;
    ofs<<"<default>0</default>"<<endl;
    ofs<<"</key>"<<endl;
    ofs<<"<key id=\"d2\" for=\"node\" attr.name=\"deadline\" attr.type=\"int\">"<<endl;
    ofs<<"<default>0</default>"<<endl;
    ofs<<"</key>"<<endl;
    ofs<<"<key id=\"d3\" for=\"node\" attr.name=\"window\" attr.type=\"int\">"<<endl;
    ofs<<"<default>0</default>"<<endl;
    ofs<<"</key>"<<endl;
    
    // after 4 initial data keys, include also resource demands of each node
    for(unsigned int i = 1; i <= file.resource_availability().size(); ++i) {
        unsigned int index = i + 3;
        ofs<<"<key id=\"d"<<index<<"\" for=\"node\" attr.name=\"res"<<i<<"_demand\" attr.type=\"int\">"<<endl;
        ofs<<"<default>0</default>"<<endl;
        ofs<<"</key>"<<endl;
    }
    
    //begin with graph
    ofs<<"<graph id=\"G\" edgedefault=\"directed\">"<<endl;
    
    // first, print nodes
        int nid = offset;
        for(int i = offset; i < file.node_count() - offset; i++) {
            ofs<<"<node id=\"n"<<nid<<"\">"<<endl;
            ofs<<"<data key=\"d0\">"<<file.nodes(i).activity_duration<<"</data>"<<endl; //d0 = activity duration
            ofs<<"<data key=\"d1\">"<<file.nodes(i).release<<"</data>"<<endl; //d1 = release
            ofs<<"<data key=\"d2\">"<<file.nodes(i).deadline<<"</data>"<<endl; //d2 = deadline
            ofs<<"<data key=\"d3\">"<<(file.nodes(i).deadline - file.nodes(i).release)<<"</data>"<<endl; //d3 = window
            
            // res_demands
            if(!file.nodes(i).resource_requirements.empty()) {
                int index = 4;
                for(vector<int>::const_iterator jt = file.nodes(i).resource_requirements.begin(); jt != file.nodes(i).resource_requirements.end(); ++jt) {
                    ofs<<"<data key=\"d"<<index<<"\">"<<*jt<<"</data>"<<endl;
                    index++;
                }
            }
            ofs<<"</node>"<<endl;
            nid++;
        }
    
    if(verbose)cout<<"nodes written..."<<endl;
    
    // progress with edges
        int eid = offset;
        nid = offset;
        for(int i = offset; i < file.node_count() - offset; ++i) {
                for(int j = 0; j < file.node_count(); ++j) {
                    if(!dummynodes && (j == 0 || j == file.node_count() - 1))
                        continue; //dummynodes disabled, skip them!
                    if(file.get(i, j))ofs<<"<edge id=\"e"<<eid<<"\" source=\"n"<<(nid)<<"\" target=\"n"<<j<<"\" />"<<endl;
                    eid++;
                }
            nid++;
        }
    
    if(verbose)cout<<"edges written..."<<endl;
    
    //print footer
    ofs<<"</graph>"<<endl<<"</graphml>"<<endl;
    
    if(verbose)cout<<"GraphML file successfully written!"<<endl;
    
    return true;
}

/**
 * @brief converts Patterson format to format as used in the formulation after Kis et al., Alfiere et al.
 * @details parses file in Patterson format first, generates then additional times and outputs then data in the structure used by the Kis / Alfieri models' implementations and if desired an additional GraphML file to investigate the network structure. Performs furthermore automatic checks whether given input is a DAG(directed acyclic graph).
 * 
 * @param verbose set to true to display additional messages
 * @param ifilename path to input file
 * @param ofilename path to output file
 * @param time_limit controls the maximum deviation release and deadlines can have. Higher values lead to greater time horizon. Default is 10. 
 * @param dummynodes set to true to ouput dummy nodes at start and end
 * @param graphml set to true to output additional graphml file to ofilename.graphml
 * @return true if no errors occured
 */
bool generate_output(const bool verbose, const char *ifilename, const char *ofilename, const int time_limit, const bool dummynodes = false, const bool graphml = false) {
    
    
    if(verbose)cout<<">>> get input >>>"<<endl;
    
    RanGenFile file(ifilename);
    
    if(file.bad()) {
        cout<<"error while parsing "<<ifilename<<endl;
        return false;
    }
    
    if(verbose)cout<<"parsed input file..."<<endl<<"<<< write output <<<"<<endl;
    
    int activity_count = dummynodes ? file.node_count() : file.node_count() - 2;
    
    int offset = dummynodes ? 0 : 1; // used for iterating vectors
    // file layout is
//    time = {1,2,3,4,5,6,7};
//    activity = {1,2,3};
//    resource = {1,2};
//    res_capacity = [[4,4],[4,4],[4,4],[4,4],[4,4],[4,4],[4,4]];
//    maxProgress  = [0.5,0.5,0.5];
//    minProgress  = [0,0,0];
//    Relations = {<1,2>, <2,3>};
//    release = [0,0,0];
//    deadline = [4,5,7];
//    res_demand = [[0,1],[1,1],[2,2]];
    
    int maxtime = 0; // fill with maximum time to generate ascending time values
    
    // first of all generate for all nodes release and deadlines.
    // it must hold:
    // deadline - release >= activity_duration
    // deadline, release >= 0
	//srand(0);

	if (verbose)cout << "generating times..." << endl;

    file.generate_times(time_limit);
    
	if (verbose)cout << "times successfully generated!" << endl;
	if (verbose)cout << "validating graph..." << endl;
    
    // check for failure of time generation procedure
	if(!file.validate_times()) {
	cout<<"error: validation of graph failed!"<<endl;
	exit(1);
	}
	else if (verbose)cout << "graph successfully validated!" << endl;
    
    // now get maxtime
     // set maxtime to ceil of latest deadline
    int imaxtime = 0;
    for(int i = 0; i < file.node_count(); i++)
        {
            // dmaxtime += it->deadline - it->release; // adding differences
            imaxtime = ::max(imaxtime, file.nodes(i).deadline);
        }
	maxtime = imaxtime;
    
	if(verbose)cout << "time horizon ist " << maxtime << " periods long" << endl;
	assert(imaxtime > 0);

    // open output file
    ofstream ofs(ofilename);
    
    if(ofs.bad() || ofs.fail()) {
        cout<<"error: output file could not been opened"<<endl;
        exit(1);
    }

    // time
    ofs<<"time = {";
    for(int i = offset; i < maxtime; i++)ofs<<i<<",";
    ofs<<maxtime<<"};"<<endl;
    if(verbose)cout<<"time written..."<<endl;
    
    // activity
    ofs<<"activity = {";
    for(int i = 1; i < activity_count; i++)ofs<<i<<",";
    ofs<<activity_count<<"};"<<endl;
    if(verbose)cout<<"activity written..."<<endl;
    
    // resource
    ofs<<"resource = {";
    for(int i = 1; i < file.resource_count(); i++)ofs<<i<<",";
    ofs<<file.resource_count()<<"};"<<endl;
    if(verbose)cout<<"resource written..."<<endl;
    
    // (overall) resource capacity (constant)
    ofs<<"res_capacity = [";
    for(int i = 1; i < maxtime; i++) {
        
        // print resource availability (is here constant) at time t
        ofs<<"[";
        for(unsigned int j = 0; j < file.resource_availability().size() - 1; j++) {
            ofs<<file.resource_availability()[j]<<",";
        }
        ofs<<file.resource_availability()[file.resource_count() - 1]<<"]";
        // end print resource availability
        ofs<<",";
    }
    ofs<<"[";
    for(unsigned int j = 0; j < file.resource_availability().size() - 1; j++) {
        ofs<<file.resource_availability()[j]<<",";
    }
    ofs<<file.resource_availability()[file.resource_count() - 1]<<"]];"<<endl;
    if(verbose)cout<<"res_capacity written..."<<endl;
    
    // max progress
    // max progress is 1.0 / activity duration. Can be also something arbitrarily
    // i.e. draw max progress as uniform random variable out of interval [0.2 1]
    ofs<<"maxProgress  = [";
    for(int i = offset; i < file.node_count() - offset; i++) {
            ofs<<(1.0 / file.nodes(i).activity_duration + 0.000001); // change here for other max progress...
        
            if(i != file.node_count() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    if(verbose)cout<<"maxProgress written..."<<endl;
    
    // min progress
    float min_progress = 0.0; // constant
    ofs<<"minProgress  = [";
    for(int i = 1; i < activity_count; i++)ofs<<min_progress<<",";
    ofs<<min_progress<<"];"<<endl;
    if(verbose)cout<<"minProgress written..."<<endl;
    
    // print relations
    ofs<<"Relations  = {";
    // print dummy node if desired
    if(dummynodes) {
        for(int j = 0; j < file.node_count(); j++)
                    if(file.get(0, j))ofs<<"<1,"<<(j + 1)<<">,";
    }
    int curid = dummynodes ? 2 : 1;
    for(int i =  1; i < file.node_count(); i++) {
        for(int j = 0; j < file.node_count(); j++) {
            if(!dummynodes) {
                if(j != file.node_count() - 1)  { // print only if not last node
                    if(file.get(i, j)) {
                        ofs<<"<"<<curid<<","<<j + 1<<">";
                    if(curid != activity_count - 1)ofs<<",";
                    }
                }
                
                
            }
            else {
                if(file.get(i, j)) {
                    
                ofs<<"<"<<curid<<","<<j + 1<<">";
              if(curid != activity_count - 1)ofs<<",";
                }
            }
        }
        curid++;
    }
    ofs<<"};"<<endl;
    if(verbose)cout<<"Relations written..."<<endl;
    
    // release
    ofs<<"release  = [";
        for(int i = offset; i < file.node_count() - offset; ++i) {
            ofs<<file.nodes(i).release; // change here for other max progress...
            if(i != file.node_count() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    if(verbose)cout<<"release written..."<<endl;
    
    // deadline
    ofs<<"deadline  = [";
    for(int i = offset; i < file.node_count() - offset; ++i) {
        ofs<<file.nodes(i).deadline; // change here for other max progress...
        if(i != file.node_count() - offset - 1)ofs<<",";
    }
    ofs<<"];"<<endl;
    if(verbose)cout<<"deadline written..."<<endl;
    
    // res_demand
    ofs<<"res_demand = [";
    for(int i = offset; i < file.node_count() - offset; ++i) {
        // go through all resources and print activity's demand out!
        ofs<<"[";
        if(!file.nodes(i).resource_requirements.empty())
            for(vector<int>::const_iterator jt = file.nodes(i).resource_requirements.begin(); jt != file.nodes(i).resource_requirements.end(); ++jt) {
                ofs<<*jt;
                if(jt != file.nodes(i).resource_requirements.end() - 1)ofs<<",";
            }
        ofs<<"]";
        if(i != file.node_count() - offset - 1)ofs<<",";
    }
    ofs<<"];"<<endl;
    if(verbose)cout<<"res_demand written..."<<endl;
    if(verbose)cout<<"file successfully converted!"<<endl;
    
    // write graphml file if desired...
    if(graphml) {
        string gmlfilename = string(ofilename) + ".graphml";
       generate_graphml(verbose, file, gmlfilename.c_str(), dummynodes);
    }
    
    return true;
}

/**
 * @brief main function
 * @details contains main loop
 * 
 * @param argc number of arguments passed via commandline
 * @param argv arguments passed to commandline. Note that argv[0] contains the name of the executable
 * 
 * @return 0 if no errors occured.
 */
int main(int argc, char * argv[]) {
    
    bool verbose = false;       // indicate if program is in verbose mode or not
    bool dummynodes = false;    // indicate if dummynodes shall be added to output
    bool graphml = false;       // indicate if additional graphml file should be generated
    
    program_name = argv[0];     // program name is stored as first argument
    int mode = 0;
    char *file_to_check = NULL;
    char *ifile = NULL;
    char *ofile = NULL;
    
	int time_limit = 10; // value of 10 per default
	int options_used = 1; // one for program name
    int next_option = 0;
    
    
    srand((unsigned int)time(NULL));
    
    
    do  {
        
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch(next_option) {
                case 'h':
                print_usage(stdout, 0);
                options_used++;
                break;
                
                case 'c':
                file_to_check = optarg;
                mode |= MODE_CHECK;
                options_used += 2;
                break;
                
                case 'v':
                verbose = true;
                options_used++;
                break;
                
                case 'd':
                dummynodes = true;
                options_used++;
                break;
                
                case 'g':
                graphml = true;
                options_used++;
                break;
                
				case 't':
				time_limit = atoi(optarg); // use better c++11 for string conversion in a later deployment
				options_used += 2;
				break;

                case '?':
                // user specified invalid options, terminate with exit code 1
                print_usage(stderr, 1);
                break;
                
            case -1: // no more options left
                break;
                
            default:
                // something else happened
                
                break;
        }
    } while(next_option != -1);
    
    // check if there is enough arguments left for input / output files
    if(argc - options_used == 1) {
        cout<<"error: outputfile not specified"<<endl;
        exit(1);
    }
    if(argc - options_used == 2) {
        mode |= MODE_REGULAR;
    }
    else if(argc - options_used > 2) {
        cout<<"error: too many files listed"<<endl;
        exit(1);
    }
    
    if(mode & MODE_REGULAR) {
        // the last two arguments represent the input / output files
        ifile = argv[argc - 2];
        ofile = argv[argc - 1];
        
        // secure that each of those two are valid files by checking if they exist
        if(!exists_file(ifile)) {
            cout<<"error: input file not found / cannot be opened"<<endl;
            exit(1);
        }
        if(!writable_file(ofile)) {
            cout<<"error: output file could not be written to disc"<<endl;
            exit(1);
        }
        
        // now perform output
        if(ifile && ofile)generate_output(verbose, ifile, ofile, time_limit, dummynodes, graphml);
    }
    
    if(mode & MODE_CHECK) {
        assert(file_to_check);
        RanGenFile file(file_to_check);
        if(file.validate_file(verbose))
            cout<<"file ok"<<endl;
        else
            cout<<"file bad"<<endl;
    }

    return 0;
}
