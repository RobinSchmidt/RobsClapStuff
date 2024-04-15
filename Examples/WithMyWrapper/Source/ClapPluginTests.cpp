
#include "ClapPluginTests.h"

bool runAllClapTests(/*bool printResults*/)
{
  bool ok = true;

  ok &= runStateRecallTest();
  ok &= runDescriptorReadTest();
  ok &= runNumberToStringTest();
  ok &= runIndexIdentifierMapTest();
  ok &= runWaveShaperTest();
  ok &= runProcessingTest();

  return ok;
}

//-------------------------------------------------------------------------------------------------
// State

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
    // put the plugin into a garbage state after an attempted recall.

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

  using ID = ClapGain::ParamId;  // For convenience
  double p;                      // Used for the parameter value

  // Create a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::descriptor;
  ClapGain gain(&desc, nullptr);

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

//-------------------------------------------------------------------------------------------------
// Instantiation

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

  return ok;
}

//-------------------------------------------------------------------------------------------------
// Utilities

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

  // For this test, we use the following mapping between 7 indices and identifiers:
  //
  //   index:  0 1 2 3 4 5 6
  //   ident:  3 2 4 0 6 5 1

  // We create the map and add the pairs in "random" order. Along the way, we check, if it has the
  // expected size:
  using namespace RobsClapHelpers;
  IndexIdentifierMap map;
  map.addIndexIdentifierPair(1, 2); ok &= map.getNumEntries() == 3;
  map.addIndexIdentifierPair(3, 0); ok &= map.getNumEntries() == 4;
  map.addIndexIdentifierPair(2, 4); ok &= map.getNumEntries() == 5;
  map.addIndexIdentifierPair(0, 3); ok &= map.getNumEntries() == 5;
  map.addIndexIdentifierPair(5, 5); ok &= map.getNumEntries() == 6;
  map.addIndexIdentifierPair(4, 6); ok &= map.getNumEntries() == 7;
  map.addIndexIdentifierPair(6, 1); ok &= map.getNumEntries() == 7;

  // During the creation process, the map may have been in an inconsisten state. But now that we 
  // are finished with filling it, the state should be consistent. Check that:
  ok &= map.isConsistent();

  // Check the index-to-identifier mapping:
  ok &= map.getIdentifier(0) == 3;
  ok &= map.getIdentifier(1) == 2;
  ok &= map.getIdentifier(2) == 4;
  ok &= map.getIdentifier(3) == 0;
  ok &= map.getIdentifier(4) == 6;
  ok &= map.getIdentifier(5) == 5;
  ok &= map.getIdentifier(6) == 1;

  // Check the identifier-to-index mapping:
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

  // Helper function to check, if the given shapeIndex is mapped to the given shapeString by the
  // ClapWaveShaper object:
  using ID = ClapWaveShaper::ParamId;
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
  using Shape = ClapWaveShaper::Shape;
  ok &= checkShapeString(Shape::kClip, "Clip");
  ok &= checkShapeString(Shape::kTanh, "Tanh");
  ok &= checkShapeString(Shape::kAtan, "Atan");
  ok &= checkShapeString(Shape::kErf,  "Erf");

  return ok;

  // ToDo:
  //
  // -Check also what happens when we use double inputs for 
  //    ws.paramsValueToText(ClapWaveShaper::Params::kShape, ...
  //  The desired behavior is that it behaves like rounding, i.e. 0..0.5 should give the same 
  //  result as the integer 0, 0.5..1.5 the same result as 2, etc.
}

//-------------------------------------------------------------------------------------------------
// Processing

void initClapProcess(clap_process* p)
{
  p->steady_time         = 0;        // int64_t
  p->frames_count        = 0;        // uint32_t
  p->transport           = nullptr;  // const clap_event_transport_t *
  p->audio_inputs        = nullptr;  // const clap_audio_buffer_t *
  p->audio_outputs       = nullptr;  // clap_audio_buffer_t *
  p->audio_inputs_count  = 0;        // uint32_t
  p->audio_outputs_count = 0;        // uint32_t
  p->in_events           = nullptr;  // const clap_input_events_t *
  p->out_events          = nullptr;  // const clap_output_events_t *

  // ToDo:
  //
  // -Explain, how the out_events buffer is supposed to be used. We somehow seem to be expected to
  //  write into it, I guess. But it's a pointer to const - so it looks like we can't modify it. It 
  //  would be a bit strange anyway because the host can't possibly know the number of output 
  //  events, so it can't be responsible for the allocation. Maybe the plugin is supposed to have a
  //  pre-allocated buffer? Figure out! Ah: clap_output_events has a "try_push" method. Looks 
  //  like the host pre-allocates memory for a queue and the plugin can push events onto that 
  //  queue. The function may fail - then it will return false. Maybe in such a case, the plugin
  //  is supposed to try again in the next process call - or if it doesn't, the events will just 
  //  get lost. If it does, the events won't be lost but will be delayed.
}

