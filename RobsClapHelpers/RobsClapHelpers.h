#pragma once

/** Main include file for RobsClapHelpers. To use these helpers, you need to include this file 
somewhere in your code and add the RobsClapHelpers.cpp file to your project. The latter is a unity
build file meaning that it includes all the .cpp files from RobsClapHelpers and compiles them in
one compilation unit. If you want want to add the separate .cpp files to your project for browsing
the code, you need to exclude them from the build. */

// Standard library includes:
#include <vector>
#include <string>
#include <sstream>       // ostringstream
#include <iomanip>       // setprecision
#include <limits>        // numeric_limits
#include <algorithm>     // min, max
//#include <cassert>       // assert - obsoloete now - we now use clapAssert
#include <cstring>       // strcmp

// The CLAP SDK:
#include "../clap/include/clap/clap.h"   // Only the stable API, no draft extensions.
//#include "../clap/include/clap/all.h"  // This would also include the drafts.

// Rob's CLAP helpers, wrapped into their own namespace:
namespace RobsClapHelpers
{

#include "Utilities.h"
#include "ClapPlugin.h"
#include "ClapPluginClasses.h"

}