
#include <algorithm>                // for min

#include "ClapPluginTests.h"


bool runAllClapTests(bool printResults)
{
  bool ok = true;

  ok &= runStateRecallTest();

  return ok;
}



// Some helpers to mock the clap stream objects that will be provided by the host during state
// load/save. This stuff needs verification. I don't know if my mock-stream behaves the way it is 
// intended by the CLAP API. I'm just guessing.

struct ClapStreamData
{
  std::vector<uint8_t> data;
  size_t pos = 0;
};

int64_t clapStreamWrite(
  const struct clap_ostream* stream, const void* buffer, uint64_t size)
{
  // Determine the number of bytes to write:
  uint64_t writeLimit = 25;        // To simulate limited number of bytes written per call
  uint64_t numToWrite = std::min(writeLimit, size);

  // Adjust size of the stream's data array:
  ClapStreamData* (csd) = (ClapStreamData*) (stream->ctx);
  uint64_t newSize      = csd->data.size() + numToWrite;
  csd->data.resize(newSize);

  // Write data from the passed buffer into the stream's data object:
  uint8_t* buf = (uint8_t*) buffer;
  for(uint64_t i = 0; i < numToWrite; i++)
    csd->data[csd->pos+i] = buf[i];
  csd->pos += numToWrite;

  // Return the number of bytes written:
  return numToWrite;
}

int64_t clapStreamRead(const struct clap_istream* stream, void* buffer, uint64_t size)
{
  uint64_t readLimit = 25;
  uint64_t numToRead = std::min(readLimit, size);

  ClapStreamData* (csd) = (ClapStreamData*) (stream->ctx);
  uint64_t remaining = csd->data.size() - csd->pos;
  numToRead = std::min(numToRead, remaining);

  uint8_t* buf = (uint8_t*) buffer;
  for(uint64_t i = 0; i < numToRead; i++)
    buf[i] = csd->data[csd->pos+i];
  csd->pos += numToRead;

  return numToRead;
}

bool runStateRecallTest()
{
  bool ok = true;

  // Create a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::pluginDescriptor;
  ClapGain gain(&desc, nullptr);

  double p;  // Used for the parameter value

  // Set up gain and pan (along the way, check if this works) and then retrieve the state:
  gain.setParameter(0,  6.02); ok &= gain.paramsValue(0, &p); ok &= p == 6.02;  // Gain
  gain.setParameter(1, -0.3 ); ok &= gain.paramsValue(1, &p); ok &= p == -0.3;  // Pan
  std::string stateString = gain.getStateAsString();

  // Modify gain and pan and then restore the state:
  gain.setParameter(0,  3.14); ok &= gain.paramsValue(0, &p); ok &= p == 3.14;
  gain.setParameter(1,  0.75); ok &= gain.paramsValue(1, &p); ok &= p == 0.75;
  ok &= gain.setStateFromString(stateString);

  // Check if the parameter values were successfully restored:
  ok &= gain.paramsValue(0, &p);  ok &= p == 6.02;
  ok &= gain.paramsValue(1, &p);  ok &= p == -0.3;
  
  // Create a ClapStreamData object to hold the data and clap output stream object with the
  // streamData:
  ClapStreamData streamData;
  clap_ostream ostream;
  ostream.write = clapStreamWrite;
  ostream.ctx   = &streamData;

  // Save state to the stream and mess up the state:
  gain.stateSave(&ostream);
  gain.setParameter(0,  3.14); ok &= gain.paramsValue(0, &p); ok &= p == 3.14;
  gain.setParameter(1,  0.75); ok &= gain.paramsValue(1, &p); ok &= p == 0.75;

  // Create a clap input stream object with the streamData:
  streamData.pos = 0;       // Reset poisition for reading
  clap_istream istream;
  istream.read = clapStreamRead;
  istream.ctx  = &streamData;

  // Load state from the stream and check if state was recalled correctly:
  gain.stateLoad(&istream);
  ok &= gain.paramsValue(0, &p); ok &= p == 6.02;
  ok &= gain.paramsValue(1, &p); ok &= p == -0.3;

  return ok;

  // ToDo:
  // -Test this with plugins with more parameters such that the ids get longer strings
  // -Test it with parameters that have an empty string as name
}


/*

Format for plugin states:

CLAP plugin state

Format : RS01                             optional, if missing, assume RS01 - can also be JSON, XML
Plugin-Id : RS-MET.StereoGain             mandatory
Plugin-Vendor : RS-MET                    optional
Plugin-Version : 2024.04.01               optional
NumParams : 2                             optional
Params : [0:Gain:-6.02;1:Pan:0.25]        manadatory if params exist





For serialization, see:
https://gist.github.com/shenfeng/4016355
https://stackoverflow.com/questions/51230764/serialization-deserialization-of-a-vector-of-integers-in-c
https://codereview.stackexchange.com/questions/280469/how-can-i-optimize-c-serialization
https://uscilab.github.io/cereal/serialization_functions.html

https://github.com/ShahriarRezghi/serio      Library for serialization
https://uscilab.github.io/cereal/            another one
https://flatbuffers.dev/                     dito
https://www.boost.org/doc/libs/1_79_0/libs/serialization/doc/index.html
https://cpp.libhunt.com/libs/serialization   List of C++ serialization libraries
https://liamappelbe.medium.com/serialization-in-c-3d381e66ecec

Hmm...this is difficult. Maybe we should use a std::string in a particular format like:

Plugin : Gain
Version : 1
Format : 1
Parameters : [Gain:0:12.54,Pan:1:-0.34]

The format for the parameters is Name:id:value ...or maybe id:name:value, like
0:Gain:12.54,1:Pan:-0.34  ...maybe use a semicolon ; as separator - maybe the comma should be
reserved for later (maybe parameter arrays)

 
See: https://www.json.org/json-en.html
https://en.wikipedia.org/wiki/JSON

*/
