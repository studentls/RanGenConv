//
//  main.cpp
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 27.11.14.
//  Copyright (c) 2014 Leonhard Spiegelberg. All rights reserved.
//

#include <iostream>
#include <cassert>
#include <vector>

// inc for easy commandline parsing
#ifdef WIN32

#undef _UNICODE
#undef UNICODE
#include "getopt_win.h"

template<typename T> T max(const T& a, const T& b) {
    return a > b ? a : b;
}

#else
#include <getopt.h>
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>

#include <cstring>

// make life easier
using namespace std;

// struct to hold one data line of the patterson format
class data_line {
public:
    int id; // min, max are the dummynodes
    int activity_duration;
    vector<int> resource_requirements;
    int num_successors;
    vector<int> successor_ids;
    int release;
    int deadline;
    
    data_line() : activity_duration(0), num_successors(0), release(0), deadline(0), id(-1) {
        
    }
};

// struct to hold data of one file
struct rangen_file {
    int num_nodes;                  // including two dummy nodes for start & end
    int num_resources;              // number of renewable resources
    vector<int> resource_availability;  // vector containing availabilitys
    // of the num_resources resources
    vector<data_line> data_lines;       // vector of all data entries
};

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
	{ "timedomain", 1, NULL, 't' },
    {NULL, 0, NULL, 0}
};

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

// print usage function including detailed help for all opts
void print_usage(FILE * stream, int exit_code) {
    
    fprintf(stream, "usage: %s options [intputfile] [outputfile]", program_name.substr(program_name.rfind(PATH_SEPARATOR) + 1).c_str());
    fprintf(stream,
            "   -h --help                   display help message\n"
            "   -c --check-input filename   check if a given input file obeys the RanGen format\n"
            "   -v --verbose                perform in verbose mode\n"
            "   -g --graphml                output additionally GraphML file\n"
            "   -d --dummy                  output dummy nodes at start and end\n"
			"   -t --timedomain  value      force time domain to value");
    exit(exit_code);
}

// function to parse a rangenfile
rangen_file* parserg_file(const char *filename) {
    rangen_file *file = new rangen_file;
    data_line dline;
    
    ifstream ifs;
    
    int line_number = 0;
        ifs.open(filename);
    
    int id = 1; // give lines ids
    
    if(ifs.fail() || ifs.bad()) {
        cout<<"error: "<<"file could not been opened successfully"<<endl;
        if(file)delete file;
        return NULL;
    }
        for (string line; getline(ifs, line); )
        {
            // go over empty lines
            if(line.length() < 2)continue;
            
            //cout << line << endl;
            if(!dline.resource_requirements.empty())dline.resource_requirements.clear();
            if(!dline.successor_ids.empty())dline.successor_ids.clear();
            
            
            // zero line
            if(line_number == 0) {
                stringstream ss(line);
                ss >> file->num_nodes >> file->num_resources;
            }
            // second line
            else if(line_number == 1) {
                
                stringstream ss(line);
                int availability = 0;
                
                for(int i = 0; i < file->num_resources; ++i) {
                    ss >> availability;
                    file->resource_availability.push_back(availability);
                }
            }
            else {
                
                /*if(!file->data_lines.empty())
                    if(file->data_lines.size() == file->num_nodes - 2)
                        continue;*/
                
                
                stringstream ss(line);
                ss >> dline.activity_duration;
                int res = 0;
                for(int i = 0; i < file->num_resources; i++) {
                    ss >> res;
                    dline.resource_requirements.push_back(res);
                }
                ss >> dline.num_successors;
                int successor = 0;
                for(int j = 0; j < dline.num_successors; j++) {
                    ss >> successor;
                    dline.successor_ids.push_back(successor);
                }
                dline.id = id++; //
                
                file->data_lines.push_back(dline);
            }
            
            line_number++;
            
            // ignore first and last line
            //if(line_number == 2)continue;
            
            //check if something went wrong...
            if(ifs.fail() || ifs.bad()) {
                cout<<"error: "<<"bad file operation occured"<<endl;
                if(file)delete file;
                return NULL;
            }
        }


    
    
    return file;
}

class activity_node {
public:
    vector<activity_node> children;
    activity_node *parent;
    double release;
    double deadline;
    
    activity_node ():release(0.0), deadline(0.0), parent(NULL) {
        
    }
};


// helper func to draw a uniform random variable
double drandom(const double dmin, const double dmax) {
    return dmin + (dmax - dmin) * (rand()%RAND_MAX) / (double)RAND_MAX;
}