void initClapAudioBuffer(clap_audio_buffer* b)
{
  b->data32        = nullptr;  // float**
  b->data64        = nullptr;  // double**
  b->channel_count = 0;        // uint32_t
  b->latency       = 0;        // uint32_t
  b->constant_mask = 0;        // uint64_t

  // Notes:
  //
  // -I think, the constant_mask has a bit set to 1 for each channel that is constant? If channel
  //  with index 0 is constant, the rightmost bit is one, if channel with index 1 is constant, the
  //  second-rightmost bit is 1 etc.? The doc says the purpose of this mask is to avoid processing
  //  "garbage" - I'm not quite sure what that means. DC signals can actually be quite useful in
  //  many contexts - of course, not directly as audio signals - but for control signals. Dunno.
  //  It's just a hint anyway and can probably be ignored.
  // -The latency field is supposed to tell us something about the "latency from/to the audio 
  //  interface" according to the doc.
}

void initClapInEventBuffer(clap_input_events* b)
{
  b->ctx  = nullptr;  // void*
  b->size = nullptr;  // function pointer: (const struct clap_input_events *list)  ->  uint32_t
  b->get  = nullptr;  // function pointer: (const struct clap_input_events *list, uint32_t index)
                      //                   -> const clap_event_header_t *

  // Notes:
  //
  // -Looks like these function pointers take as first argument a pointer to the struct itself. 
  //  This looks like th C-way of passing explicitly the "this" pointer that is the implicit first
  //  parameter of C++ classes in member functions. So, they are basically "member-functions" of 
  //  the struct
}

void initClapOutEventBuffer(clap_output_events* b)
{
  b->ctx      = nullptr;  // void*
  b->try_push = nullptr;  // (const struct clap_output_events *list, const clap_event_header_t *ev)
                          // -> bool
}

void initEventHeader(clap_event_header_t* hdr, uint32_t time = 0)
{
  hdr->size     = -1;    // uint32_t, still invalid - must be assigned by "subclass" initializer
  hdr->time     =  time; // uint32_t
  hdr->space_id =  0;    // uint16_t, 0 == CLAP_CORE_EVENT_SPACE?
  hdr->type     = -1;    // uint16_t, still invalid
  hdr->flags    =  0;    // uint32_t, 0 == CLAP_EVENT_IS_LIVE
}

clap_event_param_value createParamValueEvent(clap_id paramId, double value, uint32_t time = 0)
{
  // Create event and set up the header:
  clap_event_param_value ev;
  initEventHeader(&ev.header, time);
  ev.header.type = CLAP_EVENT_PARAM_VALUE;
  ev.header.size = sizeof(clap_event_param_value);

  // Set up the param_value specific fields and return the event:
  ev.param_id   = paramId;    // clap_id
  ev.cookie     = nullptr;    // void*
  ev.note_id    = -1;         // int32_t, -1 means: wildcard/unspecified/doesn't-matter/all
  ev.port_index = -1;         // int16_t
  ev.channel    = -1;         // int16_t
  ev.key        = -1;         // int16_t
  ev.value      = value;      // double
  return ev;
}


