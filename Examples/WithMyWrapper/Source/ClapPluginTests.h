#pragma once

#include "DemoPlugins.h"


/** Runs all the tests and returns true when all pass, false otherwise */
bool runAllClapTests(bool printResults);

/** Tests the state recall functionality implemented in class ClapPluginWithParams. */
bool runStateRecallTest();
// Maybe rename to testParameterStateRecall. We may later have states that contain more than just
// numerical parameters (like strings for audiofile locations, maybe other data)


bool runDescriptorReadTest();


bool runNumberToStringTest();

bool runIndexIdentifierMapTest();



//bool runProcessingTest();