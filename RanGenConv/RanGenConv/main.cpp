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
#include <stack>

#ifndef DEBUG
#ifdef _DEBUG
#define DEBUG
#endif
#endif
// inc for easy commandline parsing
#ifdef WIN32

#undef _UNICODE
#undef UNICODE
#include "getopt_win.h"

template<typename T> T max(const T& a, const T& b) {
    return a > b ? a : b;
}

template<typename T> T min(const T& a, const T& b) {
	return a < b ? a : b;
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

struct edge {
    int i; // parent
    int j; // child
};

// struct to hold one data line of the patterson format
class node {
public:
    int id; // min, max are the dummynodes --> starts with 1
    int activity_duration;
    vector<int> resource_requirements;
    int num_successors;
    vector<int> children; // starts with 1
	vector<int> parents; // starts with 1
    int release;
    int deadline;
    
    node() : activity_duration(0), num_successors(0), release(0), deadline(0), id(-1) {
        
    }
};


class Adjacencymatrix {
private:
	int num_nodes;
	unsigned char *adj;

	void clear() {
		if (adj)delete[] adj;
		adj = NULL;
		num_nodes = 0;
	}
public:
	Adjacencymatrix() : num_nodes(0), adj(NULL) {

	}
	~Adjacencymatrix() {
		clear();
	}

	void set(const unsigned int i, const unsigned int j, const unsigned char val) {
		assert(adj);
		assert(0 <= i && 0 <= j && i < num_nodes && j < num_nodes);
		adj[i + j * num_nodes] = val;
	}

	unsigned char get(const int i, const int j) {
		assert(adj);
		return adj[i + j * num_nodes];
	}

	void create(const int _num_nodes) {
		if (adj)clear();
		num_nodes = _num_nodes;
		adj = new unsigned char[num_nodes * num_nodes];
		for (int i = 0; i < num_nodes * num_nodes; i++)adj[i] = 0;
	}

	int node_count() { return num_nodes; }
};

// helper function to check if graph contains cycles
bool isCyclicUtil(Adjacencymatrix& m, int v, bool *visited, bool *recStack) {
	assert(visited);
	assert(recStack);

	if (!visited[v]) {
		//mark current node as visited, push on stack
		visited[v] = true;
		recStack[v] = true;

		for (int i = 0; i < m.node_count(); i++) {
			
			// skip non-adjacent nodes
			if (!m.get(v, i))continue;

			// check for all adjacent nodes, if they will lead to a back edge!
			if (!visited[i] && isCyclicUtil(m, i, visited, recStack))
				return true;
			else if (recStack[i])
				return true;
		}
	}
	recStack[v] = false; // remove from stack
	return false;
}

// check if graph is cyclic for given matrix
bool isCyclic(Adjacencymatrix& m) {
	bool res = false;
	bool *visited = new bool[m.node_count()];
	bool *recStack = new bool[m.node_count()];

	for (int i = 0; i < m.node_count(); i++) {
		visited[i] = recStack[i] = false;
	}

	// call helper function to perform recursive dfs search
	for (int i = 0; i < m.node_count(); i++) {
		if (isCyclicUtil(m, i, visited, recStack)) {
			res = true;
			break;
		}
	}
	
	delete[] visited;
	delete[] recStack;

	return res;
}

void topolocialSortUtil(Adjacencymatrix & m, int v, bool *visited, stack<int>& Stack) {
	// mark current node as visited
	visited[v] = true;

	// go through all adjacent vertices
	for (int i = 0; i < m.node_count(); i++) {
		if (!m.get(v, i))continue; // skip non-adjacent vertices

		if (!visited[i])
			topolocialSortUtil(m, i, visited, Stack);
	}

	// push vertex to stack to store result
	Stack.push(v);
}
void topologicalSort(Adjacencymatrix& m, stack<int>& Stack) {

	bool *visited = new bool[m.node_count()];
	for (int i = 0; i < m.node_count(); i++) {
		visited[i] = false;
	}

	// call helper to store topological sort on stack
	for (int i = 0; i < m.node_count(); i++)
		if (!visited[i])
			topolocialSortUtil(m, i, visited, Stack);
}

// struct to hold data of one file
struct rangen_file {
    int num_nodes;                  // including two dummy nodes for start & end
    int num_resources;              // number of renewable resources
    vector<int> resource_availability;  // vector containing availabilitys
    // of the num_resources resources
    vector<node> nodes;       // vector of all data entries
	Adjacencymatrix m; // adjacencymatrix corresponding to nodes
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
	{ "timelimit", 1, NULL, 't' },
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
			"   -t --timelimit  value       limit X, Y range. Higher values lead to more flexibility but a higher time horizon");
    exit(exit_code);
}

