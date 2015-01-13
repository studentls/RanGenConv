//
//  RanGenFile.cpp
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 12.01.15.
//  Copyright (c) 2015 Leonhard Spiegelberg. All rights reserved.
//

#include "RanGenFile.h"
#include <iostream>

RanGenFile::RanGenFile(std::string filename) {
    _bad = !parse_file(filename); // invert as parse_file returns true for success!
}

RanGenFile::~RanGenFile() {
    
}


// function to parse a rangenfile
bool RanGenFile::parse_file(std::string filename) {
    using namespace std;
    
    bool res = true;
    node dline;
    unsigned int num_nodes = 0;
    num_resources = 0;
    vector<node> nodes;
    
    ifstream ifs;
    
    int line_number = 0;
    ifs.open(filename);
    
    int id = 1; // give lines ids
    
    if(ifs.fail() || ifs.bad()) {
        cout<<"error: "<<"file could not been opened successfully"<<endl;
        return false;
    }
    for (string line; getline(ifs, line); )
    {
        // go over empty lines
        if(line.length() < 2)continue;
        
        //cout << line << endl;
        if(!dline.resource_requirements.empty())dline.resource_requirements.clear();
        if(!dline.children.empty())dline.children.clear();
        
        
        // zero line
        if(line_number == 0) {
            stringstream ss(line);
            ss >> num_nodes >> num_resources;
        }
        // second line
        else if(line_number == 1) {
            
            stringstream ss(line);
            int availability = 0;
            
            for(int i = 0; i < num_resources; ++i) {
                ss >> availability;
                _resource_availability.push_back(availability);
            }
        }
        else {
            
            stringstream ss(line);
            ss >> dline.activity_duration;
            int res = 0;
            for(int i = 0; i < num_resources; i++) {
                ss >> res;
                dline.resource_requirements.push_back(res);
            }
            ss >> dline.num_successors;
            int successor = 0;
            for(int j = 0; j < dline.num_successors; j++) {
                ss >> successor;
                dline.children.push_back(successor);
            }
            dline.id = id++; //
            
            nodes.push_back(dline);
        }
        
        line_number++;
        
        //check if something went wrong...
        if(ifs.fail() || ifs.bad()) {
            cout<<"error: "<<"bad file operation occured"<<endl;
            return false;
        }
    }
    
    
    // assign all relations
    if(!(res = build_adjmatrix(nodes)))cout<<"error while building adjacency matrix for graph"<<endl;
    
    // now check if graph is really a DAG!
    if (G.isCyclic()) {
        cout << "we have a huge problem! The graph is not a DAG!!!" << endl;
        exit(1);
    }
    else cout << "graph is a DAG, all fine" << endl;
    
    // now copy data contens to graph
    assert(!nodes.empty());
    assert(nodes.size() == node_count());
    
    int index = 0;
    for(std::vector<node>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        G.set_vertex(index++, *it);
    }
    return true;
}

// build adjacency matrix
bool RanGenFile::build_adjmatrix(const std::vector<node>& V) {
    
    bool res = true;
    
    // reserve space
    if(!(res = G.init((unsigned int)V.size())))std::cout<<"error initiating graph structure"<<std::endl;
       
       // go through nodes and add everything relevant
       if(!V.empty())
       for (std::vector<node>::const_iterator it = V.begin(); it != V.end(); ++it) {
           int i = it->id - 1; // conv to c++ index
           int j = -1;
           if(!it->children.empty())
               for (std::vector<int>::const_iterator jt = it->children.begin(); jt != it->children.end(); ++jt) {
                   j = *jt - 1; // conv to c++ index
                   G.set(i, j, true);
               }
       }
       return res;
       }
       
