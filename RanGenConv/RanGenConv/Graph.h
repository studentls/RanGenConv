//
//  Graph.h
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 12.01.15.
//  Copyright (c) 2015 Leonhard Spiegelberg. All rights reserved.
//

#ifndef __RanGenConv__Graph__
#define __RanGenConv__Graph__

#include "helper.h"

#include <stack>
#include <vector>
#include <cassert>
#include <stdio.h>

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

class Edge {
public:
    union {
        struct {
        int i, j;
        };
        struct {
            int predecessor, successor;
        };
    };
    
    Edge():i(0), j(0) {}
};

template<typename T> class Graph {
    T *_vertices; // stores all information regarding nodes
    unsigned int _vertex_count; // number of vertices
    //std::vector<Edge> _adjList; // adjacency list
    Adjacencymatrix _m; // adjacency matrix
    
    bool validIndex(int i) {
        return 0 <= i && i < _vertex_count;
    }
    
    // helper function to check if graph contains cycles
    bool isCyclicUtil(int v, bool *visited, bool *recStack) {
        assert(visited);
        assert(recStack);
        
        if (!visited[v]) {
            //mark current node as visited, push on stack
            visited[v] = true;
            recStack[v] = true;
            
            for (int i = 0; i < _m.node_count(); i++) {
                
                // skip non-adjacent nodes
                if (!_m.get(v, i))continue;
                
                // check for all adjacent nodes, if they will lead to a back edge!
                if (!visited[i] && isCyclicUtil(i, visited, recStack))
                    return true;
                else if (recStack[i])
                    return true;
            }
        }
        recStack[v] = false; // remove from stack
        return false;
    }
    
    // helper for topological sorting
    void topolocialSortUtil(const int v, bool *visited, std::stack<int>& Stack) {
        // mark current node as visited
        visited[v] = true;
        
        // go through all adjacent vertices
        for (int i = 0; i < vertex_count(); i++) {
            if (!get(v, i))continue; // skip non-adjacent vertices
            
            if (!visited[i])
                topolocialSortUtil(i, visited, Stack);
        }
        
        // push vertex to stack to store result
        Stack.push(v);
    }
public:
    Graph():_vertices(NULL), _vertex_count(0) {}
    
    ~Graph() {
        if(_vertices) delete [] _vertices;
        _vertices = NULL;
        _vertex_count = 0;
    }
    
    bool init(const unsigned int vertex_count) {
        if(_vertices) delete [] _vertices;
        _vertex_count = vertex_count;
        _vertices = new T[_vertex_count];
        
        _m.create(_vertex_count);
        
        return true;
        
    }
    
    // check if graph is cyclic for given matrix
    bool isCyclic() {
        bool res = false;
        bool *visited = new bool[_m.node_count()];
        bool *recStack = new bool[_m.node_count()];
        
        for (int i = 0; i < _m.node_count(); i++) {
            visited[i] = recStack[i] = false;
        }
        
        // call helper function to perform recursive dfs search
        for (int i = 0; i < _m.node_count(); i++) {
            if (isCyclicUtil(i, visited, recStack)) {
                res = true;
                break;
            }
        }
        
        delete[] visited;
        delete[] recStack;
        
        return res;
    }
    
    // performs a topological sort and outputs result on a stack
    void topologicalSort(std::stack<int>& Stack) {
        
        bool *visited = new bool[vertex_count()];
        for (int i = 0; i < vertex_count(); i++) {
            visited[i] = false;
        }
        
        // call helper to store topological sort on stack
        for (int i = 0; i < vertex_count(); i++)
            if (!visited[i])
                topolocialSortUtil(i, visited, Stack);
    }
    
    //
    // helper functions
    //
    // get vertex
    T& v(const int v) {
        assert(0 <= v && v < _vertex_count);
        return _vertices[v];
    }
    // set vertex
    void set_vertex(const int v, const T& val) {
        assert(_vertices);
        assert(0 <= v && v < _vertex_count);
        _vertices[v] = val;
    }
    
    // set edge
    void set(const int i, const int j, const unsigned char val) {
        assert(validIndex(i) && validIndex(j));
        _m.set(i, j, val);
    }
    // get edge
    unsigned char get(const int i, const int j) {
        assert(validIndex(i) && validIndex(j));
        return _m.get(i, j);
    }
    unsigned int vertex_count() {return _vertex_count;}
    
};


#endif /* defined(__RanGenConv__Graph__) */
