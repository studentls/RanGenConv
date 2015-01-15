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

/**
 * @brief stores graph structure as adjacency matrix
 * @details stores graph structure as adjacency matrix based on unsigned chars. Values > 0 indicate that edge exists.
 * 
 */
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
    
    /**
     * @brief sets entry in adjacency matrix to val. Positive values of val indicate edge <i, j> is part of edge set.
     * 
     * @param i predecessor
     * @param j successor
     * @param val 0 indicates <i, j> not part of edge set, values != 0 part of edge set
     */
    void set(const unsigned int i, const unsigned int j, const unsigned char val) {
        assert(adj);
        assert(0 <= i && 0 <= j && i < num_nodes && j < num_nodes);
        adj[i + j * num_nodes] = val;
    }
    
    /**
     * @brief returns positive values if edge <i, j> is contained in edge set
     * 
     * @param i predecessor
     * @param j successor
     * 
     * @return positive values if edge <i, h> is part of edge set
     */
    unsigned char get(const int i, const int j) {
        assert(adj);
        return adj[i + j * num_nodes];
    }
    
    /**
     * @brief reserves space for the adjacency matrix
     * 
     * @param _num_nodes number of nodes to reserve space for
     */
    void create(const int _num_nodes) {
        if (adj)clear();
        num_nodes = _num_nodes;
        adj = new unsigned char[num_nodes * num_nodes];
        for (int i = 0; i < num_nodes * num_nodes; i++)adj[i] = 0;
    }
    
    /**
     * @return number of nodes of the representation (not the by edge structure actual implicitly contained ones of the graph)
     */
    int node_count() { return num_nodes; }
};

/**
 * @brief handles storage of graph structure and assigned data to nodes
 * @details uses currently an adjacencymatrix for representation of edge structure, an adjancency list might be an alternative. Supports only data assigned to vertices.
 * @tparam T type of the data assigned to the individual nodes 
 */
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
    
    /**
     * @brief reserves space to store vertex_count vertices with data
     * @details reserves space to store vertex_count vertices with data. If called more than one time for the same Graph object, existing data will be deleted and the object be reset
     * 
     * @param int [description]
     * @return [description]
     */
    bool init(const unsigned int vertex_count) {
        if(_vertices) delete [] _vertices;
        _vertex_count = vertex_count;
        _vertices = new T[_vertex_count];
        
        _m.create(_vertex_count);
        
        return true;
        
    }
    
    /**
     * @brief checks if graph is cyclic
     * @details checks if graph is cyclic by performing a DFS (depth first search). For a DAG (directed acyclic graph) this function should return always false.
     * @return true if cycle was found, false otherwise.
     */
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
    
    /**
     * @brief performs a topological sort and outputs result on a stack
     * @details performs a topological sort and outputs result on a stack using a DFS (depth first search).
     * 
     * @param Stack C++ STL stack to write order of topological sort. I.e. first/top entry equals the first node which shall be visited.
     */
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

    /**
     * @brief returns refernece to node data for vertex #v
     * @details returns refernece to node data for vertex #v. Note that this function provides read/write access! Be careful using this reference.
     * 
     * @param v index of node to return data for
     * @return node data of vertex #v
     */
    T& v(const int v) {
        assert(0 <= v && v < _vertex_count);
        return _vertices[v];
    }

    /**
     * @brief sets node data for vertex #v
     * @details sets node data for vertex #v.
     * 
     * @param v index of node to set data for
     * @param val node data to set for node #v
     */
    void set_vertex(const int v, const T& val) {
        assert(_vertices);
        assert(0 <= v && v < _vertex_count);
        _vertices[v] = val;
    }
    
    /**
     * @brief adds or removes edge <i, j> to edge set
     * @details adds or removes edge <i, j> to edge set. Note that indices start with 0. I.e. valid values for i, j are 0,...,vertex_count-1
     * 
     * @param i predecessor 
     * @param j successor
     * @param val positive value will add the edge <i, j> to the edge set, 0 remove.
     */
    void set(const int i, const int j, const unsigned char val) {
        assert(validIndex(i) && validIndex(j));
        _m.set(i, j, val);
    }
    /**
     * @brief returns whether edge belongs to edge set or not
     * @details whether edge belongs to edge set or not. Positive values indicate edge belongs to edge set.
     * 
     * @param i predecessor
     * @param j successor
     * 
     * @return positive values mean edge belongs to edge set, 0 otherwise not.
     */
    unsigned char get(const int i, const int j) {
        assert(validIndex(i) && validIndex(j));
        return _m.get(i, j);
    }

    /**
     * @brief returns information on number of vertices of graph
     * @return number of nodes, vertices respectively of the graph
     */
    unsigned int vertex_count() {return _vertex_count;}
    
};


#endif /* defined(__RanGenConv__Graph__) */
