#pragma once

/** Main include file for RobsClapHelpers. To use these helpers, you need to include this file 
somewhere in your code and add the RobsClapHelpers.cpp file to your project. The latter is a unity
build file meaning that it includes all the .cpp files from RobsClapHelpers and compiles them in
one compilation unit. If you want want to add the separate .cpp files to your project for browsing
the code, you need to exclude them from the build. */

#include <vector>
#include <string>
#include <sstream>       // ostringstream
#include <iomanip>       // setprecision
#include <limits>        // numeric_limits
#include <cassert>       // assert
#include <cstring>       // strcmp


#include "../clap/include/clap/clap.h"   // Only the stable API, no drafts


#include "Utilities.h"
#include "ClapPlugin.h"
#include "ClapPluginClasses.h"