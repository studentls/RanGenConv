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
#include <getopt.h>
#include <fstream>
#include <sstream>
#include <string>

// make life easier
using namespace std;

// struct to hold one data line of the patterson format
struct data_line {
    int activity_duration;
    vector<int> resource_requirements;
    int num_successors;
    vector<int> successor_ids;
};

// struct to hold data of one file
struct rangen_file {
    int num_nodes;                  // including two dummy nodes for start & end
    int num_resources;              // number of renewable resources
    vector<int> resource_availability;  // vector containing availabilitys
    // of the num_resources resources
    vector<data_line> data_lines;       // vector of all data entries
};

const char *program_name;

// program short options
const char * const short_options = "hc:v";
// the programs options
const struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"check-input", 1, NULL, 'c'},
    {"verbose", 0, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

// print usage function including detailed help for all opts
void print_usage(FILE * stream, int exit_code) {
    
    fprintf(stream, "usage: %s options [intputfile] [outputfile]", program_name);
    fprintf(stream,
            "   -h --help                   display help message\n"
            "   -c --check-input filename   check if a given input file obeys the RanGen format\n"
            "   -v --verbose                perform in verbose mode");
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
                
                if(!file->data_lines.empty())
                    if(file->data_lines.size() == file->num_nodes - 2)
                        continue;
                
                
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
            if(line_number == 2)continue;
            
            //check if something went wrong...
            if(ifs.fail() || ifs.bad()) {
                cout<<"error: "<<"bad file operation occured"<<endl;
                if(file)delete file;
                return NULL;
            }
        }


    
    
    return file;
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
    
    bool verbose = false; // indicate if program is in verbose mode or not
    
    program_name = argv[0]; // program name is stored as first argument
    int mode = 0;
    char *file_to_check = NULL;
    
    int next_option = 0;
    do  {
        
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch(next_option) {
                case 'h':
                print_usage(stdout, 0);
                break;
                
                case 'c':
                file_to_check = optarg;
                mode |= MODE_CHECK;
                break;
                
                case 'v':
                verbose = true;
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
    
    
    if(mode & MODE_CHECK) {
        assert(file_to_check);
        if(validate_input(verbose, file_to_check))
            cout<<"file ok"<<endl;
        else
            cout<<"file bad"<<endl;
    }
    
    
    return 0;
}