// returns a draw of a exponential distributed random variable
double exprv(const double rate) {
    return -1.0 * log(1.0 - drandom(0.0, 1.0)) / rate;
}

// return geometric distributed random variable
int georv(const double rate) {
    return (int)floor(log(drandom(0.0, 1.0)) / log(1.0 - rate));
}

// here release & deadlines are generated for the vector v
// v holds the data array
// l is the current node for which things shall be generated
// rest is data of predecessor
void generate_times(vector<data_line>& v, data_line& l, const int pred_release, const int pred_deadline, const int pred_duration) {
    
    // for the generation three constraints have to be fulfilled
    
    // (1) deadline(node) - release(node) >= duration(node)
    // (2) release(succ) - release(pred) >= duration(pred)
    // (3) deadline(succ) - deadline(pred) >= duration(succ)
    
    // Algorithm:
    // step 1:  Choose X, Z >= 0 s.t.
    //          X+Z >= deadline(pred) - release(pred) - duration(pred)
    // step 2:  Set Y as
    //          Y = X + Z - (deadline(pred) - release(pred) - duration(pred))
    // step 3:  Calculate times by
    //          release(succ)  = release(pred)  + duration(pred) + X
    //          deadline(succ) = deadline(pred) + duration(succ) + Y
    // Note that: pred with all set to 0 trivially fulfills (1) - (3) ==> start with trivial solution at first place!
    
    int X = 0, Z = 0;
    int maxallowed = 4000;
    int cnt = 0;
    
    int window = pred_deadline - pred_release - pred_duration; // makewindow
    
    // generate matching values but limit to meaningful max time
    do {
        
        X = georv(1.0 / (double)l.activity_duration); // this can be arbitrary...
        Z = georv(1.0 / (double)(pred_duration + l.activity_duration)); // this can be arbitrary...
        
        // special case dummy nodes
        if(l.activity_duration == 0)X = 0;
        if(pred_duration == 0)Z = 0;
        
        cnt++;
    } while(X + Z < window && cnt < maxallowed);
    
    int Y = X + Z - window;
    
    // correct if maxallowed was passed
    if(cnt == maxallowed)Y += window;
    
    l.release = pred_release + pred_duration + X;
    l.deadline = pred_deadline + l.activity_duration + Y;
    
    // assert (1) - (3)
    assert(pred_release + pred_duration <= pred_deadline); // (1) for predecessor
    assert(l.release + l.activity_duration <= l.deadline); //(1)
    assert(pred_release + pred_duration <= l.release); // (2)
    assert(pred_deadline + l.activity_duration <= l.deadline); //(3)
    
    // do a BFS!
    if(!l.successor_ids.empty() && l.num_successors != 0) {
        for(vector<int>::const_iterator it = l.successor_ids.begin(); it != l.successor_ids.end(); ++it) {
            generate_times(v, v[*it - 1], l.release, l.deadline, l.activity_duration);
        }
    }
}

// helper func to check if file exists
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

// helper func to check if file can be written
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

bool generate_graphml(const bool verbose, rangen_file *file, const char *ofilename, const bool dummynodes = false) {
    
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
    for(unsigned int i = 1; i <= file->resource_availability.size(); ++i) {
        unsigned int index = i + 3;
        ofs<<"<key id=\"d"<<index<<"\" for=\"node\" attr.name=\"res"<<i<<"_demand\" attr.type=\"int\">"<<endl;
        ofs<<"<default>0</default>"<<endl;
        ofs<<"</key>"<<endl;
    }
    
    //begin with graph
    ofs<<"<graph id=\"G\" edgedefault=\"directed\">"<<endl;
    
    // first, print nodes
    if(!file->data_lines.empty()) {
        int nid = offset;
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            ofs<<"<node id=\"n"<<nid<<"\">"<<endl;
            ofs<<"<data key=\"d0\">"<<it->activity_duration<<"</data>"<<endl; //d0 = activity duration
            ofs<<"<data key=\"d1\">"<<it->release<<"</data>"<<endl; //d1 = release
            ofs<<"<data key=\"d2\">"<<it->deadline<<"</data>"<<endl; //d2 = deadline
            ofs<<"<data key=\"d3\">"<<(it->deadline - it->release)<<"</data>"<<endl; //d3 = window
            
            // res_demands
            if(!it->resource_requirements.empty()) {
                int index = 4;
                for(vector<int>::const_iterator jt = it->resource_requirements.begin(); jt != it->resource_requirements.end(); ++jt) {
                    ofs<<"<data key=\"d"<<index<<"\">"<<*jt<<"</data>"<<endl;
                    index++;
                }
            }
            ofs<<"</node>"<<endl;
            nid++;
        }
    }
    
    if(verbose)cout<<"nodes written..."<<endl;
    
    // progress with edges
    if(!file->data_lines.empty()) {
        int eid = offset;
        int nid = offset;
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            
            if(!it->successor_ids.empty())
                for(vector<int>::const_iterator jt = it->successor_ids.begin(); jt != it->successor_ids.end(); ++jt) {
                    if(!dummynodes && (*jt == 0 || *jt == file->num_nodes))
                        continue; //dummynodes disabled, skip them!
                    ofs<<"<edge id=\"e"<<eid<<"\" source=\"n"<<(nid)<<"\" target=\"n"<<(*jt)-1<<"\" />"<<endl;
                    eid++;
                }
            nid++;
        }
    }
    
    if(verbose)cout<<"edges written..."<<endl;
    
    //print footer
    ofs<<"</graph>"<<endl<<"</graphml>"<<endl;
    
    if(verbose)cout<<"GraphML file successfully written!"<<endl;
    
    return true;
}


