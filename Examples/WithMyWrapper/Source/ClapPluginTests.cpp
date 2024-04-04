
#include <algorithm>                // for min

#include "ClapPluginTests.h"


bool runAllClapTests(bool printResults)
{
  bool ok = true;

  ok &= runStateRecallTest();
  ok &= runDescriptorReadTest();
  ok &= runNumberToStringTest();

  return ok;
}


// Some helpers to mock the clap stream objects that will be provided by the host during state
// load/save. This stuff needs verification. I don't know if my mock-stream behaves the way it is 
// intended by the CLAP API. I'm just guessing.

struct ClapStreamData
{
  std::vector<uint8_t> data;
  size_t pos = 0;              // Current read or write position
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

  // Return the number of bytes written into the stream:
  return numToWrite;
}

int64_t clapStreamRead(const struct clap_istream* stream, void* buffer, uint64_t size)
{
  // Determine the number of bytes to read:
  uint64_t readLimit = 25;
  uint64_t numToRead = std::min(readLimit, size);
  ClapStreamData* (csd) = (ClapStreamData*) (stream->ctx);
  uint64_t remaining = csd->data.size() - csd->pos;
  numToRead = std::min(numToRead, remaining);

  // Read the bytes from the stream and write them into the buffer:
  uint8_t* buf = (uint8_t*) buffer;
  for(uint64_t i = 0; i < numToRead; i++)
    buf[i] = csd->data[csd->pos+i];
  csd->pos += numToRead;

  // Return the number of bytes consumed (i.e. written into the buffer):
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
  streamData.pos = 0;       // Reset position for reading
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
  // -Test it with more weird numbers that really need to the full 17 or 18 decimal digits because 
  //  the Gain.clap fails in the validator. maybe sue std::format
  // -Make state recall tests with randomized parameter values like the clap validator does. The 
  //  Gain.clap actually passes the state recall tests with the validator, so we should be fine. 
  //  But it would nevertheless be nice to have a similar test here in this test suite.
  // -Make a state recall test with a state string that has only values for a subset of all 
  //  parameters. Make sure that after the recall, the remaining parameters are at their default 
  //  values.
}

bool runDescriptorReadTest()
{
  bool ok = true;


  // Create a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::pluginDescriptor;
  ClapGain gain(&desc, nullptr);

  // Read the feature list. We expect it to have "audio-effect", "utility", "mixing" set:
  std::vector<std::string> features = gain.getFeatures();
  //ok &= features == { "audio-effect", "utility", "mixing" };  // Syntax error :-(
    // ...as much as I would like write the code like the line above, this apparently isn't 
    // currently possible with C++ so we have to do it the verbose way:
  ok &= features.size() == 3;
  if(ok)
  {
    ok &= features[0] == "audio-effect";
    ok &= features[1] == "utility";
    ok &= features[2] == "mixing";
  }
  // Note that it is important to have at least one of the main categories (audio-effect, 
  // instrument, note-effect, analyzer - I think) set. Otherwise the clap validator will complain.
  // The other ones are optional and for further, finer specification.


  return ok;
}


bool runNumberToStringTest()
{
  bool ok = true;


  static const int bufSize = 20;
  char buf[bufSize];                // Character buffer into which we write the strings.

  // Helper function to initilaize the buffer with some recognizable content which shall be 
  // overwritten by our number-to-string functions:
  auto initBuffer = [&]()
  {
    for(int i = 0; i < bufSize; i++)
      buf[i] = 'X';
  };

  // Abbreviation for the constructor of std::string from a C-string:
  auto Str = [](const char *cStr) { return std::string(cStr); };

  using namespace RobsClapHelpers;

  int pos;  // Maybe rename to len or length

  // Buffer more than long enough:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf, 20, 3 , nullptr);
  ok  &= Str(buf) == Str("2673.251");
  ok  &= pos == 8;

  // Buffer exactly long enough:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  9, 3, nullptr);
  ok  &= Str(buf) == Str("2673.251");
  ok  &= pos == 8;

  // Buffer too short by 1:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  8, 3, nullptr);
  ok  &= Str(buf) == Str("2673.25");
  ok  &= pos == 7;

  // Buffer too short by 3:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  6, 3, nullptr);
  ok  &= Str(buf) == Str("2673.");
  ok  &= pos == 5;

  // Buffer has length 2 - only the first digit and the null-terminator fit:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  2, 3, nullptr);
  ok  &= Str(buf) == Str("2");
  ok  &= pos == 1;

  // Buffer has length 1 - only the null-terminator fits in and we get an empty string:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  1, 3, nullptr);
  ok  &= Str(buf) == Str("");
  ok  &= pos == 0;

  // Buffer has length 0 - nothing is written into the buffer and the error code of -1 is 
  // returned:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, buf,  0, 3, nullptr);
  ok  &= pos == -1;  // -1 is the error code

  // Buffer is a nullptr - this is also an error:
  initBuffer();
  pos  = toStringWithSuffix(2673.2512891, nullptr, 20, 3, nullptr);
  ok  &= pos == -1; 

  // Big numbers - should use exponential notation:
  initBuffer();
  pos = toStringWithSuffix(1.e20, buf, bufSize, 3, nullptr);
  ok  &= Str(buf) == Str("1e+20");
  ok  &= pos == 5;
  initBuffer();
  pos = toStringWithSuffix(-1.e20, buf, bufSize, 3, nullptr);
  ok  &= Str(buf) == Str("-1e+20");
  ok  &= pos == 6;

  // Now with a suffix:
  initBuffer();
  pos = toStringWithSuffix(2673.2512891, buf, bufSize, 3, " Hz");
  ok  &= Str(buf) == Str("2673.251 Hz");
  ok  &= pos == 11;








  return ok;

  // ToDo:
  // -Check if automatic switch to exponential notation works as intended
}


/*

ToDo:

- Test processing function. Test also, if sample accurate automation works (not sure how to test
  that, though)

*/