// build adjacency matrix
void build_adjmatrix(rangen_file *file) {

	// reserve space
	file->m.create(file->nodes.size());

	// go through nodes and add everything relevant
	if(!file->nodes.empty())
		for (vector<node>::const_iterator it = file->nodes.begin(); it != file->nodes.end(); ++it) {
		int i = it->id - 1; // conv to c++ index
		int j = -1;
		if(!it->children.empty())
			for (vector<int>::const_iterator jt = it->children.begin(); jt != it->children.end(); ++jt) {
			j = *jt - 1; // conv to c++ index
			file->m.set(i, j, true);
		}
	}
}

// function to parse a rangenfile
rangen_file* parserg_file(const char *filename) {
    rangen_file *file = new rangen_file;
    node dline;
    
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
            if(!dline.children.empty())dline.children.clear();
            
            
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
                
                /*if(!file->nodes.empty())
                    if(file->nodes.size() == file->num_nodes - 2)
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
                    dline.children.push_back(successor);
                }
                dline.id = id++; //
                
                file->nodes.push_back(dline);
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

    
    // assign all relations
		build_adjmatrix(file);
    
		// now check if graph is really a DAG!
		if (isCyclic(file->m)) {
			cout << "we have a huge problem! The graph is not a DAG!!!" << endl;
			exit(1);
		}
		else cout << "graph is a DAG, all fine" << endl;

		// now perform the topological sort!
		//topologicalSort(file->m);


		for (int i = 0; i < file->num_nodes; i++) {
			cout << "node " << i << " children:  ";
			for (int j = 0; j < file->num_nodes; j++) {
				if (file->m.get(i, j)) {
					cout << "(" << j << ")  ";
				}
			}
			cout << endl;
		}

		for (int i = 0; i < file->num_nodes; i++) {
			cout << "node " << i << " parents:  ";
			for (int j = 0; j < file->num_nodes; j++) {
				if (file->m.get(j, i)) {
					cout << "(" << j << ")  ";
				}
			}
			cout << endl;
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
	// dont forget max, as random generator can return 0
	  return ::max((int)floor(log(drandom(0.0, 1.0)) / log(1.0 - rate)), 0);
}

int vector_maxi(const vector<int>& v) {
    int _max = -999999999; // -inf
    
    for(vector<int>::const_iterator it = v.begin(); it != v.end(); ++it) {
        _max = ::max(_max, *it);
    }
    
    return _max;
}

vector<int> g_visits;

// new advanced algorithm
void ngenerate_times(rangen_file *file, stack<int>& Stack, const int limit = 10) {

	// for better reading
	Adjacencymatrix& adj = file->m;
	vector<node>& v = file->nodes;

#ifdef DEBUG
	//g_visits.push_back(j);
#endif

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
		for (int i = 0; i < file->num_nodes; ++i) {
			// check if set
			if (adj.get(i, j)) {
				// i is parent of j!
				node & parent = v[i];

				d_max = ::max(d_max, parent.deadline);
				r_max = ::max(r_max, parent.release);
				p_max = ::max(p_max, parent.activity_duration);
			}
		}

		static double l1 = 0.6;
		static double l2 = 0.4;

		int W = georv(l1);
		int Z = georv(l2);

		// only allowed values, limit W, Z to avoid exploding the time horizon
		W = ::min(W, limit - rand() % (limit / 2)); // add some dynamic to limiting!
		Z = ::min(Z, limit - rand() % (limit / 2));

		// special case, first dummy node will have everything set to zero!!!
		if (j == 0) {
			W = Z = 0;
		}

		if (d_max < r_max) {
			cout << "error: logical flaw found!!! d_max < r_max" << endl;
		}

		int X = p_max + W;
		int Y = X + Z;

		v[j].release = r_max + X;
		v[j].deadline = v[j].activity_duration + d_max + Y;


		if (Y < X) {
			cout << "something is wrong here" << endl;
		}
#ifdef DEBUG
		assert(p_max >= 0);
		assert(r_max >= 0);
		assert(d_max >= 0);
		assert(X >= p_max);
		assert(Y >= X);
		assert(v[j].deadline - v[j].release >= v[j].activity_duration);
#endif
	}

}

// here release & deadlines are generated for the vector v
// v holds the data array
// l is the current node for which things shall be generated
// rest is data of predecessor
void generate_times(rangen_file *file, const int j, const int limit = 10) {

	// for better reading
	Adjacencymatrix& adj = file->m;
	vector<node>& v = file->nodes;

#ifdef DEBUG
	//g_visits.push_back(j);
#endif

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
	for (int i = 0; i < file->num_nodes; ++i) {
		// check if set
		if (adj.get(i, j)) {
			// i is parent of j!
			node & parent = v[i];

			d_max = ::max(d_max, parent.deadline);
			r_max = ::max(r_max, parent.release);
			p_max = ::max(p_max, parent.activity_duration);
		}
	}

    static double l1 = 0.6;
    static double l2 = 0.4;
    
    int W = georv(l1);
    int Z = georv(l2);
    
	// only allowed values, limit W, Z to avoid exploding the time horizon
	W = ::min(W, limit - rand() % (limit / 2)); // add some dynamic to limiting!
	Z = ::min(Z, limit - rand() % (limit / 2));

	// special case, first dummy node will have everything set to zero!!!
    if(j == 0) {
        W = Z = 0;
    }
    
	if (d_max < r_max) {
		cout << "error: logical flaw found!!! d_max < r_max" << endl;
	}

	int X = p_max + W;
	int Y = X + Z;

	v[j].release = r_max + X;
	v[j].deadline = v[j].activity_duration + d_max + Y;


	if (Y < X) {
		cout << "something is wrong here" << endl;
	}
#ifdef DEBUG
	assert(p_max >= 0);
	assert(r_max >= 0);
	assert(d_max >= 0);
	assert(X >= p_max);
	assert(Y >= X);
	assert(v[j].deadline - v[j].release >= v[j].activity_duration);
#endif


	for (int k = 0; k < file->num_nodes; ++k) {
		if (adj.get(j, k)) {
			// k is a child of j!
			generate_times(file, k, limit);
		}
	}

#ifdef DEBUG
	//if(g_visits.size() % 10 == 0)cout << "visited " << g_visits.size() <<" nodes till now..."<< endl;
#endif
}


// validate generated times
// returns true if no error occured
bool nvalidate_times(rangen_file *file) {

	bool res = true;

	for(int j = 0; j < file->m.node_count(); j++) {

		vector<node>& v = file->nodes;
		Adjacencymatrix& adj = file->m;
		node& root = v[j];

		double rmaxprogress = 1.0 / root.activity_duration + 0.000001;
		int rduration = 1.0 / rmaxprogress;


		if (root.deadline - root.release < root.activity_duration) {
			cout << "violation found: d_" << root.id << " - r_" << root.id << " < p_" << root.id << endl;
		}

		if (root.deadline - root.release < rduration) {
			cout << "rounding violation found: d_" << root.id << " - r_" << root.id << " < p_" << root.id << endl;
		}

		// go through parents
		for (int i = 0; i < v.size(); ++i) {

			if (adj.get(i, j)) {
				node & parent = v[i];

				// check all things
				edge e;
				e.i = parent.id; // i is parent
				e.j = root.id; // j is root
				//cout << "checking <" << i << "," << j << ">" << endl;
				if (!(parent.deadline - parent.release >= parent.activity_duration  // d_i - r_i >= p_i
					&& root.release - parent.release >= parent.activity_duration // r_j - r_i >= p_i
					&& root.deadline - parent.deadline >= root.activity_duration // d_j - d_i >= p_j
					)) {
					res = false;
					cout << "violation found: <" << e.i << "," << e.j << ">" << endl;
				}

				double pmaxprogress = 1.0 / parent.activity_duration + 0.000001;
				int pduration = 1.0 / pmaxprogress;

				if (!(parent.deadline - parent.release >= pduration  // d_i - r_i >= p_i
					&& root.release - parent.release >= pduration // r_j - r_i >= p_i
					&& root.deadline - parent.deadline >= rduration // d_j - d_i >= p_j
					)) {
					res = false;
					cout << "rounding violation found: <" << e.i << "," << e.j << ">" << endl;
				}
			}

		}

	}

	return res;
}


// validate generated times
// returns true if no error occured
bool validate_times(rangen_file *file, int j) {
    
    bool res = true;
    
	vector<node>& v = file->nodes;
	Adjacencymatrix& adj = file->m;
	node& root = v[j];

	double rmaxprogress = 1.0 / root.activity_duration + 0.000001;
	int rduration = 1.0 / rmaxprogress;


    if(root.deadline - root.release < root.activity_duration) {
        cout<<"violation found: d_"<<root.id<<" - r_"<<root.id<<" < p_"<<root.id<<endl;
    }
    
	if (root.deadline - root.release < rduration) {
		cout << "rounding violation found: d_" << root.id << " - r_" << root.id << " < p_" << root.id << endl;
	}

	// go through parents
	for (int i = 0; i < v.size(); ++i) {
		
		if (adj.get(i, j)) {
			node & parent = v[i];

			// check all things
			edge e;
			e.i = parent.id; // i is parent
			e.j = root.id; // j is root
			//cout << "checking <" << i << "," << j << ">" << endl;
			if (!(parent.deadline - parent.release >= parent.activity_duration  // d_i - r_i >= p_i
				&& root.release - parent.release >= parent.activity_duration // r_j - r_i >= p_i
				&& root.deadline - parent.deadline >= root.activity_duration // d_j - d_i >= p_j
				)) {
				res = false;
				cout << "violation found: <" << e.i << "," << e.j << ">" << endl;
			}

			double pmaxprogress = 1.0 / parent.activity_duration + 0.000001;
			int pduration = 1.0 / pmaxprogress;

			if (!(parent.deadline - parent.release >= pduration  // d_i - r_i >= p_i
				&& root.release - parent.release >= pduration // r_j - r_i >= p_i
				&& root.deadline - parent.deadline >= rduration // d_j - d_i >= p_j
				)) {
				res = false;
				cout << "rounding violation found: <" << e.i << "," << e.j << ">" << endl;
			}
		}
		
	}


	// go through all children
	for (int k = 0; k < v.size(); k++) {
		if (adj.get(j, k)) {
			if (!validate_times(file, k))res = false;
		}
	}

    return res;
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
    if(!file->nodes.empty()) {
        int nid = offset;
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
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
    if(!file->nodes.empty()) {
        int eid = offset;
        int nid = offset;
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
            
            if(!it->children.empty())
                for(vector<int>::const_iterator jt = it->children.begin(); jt != it->children.end(); ++jt) {
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


bool generate_output(const bool verbose, const char *ifilename, const char *ofilename, const int time_limit, const bool dummynodes = false, const bool graphml = false) {
    
    
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
    //srand((unsigned int)time(NULL));
	srand(0);

	if (verbose)cout << "generating times..." << endl;

	/*
	// old version
    if(!file->nodes.empty())
        generate_times(file, 0, time_limit); // call with trivial solution
#ifdef DEBUG
	cout << "visited " << g_visits.size() << endl;
#endif
	*/
	// new advanced algorithm
	stack<int> Stack;
	topologicalSort(file->m, Stack); // perform topological sort first
	// new method
	ngenerate_times(file, Stack, time_limit);

	if (verbose)cout << "times successfully generated!" << endl;
	if (verbose)cout << "validating graph..." << endl;
    // check for failure
   /* if(!validate_times(file, 0)) {
        cout<<"error: validation of graph failed!"<<endl;
        exit(1);
	}*/

	// validation
	if(!nvalidate_times(file)) {
	cout<<"error: validation of graph failed!"<<endl;
	exit(1);
	}

#pragma warning "here put something new!"
	else if (verbose)cout << "graph successfully validated!" << endl;
    
    // now get maxtime
     // set maxtime to ceil of latest deadline
    int imaxtime = 0;
    if(!file->nodes.empty())
        for(vector<node>::const_iterator it = file->nodes.begin();
            it != file->nodes.end(); it++)
        {
            // dmaxtime += it->deadline - it->release; // adding differences
            imaxtime = ::max(imaxtime, it->deadline);
        }
	maxtime = imaxtime;
   // maxtime = ::max(time_domain, imaxtime); // yield value to forced value if it makes sense
    
	if(verbose)cout << "time horizon ist " << maxtime << " periods long" << endl;
    
	assert(imaxtime > 0);

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
    if(!file->nodes.empty())
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
            ofs<<(1.0 / it->activity_duration + 0.000001); // change here for other max progress...
            if(it != file->nodes.end() - offset - 1)ofs<<",";
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
        if(!file->nodes.empty())
            if(!file->nodes[0].children.empty())
                for(vector<int>::const_iterator it = file->nodes[0].children.begin(); it != file->nodes.front().children.end(); ++it)
                    ofs<<"<1,"<<*it<<">,";
    }
    
    if(!file->nodes.empty()) {
        int curid = dummynodes ? 2 : 1;
        for(vector<node>::const_iterator it = file->nodes.begin() + 1; it != file->nodes.end(); ++it) {
            for(vector<int>::const_iterator jt = it->children.begin(); jt != it->children.end(); ++jt) {
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
    if(!file->nodes.empty())
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
            ofs<<it->release; // change here for other max progress...
            if(it != file->nodes.end() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    
    if(verbose)cout<<"release written..."<<endl;
    
    // deadline
    ofs<<"deadline  = [";
    if(!file->nodes.empty())
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
            ofs<<it->deadline; // change here for other max progress...
            if(it != file->nodes.end() - offset - 1)ofs<<",";
        }
    ofs<<"];"<<endl;
    
    if(verbose)cout<<"deadline written..."<<endl;
    
    // res_demand
    ofs<<"res_demand = [";
    if(!file->nodes.empty()) {
        for(vector<node>::const_iterator it = file->nodes.begin() + offset; it != file->nodes.end() - offset; ++it) {
            // go through all resources and print activity's demand out!
            ofs<<"[";
            if(!it->resource_requirements.empty())
                for(vector<int>::const_iterator jt = it->resource_requirements.begin(); jt != it->resource_requirements.end(); ++jt) {
                    ofs<<*jt;
                    if(jt != it->resource_requirements.end() - 1)ofs<<",";
                }
            ofs<<"]";
            if(it != file->nodes.end() - offset - 1)ofs<<",";
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
    for(vector<node>::const_iterator it = file->nodes.begin(); it != file->nodes.end(); ++it) {
        if(it->num_successors != it->children.size()) {
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
    
	int time_limit = 10; // value of 10 per default
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
        if(ifile && ofile)generate_output(verbose, ifile, ofile, time_limit, dummynodes, graphml);
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
