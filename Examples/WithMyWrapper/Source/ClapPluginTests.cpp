
#include <algorithm>                // for min

#include "ClapPluginTests.h"


bool runAllClapTests(bool printResults)
{
  bool ok = true;

  ok &= runStateRecallTest();
  ok &= runDescriptorReadTest();
  ok &= runNumberToStringTest();
  ok &= runIndexIdentifierMapTest();

  ok &= runWaveShaperTest();

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


/** A dummy class to simulate addition and re-ordering of parameters in an updated version of a 
plugin. We want to check, that state-recall still works with the new version. This "Gain 2" plugin
mocks an updated "StereoGain" plugin that has one parameter more and the old parameters in a 
different order ...TBC... */

class ClapGain2 : public RobsClapHelpers::ClapPluginStereo32Bit
{

public:

  enum ParamId
  {
    kGain,
    kPan,
    // Up to here, this matches the ParamId enum of the ClapGain class, i.e. the "old version" of
    // the plugin. This is important. If it doesn't match, trying to set up a new version with a 
    // state stored by the old version will mix up the parameters, i.e. break the state recall and 
    // put the plugin into a garbage state.

    // From here, new parameters are introduced that were not present in the old version:
    kMidSide,
    kMono,


    numParams
  };


  ClapGain2(const clap_plugin_descriptor* desc, const clap_host* host)  
    : ClapPluginStereo32Bit(desc, host) 
  {
    // Flags for our parameters - they are automatable:
    clap_param_info_flags automatable = CLAP_PARAM_IS_AUTOMATABLE;

    // Add the parameters. The order in which we add them here determines their "index" which in 
    // turn determines the order in which the host presents the knobs/sliders. The "id", on the 
    // other hand, is determined by our enum and must remain stable from version to version. We can
    // reorder the parameters on the host-generated GUI - but we cannot reorder the ids, once they
    // have been assigned.
                                                                      // new index   old index   id
    addParameter(kMono,    "Mono",     0.0,  +1.0, 0.0, automatable); // 0           none        3
    addParameter(kMidSide, "MidSide",  0.0,  +1.0, 0.5, automatable); // 1           none        2
    addParameter(kPan,     "Pan",     -1.0,  +1.0, 0.0, automatable); // 2           1           1
    addParameter(kGain,    "Gain",   -40.0, +40.0, 0.0, automatable); // 3           0           0

    // In the "old version", index and id did actually match but in the new version, it's all 
    // messed up. The state recall should nevertheless work - even for a state saved with the old 
    // version. A unit test verifies this...
  }

  static const char* const features[5];
  static const clap_plugin_descriptor_t descriptor;


  // Dummy functions - we need to override them because they are purely virtual in the baseclass:
  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR,
    uint32_t numFrames) override {}
  void parameterChanged(clap_id id, double newValue) override {}
  // The unit tests are actually not interested in what they do, so we can leave them empty for 
  // the time being. The test is just concerned with state recall after a version update with 
  // parameter extension and reordering.


};

const char* const ClapGain2::features[5] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_UTILITY, 
  CLAP_PLUGIN_FEATURE_MIXING, 
  CLAP_PLUGIN_FEATURE_MASTERING,       // was not present in the old version
  NULL 
};

const clap_plugin_descriptor_t ClapGain2::descriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.StereoGainDemo",  // This field must match the old version's
  .name         = "StereoGainDemo",         // ...all the other fields are (probably) not important
  .vendor       = "",
  .url          = "",
  .manual_url   = "",
  .support_url  = "",
  .version      = "0.0.0",
  .description  = "Stereo gain and panning",
  .features     = ClapGain2::features,
};