bool generate_output(const bool verbose, const char *ifilename, const char *ofilename, const int time_domain, const bool dummynodes = false, const bool graphml = false) {
    
    
    if(verbose)cout<<">>> get input >>>"<<endl;
    
    rangen_file *file = parserg_file(ifilename);
    
    if(!file) {
        cout<<"error while parsing "<<ifilename<<endl;
        return false;
    }
    
    if(verbose)cout<<"parsed input file..."<<endl<<"<<< write output <<<"<<endl;
    
    int activity_count = dummynodes ? file->num_nodes : file->num_nodes - 2;
    
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
    srand((unsigned int)time(NULL));
    if(!file->data_lines.empty())
        generate_times(file->data_lines, file->data_lines[0], 0, 0, 0); // call with trivial solution
    
    
    // now get maxtime
     // set maxtime to ceil of latest deadline
    int imaxtime = 0;
    if(!file->data_lines.empty())
        for(vector<data_line>::const_iterator it = file->data_lines.begin();
            it != file->data_lines.end(); it++)
        {
            // dmaxtime += it->deadline - it->release; // adding differences
            imaxtime = ::max(imaxtime, it->deadline);
        }
    maxtime = ::max(time_domain, imaxtime); // yield value to forced value if it makes sense
    
    
    
    ofstream ofs(ofilename);
    
    if(ofs.bad() || ofs.fail()) {
        cout<<"error: output file could not been opened"<<endl;
        exit(1);
    }
    //stringstream ofs;
    
    // time
    ofs<<"time = {";
    for(int i = 1; i < maxtime; i++)ofs<<i<<",";
    ofs<<maxtime<<"};"<<endl;
    
    if(verbose)cout<<"time written..."<<endl;
    
    // activity
    ofs<<"activity = {";
    for(int i = 1; i < activity_count; i++)ofs<<i<<",";
    ofs<<activity_count<<"};"<<endl;
    
    if(verbose)cout<<"activity written..."<<endl;
    
    // resource
    ofs<<"resource = {";
    for(int i = 1; i < file->num_resources; i++)ofs<<i<<",";
    ofs<<file->num_resources<<"};"<<endl;
    
    if(verbose)cout<<"resource written..."<<endl;
    
    // (overall) resource capacity (constant)
    ofs<<"res_capacity = [";
    for(int i = 1; i < maxtime; i++) {
        
        // print resource availability (is here constant) at time t
        ofs<<"[";
        for(unsigned int j = 0; j < file->resource_availability.size() - 1; j++) {
            ofs<<file->resource_availability[j]<<",";
        }
        ofs<<file->resource_availability[file->num_resources - 1]<<"]";
        // end print resource availability
        
        ofs<<",";
    }
    ofs<<"[";
    for(unsigned int j = 0; j < file->resource_availability.size() - 1; j++) {
        ofs<<file->resource_availability[j]<<",";
    }
    ofs<<file->resource_availability[file->num_resources - 1]<<"]];"<<endl;
    
    if(verbose)cout<<"res_capacity written..."<<endl;
    
    // max progress
    // max progress is 1.0 / activity duration. Can be also something arbitrarily
    // i.e. draw max progress as uniform random variable out of interval [0.2 1]
    ofs<<"maxProgress  = [";
    if(!file->data_lines.empty())
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            ofs<<(1.0 / it->activity_duration + 0.000001); // change here for other max progress...
            if(it != file->data_lines.end() - offset - 1)ofs<<",";
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
        if(!file->data_lines.empty())
            if(!file->data_lines[0].successor_ids.empty())
                for(vector<int>::const_iterator it = file->data_lines[0].successor_ids.begin(); it != file->data_lines.front().successor_ids.end(); ++it)
                    ofs<<"<1,"<<*it<<">,";
    }
    
    if(!file->data_lines.empty()) {
        int curid = dummynodes ? 2 : 1;
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + 1; it != file->data_lines.end(); ++it) {
            for(vector<int>::const_iterator jt = it->successor_ids.begin(); jt != it->successor_ids.end(); ++jt) {
                if(!dummynodes) {
                    if(*jt != file->num_nodes)  { // print only if not last node
                        ofs<<"<"<<curid<<","<<(*jt) - 1<<">";
                        if(curid != activity_count - 1
                           && (it)->successor_ids[0] != activity_count - 1)ofs<<",";
                    }
                    
                    
                }
                else {
                  ofs<<"<"<<curid<<","<<*jt<<">";
                  if(curid != activity_count - 1)ofs<<",";
                }
            }
            curid++;
        }
    }
    ofs<<"};"<<endl;
    
    if(verbose)cout<<"Relations written..."<<endl;
    
    // release
    ofs<<"release  = [";
    if(!file->data_lines.empty())
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            ofs<<it->release; // change here for other max progress...
            if(it != file->data_lines.end() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    
    if(verbose)cout<<"release written..."<<endl;
    
    // deadline
    ofs<<"deadline  = [";
    if(!file->data_lines.empty())
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            ofs<<it->deadline; // change here for other max progress...
            if(it != file->data_lines.end() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    
    if(verbose)cout<<"deadline written..."<<endl;
    
    // res_demand
    ofs<<"res_demand = [";
    if(!file->data_lines.empty()) {
        for(vector<data_line>::const_iterator it = file->data_lines.begin() + offset; it != file->data_lines.end() - offset; ++it) {
            // go through all resources and print activity's demand out!
            ofs<<"[";
            if(!it->resource_requirements.empty())
                for(vector<int>::const_iterator jt = it->resource_requirements.begin(); jt != it->resource_requirements.end(); ++jt) {
                    ofs<<*jt;
                    if(jt != it->resource_requirements.end() - 1)ofs<<",";
                }
            ofs<<"]";
            if(it != file->data_lines.end() - offset - 1)ofs<<",";
        }
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

// function in order to validate if given input follows RanGen format
// returns false if problems occured
bool validate_input(bool verbose, const char *filename) {
    
    // parse file
    rangen_file *file = parserg_file(filename);
    
    if(!file) {
        cout<<"error while parsing "<<filename<<endl;
        return false;
    }
    
    bool err = false;
    // next perform checks
    if(file->resource_availability.size() != file->num_resources) {
        if(verbose)cout<<"inconsistency found: resource availability does not match number of resources"<<endl;
        err = true;
    }
    
    // check for all lines if numbers make sense (i.e. all info is there)
    int l = 1;
    for(vector<data_line>::const_iterator it = file->data_lines.begin(); it != file->data_lines.end(); ++it) {
        if(it->num_successors != it->successor_ids.size()) {
            cout <<"line #"<<l<<": inconsistency found"<<endl;
            err = true;
        }
        
        ++l;
    }
   
    return !err;
}

#define MODE_CHECK 0x2
#define MODE_REGULAR 0x4

int main(int argc, char * argv[]) {
    
    bool verbose = false;       // indicate if program is in verbose mode or not
    bool dummynodes = false;    // indicate if dummynodes shall be added to output
    bool graphml = false;       // indicate if additional graphml file should be generated
    
    program_name = argv[0];     // program name is stored as first argument
    int mode = 0;
    char *file_to_check = NULL;
    char *ifile = NULL;
    char *ofile = NULL;
    
	int time_domain = -1; // value of -1 indicates nothing forced...
    int options_used = 1; // one for program name
    int next_option = 0;
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
				time_domain = atoi(optarg); // use better c++11 for string conversion in a later deployment
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
    }while(next_option != -1);
    
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
        if(ifile && ofile)generate_output(verbose, ifile, ofile, time_domain, dummynodes, graphml);
    }
    
    if(mode & MODE_CHECK) {
        assert(file_to_check);
        if(validate_input(verbose, file_to_check))
            cout<<"file ok"<<endl;
        else
            cout<<"file bad"<<endl;
    }

    return 0;
}
