//
//  main.cpp
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 27.11.14.
//  Copyright (c) 2014 Leonhard Spiegelberg. All rights reserved.
//

template<typename T> T max(const T& a, const T& b) {
	return a > b ? a : b;
}


#include <iostream>
#include <cassert>
#include <vector>

// inc for easy commandline parsing
#ifdef WIN32
#undef _UNICODE
#undef UNICODE
#include "getopt_win.h"
#else
#include <getopt.h>
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>

// make life easier
using namespace std;

// struct to hold one data line of the patterson format
class data_line {
public:
    int activity_duration;
    vector<int> resource_requirements;
    int num_successors;
    vector<int> successor_ids;
    double release;
    double deadline;
    
    data_line() : activity_duration(0), num_successors(0), release(0.0), deadline(0.0) {
        
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
const char * const short_options = "hc:vd";
// the programs options
const struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"check-input", 1, NULL, 'c'},
    {"verbose", 0, NULL, 'v'},
    {"dummy", 0, NULL, 'd'},
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
            "   -d --dummy                  output dummy nodes at start and end");
    exit(exit_code);
}

// function to parse a rangenfile
rangen_file* parserg_file(const char *filename) {
    rangen_file *file = new rangen_file;
    data_line dline;
    
    ifstream ifs;
    
    int line_number = 0;
        ifs.open(filename);
    
    if(ifs.fail() || ifs.bad()) {
        cout<<"error: "<<"file could not been opened successfully"<<endl;
        if(file)delete file;
        return NULL;
    }
        for (string line; getline(ifs, line); )
        {
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

// here release & deadlines are generated for the vector v
void generate_times(vector<data_line>& v, data_line& l, double mintime) {
    
    double eps = 0.001; // add / sub this to avoid that rounding will ruin everything
    
    // end?
    if(l.num_successors == 0) {
        
    }
    else {
        // generate time for leaf
        l.release += mintime;
        l.deadline = l.release + l.activity_duration;
        
        // here now alter the times to give some random element
        
        // algorithm here is to simple use exponentially distributed times with rate = 1.0 / activity duration
        l.release -= exprv(1.0 / l.activity_duration) - eps;
        l.deadline += exprv(1.0 / l.activity_duration)+ eps;
        
        l.release = max(0.0, l.release); // clamp to 0
        
        assert(l.deadline - l.release >= l.activity_duration);
        
        // now proceed with successors and use as mintime mintime+duration
        if(!l.successor_ids.empty())
            for(vector<int>::const_iterator it = l.successor_ids.begin(); it != l.successor_ids.end(); ++it) {
                generate_times(v, v[(*it) - 1], mintime + l.activity_duration);
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

bool generate_output(const bool verbose, const char *ifilename, const char *ofilename, const bool dummynodes = false) {
    
    
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
        generate_times(file->data_lines, file->data_lines[0], 0.0);
    
    
    // now get maxtime
     // set maxtime to ceil of latest deadline
    double dmaxtime = 0.0;
    if(!file->data_lines.empty())
        for(vector<data_line>::const_iterator it = file->data_lines.begin();
            it != file->data_lines.end(); it++)
        {
            // dmaxtime += it->deadline - it->release; // adding differences
            dmaxtime = ::max(dmaxtime, it->deadline);
        }
    maxtime = (int)ceil(dmaxtime);
    
    
    
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
        for(int j = 0; j < file->resource_availability.size() - 1; j++) {
            ofs<<file->resource_availability[j]<<",";
        }
        ofs<<file->resource_availability[file->num_resources - 1]<<"]";
        // end print resource availability
        
        ofs<<",";
    }
    ofs<<"[";
    for(int j = 0; j < file->resource_availability.size() - 1; j++) {
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
            ofs<<(1.0 / it->activity_duration); // change here for other max progress...
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
                        if(curid != activity_count - 1)ofs<<",";
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
    
    program_name = argv[0];     // program name is stored as first argument
    int mode = 0;
    char *file_to_check = NULL;
    char *ifile = NULL;
    char *ofile = NULL;
    
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
        if(ifile && ofile)generate_output(verbose, ifile, ofile, dummynodes);
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
