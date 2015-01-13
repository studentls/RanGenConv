//
//  helper.h
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 13.01.15.
//  Copyright (c) 2015 Leonhard Spiegelberg. All rights reserved.
//

#ifndef RanGenConv_helper_h
#define RanGenConv_helper_h

#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <algorithm>

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

#else
#include <getopt.h>
#endif

// helper func to draw a uniform random variable
inline double drandom(const double dmin, const double dmax) {
    return dmin + (dmax - dmin) * (rand()%RAND_MAX) / (double)RAND_MAX;
}

// returns a draw of a exponential distributed random variable
inline double exprv(const double rate) {
    return -1.0 * log(1.0 - drandom(0.0, 1.0)) / rate;
}

// return geometric distributed random variable
inline int georv(const double rate) {
    using namespace std;
    // dont forget max, as random generator can return 0
    return max((int)floor(log(drandom(0.0, 1.0)) / log(1.0 - rate)), 0);
}

#endif