bool runStateRecallTest()
{
  bool ok = true;

  // Create a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::descriptor;
  ClapGain gain(&desc, nullptr);
  using ID = ClapGain::ParamId;


  double p;  // Used for the parameter value

  // Set up gain and pan (along the way, check if this works) and then retrieve the state:
  gain.setParameter(ID::kGain,  6.02); ok &= gain.paramsValue(0, &p); ok &= p == 6.02;
  gain.setParameter(ID::kPan,  -0.3 ); ok &= gain.paramsValue(1, &p); ok &= p == -0.3;
  std::string stateString = gain.getStateAsString();

  // Modify gain and pan and then restore the state:
  gain.setParameter(ID::kGain, 3.14); ok &= gain.paramsValue(0, &p); ok &= p == 3.14;
  gain.setParameter(ID::kPan,  0.75); ok &= gain.paramsValue(1, &p); ok &= p == 0.75;
  ok &= gain.setStateFromString(stateString);

  // Check if the parameter values were successfully restored:
  ok &= gain.paramsValue(ID::kGain, &p);  ok &= p == 6.02;
  ok &= gain.paramsValue(ID::kPan,  &p);  ok &= p == -0.3;
  
  // Create a ClapStreamData object to hold the data and clap output stream object with the
  // streamData:
  ClapStreamData streamData;
  clap_ostream ostream;
  ostream.write = clapStreamWrite;
  ostream.ctx   = &streamData;

  // Save state to the stream and mess up the state:
  gain.stateSave(&ostream);
  gain.setParameter(ID::kGain, 3.14); ok &= gain.paramsValue(0, &p); ok &= p == 3.14;
  gain.setParameter(ID::kPan,  0.75); ok &= gain.paramsValue(1, &p); ok &= p == 0.75;

  // Create a clap input stream object with the streamData:
  streamData.pos = 0;       // Reset position for reading - use a function reset()
  clap_istream istream;
  istream.read = clapStreamRead;
  istream.ctx  = &streamData;

  // Load state from the stream and check if state was recalled correctly:
  gain.stateLoad(&istream);
  ok &= gain.paramsValue(ID::kGain, &p); ok &= p == 6.02;
  ok &= gain.paramsValue(ID::kPan,  &p); ok &= p == -0.3;


  // Create a ClapGain2 object - representing an updated version of the plugin:
  clap_plugin_descriptor_t desc2 = ClapGain2::descriptor;
  ClapGain2 gain2(&desc2, nullptr);
  using ID2 = ClapGain2::ParamId;

  // Set up parameters to some "random" values:
  gain2.setParameter(ID2::kGain,    3.01);
  gain2.setParameter(ID2::kPan,     0.25);
  gain2.setParameter(ID2::kMono,    1.0 );
  gain2.setParameter(ID2::kMidSide, 0.2 );

  // Recall the state of the "new" version (i.e. ClapGain2) with the state stream created with the 
  // "old" version (i.e. ClapGain):
  streamData.pos = 0;
  gain2.stateLoad(&istream);

  // Check if the Gain and Pan parameters have the values stored in the old state and the Mono and
  // MidSide parameters (which did not exist in the old version) are at their default values:
  ok &= gain2.paramsValue(ID2::kGain,    &p); ok &= p ==  6.02;
  ok &= gain2.paramsValue(ID2::kPan,     &p); ok &= p == -0.3;
  ok &= gain2.paramsValue(ID2::kMono,    &p); ok &= p ==  0.0;
  ok &= gain2.paramsValue(ID2::kMidSide, &p); ok &= p ==  0.5;


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
  clap_plugin_descriptor_t desc = ClapGain::descriptor;
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


  // Now test the string copying function:

  // Buffer more than long enough:
  initBuffer();
  pos = copyString("0123456789", buf, 20);
  ok  &= Str(buf) == Str("0123456789");
  ok  &= pos == 10;

  // Buffer exactly long enough:
  initBuffer();
  pos = copyString("0123456789", buf, 11);
  ok  &= Str(buf) == Str("0123456789");
  ok  &= pos == 10;

  // Buffer too short by 1:
  initBuffer();
  pos = copyString("0123456789", buf, 10);
  ok  &= Str(buf) == Str("012345678");
  ok  &= pos == 9;

  // Buffer has length 2:
  initBuffer();
  pos = copyString("0123456789", buf, 2);
  ok  &= Str(buf) == Str("0");
  ok  &= pos == 1;


  return ok;

  // ToDo:
  // -Check if automatic switch to exponential notation works as intended
  // -Use an empty string as suffix
}