bool runProcessingTest()
{
  bool ok = true;

  using namespace RobsClapHelpers;

  // Create and set up a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::descriptor;
  ClapGain gain(&desc, nullptr);
  using  ID     = ClapGain::ParamId;       // For convenience
  double gainDb = -10.0;
  gain.setParameter(ID::kGain, gainDb);

  // Create an initialize the clap processing buffer:
  clap_process p;
  initClapProcess(&p);

  // Call process on the gain with the invalid processing buffer and check that it correctly 
  // returns an error:
  clap_process_status status = gain.process(&p);
  ok &= status == CLAP_PROCESS_ERROR;

  // Create and initialize the clap audio buffers to be used in the process buffer:
  clap_audio_buffer inBuf, outBuf;
  initClapAudioBuffer(&inBuf);
  initClapAudioBuffer(&outBuf);
  p.audio_inputs        = &inBuf;
  p.audio_inputs_count  = 1;
  p.audio_outputs       = &outBuf;
  p.audio_outputs_count = 1;

  // Now the process buffer has valid pointers to audio buffers. Try to run the gain again. It 
  // should still return CLAP_PROCESS_ERROR because the audio buffers themselves are still invalid:
  status = gain.process(&p);
  ok &= status == CLAP_PROCESS_ERROR;

  // Create the actual stereo input and output buffers as std::vectors and create length-2 arrays
  // of pointers to float pointing to the beginning of these vectors
  int N = 60;   // Processing buffer size
  std::vector<float> inL(N), inR(N), outL(N), outR(N);
  float *ins[2], *outs[2];
  ins[0]  = &inL[0];
  ins[1]  = &inR[0];
  outs[0] = &outL[0];
  outs[1] = &outR[0];

  // Assign the pointers in clap_audio_buffer objects:
  inBuf.data32         = ins;
  inBuf.channel_count  = 2;
  outBuf.data32        = outs;
  outBuf.channel_count = 2;

  // We also need to set up the in/out event buffers or else we'll get an access violation
  clap_input_events  inEvents;
  clap_output_events outEvents;
  initClapInEventBuffer(&inEvents);
  initClapOutEventBuffer(&outEvents);

  // Define some lambda functions and assign them to the function pointers in the inEvents buffer:
  auto inEventsSize0 = [](const struct clap_input_events *list) -> uint32_t
  {
    uint32_t result = 0;
    return result;
  };
  inEvents.size = inEventsSize0;

  auto inEventsGet0 = [](const struct clap_input_events* list, uint32_t index)
                         -> const clap_event_header_t *
  {
    clap_event_header_t* hdr = nullptr;
    return hdr;
  };
  inEvents.get = inEventsGet0;

  // Set up the in_events and out_events fields in the process buffer
  p.in_events  = &inEvents;
  p.out_events = &outEvents;

  // Now we are finally set up with valid buffers and can let the gain plugin process some audio:
  status = gain.process(&p);
  ok &= status == CLAP_PROCESS_CONTINUE;


  // If it all worked out so far, it means that the general infrastructure is correctly wired up. 
  // Our calls to gain.process so far were dummy runs with an empty (length zero) buffer. That's
  // actually a situation, that should not even occurr in practice. The buffers will always at 
  // least be of length 1 (for the sample frames). But the plugin can actually deal with length 
  // zero, too. We now try to do some actual processing...


  // Create a test input signal - we use a sinusoid:
  float w = 0.2;  // Normalized radian freq of input sine
  for(int n = 0; n < N; n++)
  {
    inL[n] = sin(w*n);
    inR[n] = cos(w*n);
  }

  // Compute target output:
  float gainLin = (float) dbToAmp(gainDb);
  std::vector<float> tgtL(N), tgtR(N);
  for(int n = 0; n < N; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }

  // Let the gain plugin compute its output for the given sine input:
  p.frames_count = N;
  status = gain.process(&p);
  ok &= status == CLAP_PROCESS_CONTINUE;

  // Check, if the output buffer matches the target:
  ok &= tgtL == outL;
  ok &= tgtR == outR;


  // So far there were not any events to process. Now we create some processing buffers with 
  // parameter change events....


  // Create a std::vector for the parameter change events, assign the inEvents.ctx to a pointer to
  // it (this is the "context", I guess) and then re-assign the function pointers in the inEvents 
  // buffer such that these functions use the vector instead of just returning zero or nullptr:
  std::vector<clap_event_param_value> inEventVec;
  inEvents.ctx = &inEventVec;

  auto inEventsSize = [](const struct clap_input_events *list) -> uint32_t
  {
    std::vector<clap_event_param_value>* vec;
    vec = (std::vector<clap_event_param_value>*) (list->ctx);
    uint32_t result = (uint32_t) vec->size();
    return result;
  };
  inEvents.size = inEventsSize;

  auto inEventsGet = [](const struct clap_input_events* list, uint32_t index)
    -> const clap_event_header_t *
  {
    std::vector<clap_event_param_value>* vec;
    vec = (std::vector<clap_event_param_value>*) (list->ctx);

    if(index < (uint32_t)vec->size() )
      return &(vec->at(index).header);
    else
      return nullptr;
  };
  inEvents.get = inEventsGet;

  // Now we are all set up. Whatever the content we put into our inEventVec, it will be received as
  // events in the process calls. We first start with a single event with sample offset 0. It sets
  // the dB-gain to -20.
  gain.setAllParametersToDefault();  // Do a reset first
  gainDb = -20.0;
  inEventVec.push_back(createParamValueEvent(ID::kGain, gainDb, 0));

  status = gain.process(&p);
  ok &= status == CLAP_PROCESS_CONTINUE;

  // Compute target output and compare to actual output:
  gainLin = (float) dbToAmp(gainDb);
  for(int n = 0; n < N; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }
  ok &= tgtL == outL;
  ok &= tgtR == outR;



  // Maybe have an uint32_t time as 3rd parameter



  //clap_event_param_value



  return ok;

  // ToDo:
  //
  // -Check, what happens if both the float and double buffers are non-nullptrs. I think, this 
  //  should not occur and counts as host misbehavior
  // -Test in place processing by using  outBuf.data32 = ins;
  // -Maybe factor out the whole tedious process of setting up the clap_process object into a 
  //  function. Maybe make a class ClapProcessBuffer that allocates all the required buffers. It 
  //  may also be useful for writing a clap host
}


/*

ToDo:

- Test processing function. Test also, if sample accurate automation works (not sure how to test
  that, though)

*/
