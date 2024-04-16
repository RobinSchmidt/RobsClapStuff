
#include "ClapPluginTests.h"
#include "GNUPlotter.h"        // For plotting output signals of the plugins when a test fails

bool runAllClapTests(/*bool printResults*/)
{
  bool ok = true;

  // Currently debugged test:
  //ok &= runProcessingTest2();

  // All tests in order:
  ok &= runStateRecallTest();
  ok &= runDescriptorReadTest();
  ok &= runNumberToStringTest();
  ok &= runIndexIdentifierMapTest();
  ok &= runWaveShaperTest();
  ok &= runProcessingTest1();
  ok &= runProcessingTest2();

  return ok;
}

//-------------------------------------------------------------------------------------------------
// State

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
  // -Test it with more weird numbers that really need the full 19 decimal digits 
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
  using namespace RobsClapHelpers;

  static const int bufSize = 20;
  char buf[bufSize];                // Character buffer into which we write the strings.

  // Helper function to initialize the buffer with some recognizable content which shall be 
  // overwritten by our number-to-string functions:
  auto initBuffer = [&]()
  {
    for(int i = 0; i < bufSize; i++)
      buf[i] = 'X';
  };

  // Abbreviation for the constructor of std::string from a C-string:
  auto Str = [](const char *cStr) { return std::string(cStr); };
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

  // During the creation process, the map may have been in an inconsistent state. But now that we 
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

bool runProcessingTest1()
{
  // In this test, we test the actual audio processing function "process" of the gain demo plugin
  // and check, if it produces the expected outputs. Before being ready to call process() on a
  // plugin object, we need to prepare a clap_process struct. That is a quite tedious task which we
  // do here manually - allocating all the memory, setting up the pointers in the struct, etc. Most
  // of the function deals with that stuff rather than the actual test. That also demonstrates how
  // this setup can be done on a low level.

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

  // We also need to set up the in/out event buffers or else we'll get an access violation:
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
  float w = 0.2f;  // Normalized radian freq of input sine
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
  gainDb = -20.0;
  inEventVec.push_back(createParamValueEvent(ID::kGain, gainDb, 0));

  // Compute output:
  gain.setAllParametersToDefault();  // Do a reset first
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

  // No add a second parameter change event in the middle of the buffer (the old one is still 
  // there):
  double gainDb2 = -10.0;
  inEventVec.push_back(createParamValueEvent(ID::kGain, gainDb2, N/2));

  // Compute the output again, now with 2 events in the event buffer:
  gain.setAllParametersToDefault();
  status = gain.process(&p);
  ok &= status == CLAP_PROCESS_CONTINUE;

  // Compute target signal and compare:
  gainLin = (float) dbToAmp(gainDb);      // 1st half of buffer should have gainDb applied
  for(int n = 0; n < N/2; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }
  gainLin = (float) dbToAmp(gainDb2);     // 2nd half of buffer should have gainDb2 applied
  for(int n = N/2; n < N; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }
  ok &= tgtL == outL;
  ok &= tgtR == outR;

  return ok;

  // ToDo:
  //
  // -Check, what happens if both the float and double buffers are non-nullptrs. I think, this 
  //  should not occur and counts as host misbehavior
  // -Test in place processing by using  outBuf.data32 = ins;
  // -Maybe factor out the whole tedious process of setting up the clap_process object into a 
  //  function. Maybe make a class ClapProcessBuffer that allocates all the required buffers. It 
  //  may also be useful for writing a clap host
  // -Check what happens when we have more than one event at one time instant
  // -Instead of calling gain.process directly, go through the actual CLAP API, i.e. obtain the 
  //  wrapped C-struct and let it do the processing
}

