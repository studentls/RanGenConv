//
//  RanGenFile.h
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 12.01.15.
//  Copyright (c) 2015 Leonhard Spiegelberg. All rights reserved.
//

#ifndef RanGenConv_RanGenFile_h
#define RanGenConv_RanGenFile_h

#include "Graph.h"
#include "helper.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

/**
 * @brief struct to hold one data line of the patterson format
 * @details struct to hold one data line of the patterson format. Is used as assigned data to a node.
 */
class node {
public:
    int                 id;                     /**< min, max are the dummynodes --> starts with 1 */
    int                 activity_duration;      /**< duration of a activit, equals 1.0 / maxprogress*/
    std::vector<int>    resource_requirements;  /**< resource requirements of an acitivity*/
    int                 num_successors;         /**< number of successors */
    std::vector<int>    children;               /**< chidlren, indexing starts with 1 */
    int                 release;
    int                 deadline;
    
    node() : activity_duration(0), num_successors(0), release(0), deadline(0), id(-1) {
        
    }
};

/**
 * @brief holds data of a given file in Pattersonformat
 * 
 */
class RanGenFile {
private:
    bool                    _bad;
    int                     num_resources;              /**< number of renewable resources */
    std::vector<int>        _resource_availability;     /**< vector containing availabilitys */
                                                        /**< of the num_resources resources */
    Graph<node>             G;                          /**< graph to store all information */
    Adjacencymatrix         m;                          /**< adjacencymatrix corresponding to nodes */
    
    // util functions
    bool                    parse_file(std::string filename);
    bool                    build_adjmatrix(const std::vector<node>& V);
    
public:
    RanGenFile():_bad(false) {}
    RanGenFile(std::string filename);
    ~RanGenFile();
    
    bool                    generate_times(const int time_limit);
    bool                    validate_times();
    bool                    validate_file(const bool verbose);
    bool                    bad() {return _bad;}
    
    unsigned int            node_count() {return G.vertex_count();}
    unsigned int            resource_count() {return num_resources;}
    
    const std::vector<int>& resource_availability() {return _resource_availability;}
    
    node&                   nodes(const unsigned int i) {
        assert(0 <= i && i < node_count());
        return G.v(i);
    }
    
    /**
     * @brief returns entry of adjacency matrix for edge <i, j>
     * @details returns entry of adjacency matrix for edge <i, j>. Positive values mean edge <i, j> belongs to edge set
     * 
     * @param i predecessor
     * @param j successor
     * 
     * @return value indicating if <i, j> belongs to edge set. (Positive means yes, 0 no)
     */
    unsigned char           get(const unsigned int i, const unsigned int j) {return G.get(i, j);}
};


#endif
