//
//  helper.h
//  RanGenConv
//
//  Created by Leonhard Spiegelberg on 13.01.15.
//  Copyright (c) 2015 Leonhard Spiegelberg. All rights reserved.
//

#ifndef RanGenConv_helper_h
#define RanGenConv_helper_h

#include <cassert>
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
#include "getopt_win.h" // windows does not support getpot, use some backup here...
#else
#include <getopt.h>
#endif

/**
 * @brief returns double random variable uniformly distributed on [dmin, dmax]
 * @details returns double random variables uniformly distributed on [dmin, dmax]
 * 
 * @param dmin lower bound of interval
 * @param dmax upper bound of interval
 * 
 * @return realisation of uniformly distributed random variable on [dmin, dmax]
 */
inline double drandom(const double dmin, const double dmax) {
    return dmin + (dmax - dmin) * (rand()%RAND_MAX) / (double)RAND_MAX;
}

/**
 * @brief draw of geometric distributed random variable
 * @details returns a realisation of a geometric distributed random variable. Note that rate has to be positive.
 * 
 * @param rate rate of the geometric distribution
 * @return realisation of a geometric distributed random variable
 */
inline int georv(const double rate) {
    using namespace std;
    assert(rate >= 0);
    // dont forget max, as random generator can return 0
    return max((int)floor(log(drandom(0.0, 1.0)) / log(1.0 - rate)), 0);
}

#endif