bool runProcessingTest2()
{
  // This test is quite similar to the one before just that here, we do not do all the tedious 
  // allocation and pointer setup work ourselves but instead use the convenience class
  // ClapProcessBuffer_1In_1Out which encapsulates the whole mumbo-jumbo. The setup overhead is
  // therefore much simplified here. ...TBC...

  bool ok = true;
  using namespace RobsClapHelpers;

  // Create and set up a ClapGain object:
  clap_plugin_descriptor_t desc = ClapGain::descriptor;
  ClapGain gain(&desc, nullptr);
  using  ID     = ClapGain::ParamId;       // For convenience
  double gainDb = -10.0;
  gain.setParameter(ID::kGain, gainDb);

  // Create a processing buffer:
  uint32_t numChannels =  2;  // Stereo
  uint32_t numFrames   = 60;  // 60 is nice - has many divisors
  ClapProcessBuffer_1In_1Out procBuf(numChannels, numFrames);

  // Retrieve pointers to the actual signal buffers:
  float* inL  = procBuf.getInChannelPointer(0);
  float* inR  = procBuf.getInChannelPointer(1);
  float* outL = procBuf.getOutChannelPointer(0);
  float* outR = procBuf.getOutChannelPointer(1);
  // Maybe it would be more convenient to work with references to the std::vectors

  // Create a test input signal - we use a sinusoid:
  uint32_t N = numFrames;  // Shorthand, because we need it often
  float w = 0.2f;          // Normalized radian freq of input sine
  //float w = 0.0f;        // DC - for test (plots are easier to interpret with DC)
  uint32_t n;              // Sample index
  for(n = 0; n < N; n++)
  {
    inL[n] = sin(w*n);
    inR[n] = cos(w*n);
  }

  // Compute target output:
  float gainLin = (float) dbToAmp(gainDb);
  std::vector<float> tgtL(N), tgtR(N);
  for(n = 0; n < N; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }

  // Let the gain plugin compute its output for the given sine input:
  clap_process_status status = gain.process(procBuf.getWrappee());
  ok &= status == CLAP_PROCESS_CONTINUE;
  ok &= equals(&tgtL[0], outL, N);
  ok &= equals(&tgtR[0], outR, N);


  // OK - so far so good. This was a process buffer without any events. Next, we create 3 events
  // at indices 0, N/3 and 2*N/3 to change the gain (in dB) to +1, +3, -2 respectively. We expect 
  // to see the 1st 3rd of the buffer with +1 dB, etc. We verify that this is actually the case.


  // Create the 3 gain-change events and put them into the input event list for the process buffer:
  double gainDb0 = +1.0; uint32_t n0 = 0;
  double gainDb1 = +3.0; uint32_t n1 = N/3;
  double gainDb2 = -2.0; uint32_t n2 = 2*N/3;
  procBuf.addInputParamValueEvent(ID::kGain, gainDb0, n0);
  procBuf.addInputParamValueEvent(ID::kGain, gainDb1, n1);
  procBuf.addInputParamValueEvent(ID::kGain, gainDb2, n2);

  // Compute target output:
  gainLin = (float) dbToAmp(gainDb0);    // 1.12201846
  for(n = 0; n < n1; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }
  gainLin = (float) dbToAmp(gainDb1);    // 1.41253757
  for(n = n1; n < n2; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }
  gainLin = (float) dbToAmp(gainDb2);    // 0.794328213
  for(n = n2; n < N; n++)
  {
    tgtL[n] = gainLin * inL[n];
    tgtR[n] = gainLin * inR[n];
  }

  // Let the plugin compute its output with the gain-change events and compare to target output:
  gain.setAllParametersToDefault();
  status = gain.process(procBuf.getWrappee());
  ok &= status == CLAP_PROCESS_CONTINUE;
  ok &= equals(&tgtL[0], outL, N);
  ok &= equals(&tgtR[0], outR, N);

  // Plot outputs and target signals:
  //GNUPlotter plt;
  //plt.plotArrays(N, &tgtL[0], outL, &tgtR[0], outR);

  return ok;

  // ToDo:
  //
  // -Test it with multiple events of the same kind at a single sample. Only the last event should
  //  count
  // -Test it with multiple events of different kinds at a single sample. They should all count.
  // -Test processing with in-place buffers
  // -Test the tone generator with midi and clap-note events (notes can be sent in different 
  //  "dialects")
}



/*

ToDo:


*/
