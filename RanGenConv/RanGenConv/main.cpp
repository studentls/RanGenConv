//
//  main.cpp
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 27.11.14.
//  Copyright (c) 2014 Leonhard Spiegelberg. All rights reserved.
//

#include <iostream>

// inc for easy commandline parsing
#include <getopt.h>

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
int main(int argc, char * argv[]) {
    
    using namespace std;
    
    bool verbose = false; // indicate if program is in verbose mode or not
    
    program_name = argv[0]; // program name is stored as first argument
    
    int next_option = 0;
    do  {
        
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch(next_option) {
                case 'h':
                print_usage(stdout, 0);
                break;
                
                case 'c':
                cout<<"file to check is "<<optarg<<endl;
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
    
    //option opt = NULL;
    
    // print other arguments
    for (int i = optind; i < argc; ++i) {
        cout<<"argument: "<<argv[i]<<endl;
    }
    
    return 0;
}