// new advanced algorithm
bool RanGenFile::generate_times(const int limit = 10) {
   
    using namespace std;
    stack<int> Stack;
    
    // perform firsthand topological sort
    G.topologicalSort(Stack);
    
   while (!Stack.empty()) {
       int j = Stack.top();
       Stack.pop();
       
       // for the generation three constraints have to be fulfilled
       
       // Let I be the set of all predecessor of node j
       // i.e. precedences <i,j> hold for all i € I
       
       // d_x ... deadline of node x
       // r_x ... release time of node x
       // p_x ... duration of node x
       
       //  (1) d_i - r_i >= p_i    forall i € I
       //      d_j - r_j >= p_j
       //  (2) r_j - r_i >= p_i    forall i € I
       //  (3) d_j - d_i >= p_j    forall i € I
       
       // algorithm:
       // step1:   d_max = max d_i over I
       //          r_max = max r_i over I
       //          p_max = max p_i over I
       // step2:   W ~ Geo(l1) Z ~ Geo(l2) or other discrete distribution, but W, Z >= 0 must hold true
       //          X := p_max + W
       //          Y := X + Z
       // step3:
       //          r_j := r_max + X
       //          d_j := p_j + d_max + Y
       
       int d_max = 0; // -inf
       int r_max = 0;
       int p_max = 0;
       
       // go through all parents
       for (int i = 0; i < node_count(); ++i) {
           // check if set
           if (G.get(i, j)) {
               // i is parent of j!
               node & parent = G.v(i);
               
               d_max = max(d_max, parent.deadline);
               r_max = max(r_max, parent.release);
               p_max = max(p_max, parent.activity_duration);
           }
       }
       
       static double l1 = 0.6;
       static double l2 = 0.4;
       
       int W = georv(l1);
       int Z = georv(l2);
       
       // only allowed values, limit W, Z to avoid exploding the time horizon
       W = min(W, limit - rand() % (limit / 2)); // add some dynamic to limiting!
       Z = min(Z, limit - rand() % (limit / 2));
       
       // special case, first dummy node will have everything set to zero!!!
       if (j == 0) {
           W = Z = 0;
       }
       
       if (d_max < r_max) {
           cout << "error: logical flaw found!!! d_max < r_max" << endl;
       }
       
       int X = p_max + W;
       int Y = X + Z;
       
       G.v(j).release = r_max + X;
       G.v(j).deadline = G.v(j).activity_duration + d_max + Y;
       
       
       if (Y < X) {
           cout << "something is wrong here" << endl;
       }
#ifdef DEBUG
       assert(p_max >= 0);
       assert(r_max >= 0);
       assert(d_max >= 0);
       assert(X >= p_max);
       assert(Y >= X);
       assert(G.v(j).deadline - G.v(j).release >= G.v(j).activity_duration);
#endif
   }
   
    return true;
}

// validate generated times
// returns true if no error occured
bool RanGenFile::validate_times() {
    using namespace std;
    
    bool res = true;
    
    for(int j = 0; j < node_count(); j++) {
        node& root = G.v(j);
        
        double rmaxprogress = 1.0 / root.activity_duration + 0.000001;
        int rduration = 1.0 / rmaxprogress;
        
        
        if (root.deadline - root.release < root.activity_duration) {
            cout << "violation found: d_" << root.id << " - r_" << root.id << " < p_" << root.id << endl;
        }
        
        if (root.deadline - root.release < rduration) {
            cout << "rounding violation found: d_" << root.id << " - r_" << root.id << " < p_" << root.id << endl;
        }
        
        // go through parents
        for (int i = 0; i < node_count(); ++i) {
            
            if (G.get(i, j)) {
                node & parent = G.v(i);
                if (!(parent.deadline - parent.release >= parent.activity_duration  // d_i - r_i >= p_i
                      && root.release - parent.release >= parent.activity_duration // r_j - r_i >= p_i
                      && root.deadline - parent.deadline >= root.activity_duration // d_j - d_i >= p_j
                      )) {
                    res = false;
                    cout << "violation found: <" << i + 1 << "," << j + 1 << ">" << endl;
                }
                
                double pmaxprogress = 1.0 / parent.activity_duration + 0.000001;
                int pduration = 1.0 / pmaxprogress;
                
                if (!(parent.deadline - parent.release >= pduration  // d_i - r_i >= p_i
                      && root.release - parent.release >= pduration // r_j - r_i >= p_i
                      && root.deadline - parent.deadline >= rduration // d_j - d_i >= p_j
                      )) {
                    res = false;
                    cout << "rounding violation found: <" << i + 1<< "," << j + 1<< ">" << endl;
                }
            }
            
        }
        
    }
    
    return res;
}

// function in order to validate if given input follows RanGen format
// returns false if problems occured
bool RanGenFile::validate_file(const bool verbose) {
    using namespace std;
    
    bool err = false;
    // next perform checks
    if(resource_availability().size() != resource_count()) {
        if(verbose)cout<<"inconsistency found: resource availability does not match number of resources"<<endl;
        err = true;
    }
    
    // check for all lines if numbers make sense (i.e. all info is there)
    int l = 1;
    for(int i = 0; i < node_count(); i++) {
        if(nodes(i).num_successors != nodes(i).children.size()) {
            cout <<"line #"<<l<<": inconsistency found"<<endl;
            err = true;
        }
        
        ++l;
    }
    
    return !err;
}