bool runIndexIdentifierMapTest()
{
  bool ok = true;

  //
  // index:  0 1 2 3 4 5 6
  // ident:  3 2 4 0 6 5 1


  using namespace RobsClapHelpers;


  // We create the map and add the pairs in "random" order. Along the way, we check, if it has the
  // expected size:
  IndexIdentifierMap map;
  map.addIndexIdentifierPair(1, 2); ok &= map.getNumEntries() == 3;
  map.addIndexIdentifierPair(3, 0); ok &= map.getNumEntries() == 4;
  map.addIndexIdentifierPair(2, 4); ok &= map.getNumEntries() == 5;
  map.addIndexIdentifierPair(0, 3); ok &= map.getNumEntries() == 5;
  map.addIndexIdentifierPair(5, 5); ok &= map.getNumEntries() == 6;
  map.addIndexIdentifierPair(4, 6); ok &= map.getNumEntries() == 7;
  map.addIndexIdentifierPair(6, 1); ok &= map.getNumEntries() == 7;

  // During the creaion process, the map may have been in an inconsisten state. But now that we are 
  // finished with filling it, the state should be consistent. Check that:
  ok &= map.isConsistent();

  // Check the index-to-indentifier mapping:
  ok &= map.getIdentifier(0) == 3;
  ok &= map.getIdentifier(1) == 2;
  ok &= map.getIdentifier(2) == 4;
  ok &= map.getIdentifier(3) == 0;
  ok &= map.getIdentifier(4) == 6;
  ok &= map.getIdentifier(5) == 5;
  ok &= map.getIdentifier(6) == 1;

  // check the identifier-to-index mapping:
  ok &= map.getIndex(0) == 3;
  ok &= map.getIndex(1) == 6;
  ok &= map.getIndex(2) == 1;
  ok &= map.getIndex(3) == 0;
  ok &= map.getIndex(4) == 2;
  ok &= map.getIndex(5) == 5;
  ok &= map.getIndex(6) == 4;




  return ok;
}

bool runWaveShaperTest()
{
  bool ok = true;

  // Create the WaveShaper:
  clap_plugin_descriptor_t desc = ClapWaveShaper::descriptor;
  ClapWaveShaper ws(&desc, nullptr);


  using ID = ClapWaveShaper::ParamId;


  // Helper function to check, if the given shapeIndex is mapped to the given shapeString by the
  // ClapWaveShaper object:
  auto checkShapeToString = [&](int shapeIndex, const std::string& shapeString)
  {
    static const int bufSize = 32;
    char buf[bufSize];
    ws.paramsValueToText(ID::kShape, (double) shapeIndex, buf, bufSize);
    std::string str(buf);
    return str == shapeString;
  };

  // This checks the reverse mapping, i.e. from string back to shape-index:
  auto checkStringToShape = [&](const std::string& shapeString, int shapeIndex)
  {
    double value;
    bool found = ws.paramsTextToValue(ID::kShape, shapeString.c_str(), &value);
    if(!found)
      return false;
    return value == (double) shapeIndex;
  };

  // This checks both forward and backward mapping, i.e. shape index to and from string:
  auto checkShapeString = [&](int shapeIndex, const std::string& shapeString)
  {
    bool toStringOK   = checkShapeToString(shapeIndex, shapeString);
    bool fromStringOK = checkStringToShape(shapeString, shapeIndex);
    return toStringOK && fromStringOK;
  };




  // Check if the shape-id to string mapping works correctly:
  using Shapes = ClapWaveShaper::Shapes;
  ok &= checkShapeString(Shapes::kClip, "Clip");
  ok &= checkShapeString(Shapes::kTanh, "Tanh");
  ok &= checkShapeString(Shapes::kAtan, "Atan");
  ok &= checkShapeString(Shapes::kErf,  "Erf");


  return ok;

  // ToDo:
  //
  // -Check also what happens when we use double inputs for 
  //    ws.paramsValueToText(ClapWaveShaper::Params::kShape, ...
  //  The desired behavior is that it behaves like rounding, i.e. 0..0.5 should give the same 
  //  result as the integer 0, 0.5..1.5 the same result as 2, etc.
}


/*

ToDo:

- Test processing function. Test also, if sample accurate automation works (not sure how to test
  that, though)

*/
