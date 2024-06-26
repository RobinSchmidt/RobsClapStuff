#pragma once

#include "DemoPlugins.h"

// This file contains functions and classes that facilitate unit testing of the plugins by mocking 
// certain data structures that are provided by the host in normal plugin use.
//
// ToDo: add documentation

//=================================================================================================
// Test Signal Creation

template<class T>
inline void createSinCosSignal(T* s, T* c, uint32_t N, T w)
{
  for(uint32_t n = 0; n < N; n++)
  {
    s[n] = sin(w*n);
    c[n] = cos(w*n);
  }
}


//=================================================================================================
// Streams
//
// Some helpers to mock the clap stream objects that will be provided by the host during state
// load/save. This stuff needs verification. I don't know if my mock-stream behaves the way it is 
// intended by the CLAP API. I'm just guessing.

struct ClapStreamData
{
  std::vector<uint8_t> data;
  size_t pos = 0;              // Current read or write position
};

int64_t clapStreamWrite(const struct clap_ostream* stream, const void* buffer, uint64_t size);

int64_t clapStreamRead(const struct clap_istream* stream, void* buffer, uint64_t size);

//=================================================================================================
// Events
//
// ToDo: make functions for creating note-events in different dialects

void initEventHeader(clap_event_header_t* hdr, uint32_t time = 0);

void initClapInEventBuffer(clap_input_events* b);

void initClapOutEventBuffer(clap_output_events* b);

clap_event_param_value createParamValueEvent(clap_id paramId, double value, uint32_t time = 0);


union ClapEvent
{
  clap_event_midi        midi;
  clap_event_note        note;
  clap_event_param_value paramValue;

  // ...more to come...
};


/** Returns the type of the given event. The return value is one of the values in the unnamed enum
in events.h - for example CLAP_EVENT_NOTE_ON, CLAP_EVENT_PARAM_VALUE, CLAP_EVENT_MIDI, etc. */
inline uint16_t getEventType(const ClapEvent* ev) { return ev->midi.header.type; }
// Needs test.
// Why has the enum no name? Wouldn't it make sense if it would be named clap_event_type?



//-------------------------------------------------------------------------------------------------

class ClapEventBuffer
{

public:

  uint32_t getNumEvents() { return (uint32_t) events.size(); }

  const clap_event_header_t* getEventHeader(uint32_t index)
  {
    return &events[index].midi.header; 
    // It should not matter which field of the union we use. The header has always the same 
    // meaning. We use mid here, but paramValue or note should work just as well.

    // This will cause an access violation when index >= numEvents, in particular, when numEvents
    // is zero. Maybe in this case, we should return a pointer to some dummy header - as in the
    // null-object pattern?.
  }

  void clear() { events.clear(); }

  void addEvent(const ClapEvent& newEvent) { events.push_back(newEvent); }

  void addParamValueEvent(clap_id paramId, double value, uint32_t time);

private:

  std::vector<ClapEvent> events;


  // ToDo:
  // 
  // -Provide functions like isSorted() and sort(), maybe merge()

};


//-------------------------------------------------------------------------------------------------

/** C++ wrapper around clap_input_events. By deriving from ClapEventBuffer, we inherit the owned
vector of events. */

class ClapInEventBuffer : public ClapEventBuffer
{

public:

  ClapInEventBuffer()
  {
    _inEvents.ctx  = this;
    _inEvents.size = ClapInEventBuffer::getSize;
    _inEvents.get  = ClapInEventBuffer::getEvent;
  }

  /** Returns a const pointer to our wrapped C-struct. */
  const clap_input_events* getWrappee() const { return &_inEvents; }


private:

  clap_input_events _inEvents;

  static uint32_t getSize(const struct clap_input_events* list)
  {
    ClapInEventBuffer* self = (ClapInEventBuffer*) list->ctx;
    return self->getNumEvents();
  }

  static const clap_event_header_t* getEvent(const struct clap_input_events* list, uint32_t index)
  {
    ClapInEventBuffer* self = (ClapInEventBuffer*) list->ctx;
    return self->getEventHeader(index);
  }

};

//-------------------------------------------------------------------------------------------------

class ClapOutEventBuffer : public ClapEventBuffer
{

public:

  ClapOutEventBuffer()
  {
    _outEvents.ctx      = this;
    _outEvents.try_push = ClapOutEventBuffer::tryPushEvent;
  }


  /** Returns a pointer to our wrapped C-struct. */
  clap_output_events* getWrappee() { return &_outEvents; }


private:

  clap_output_events _outEvents;

  static bool tryPushEvent(const struct clap_output_events *list, const clap_event_header_t *ev)
  {
    RobsClapHelpers::clapError("Not yet implemented");
    return false;
  }

};

//=================================================================================================
// Audio Buffers
//
// Classes for making it convenient to mock buffer objects like the clap_process struct that 
// gets passed to the processing function. Setting these buffer objects up by allocating the 
// required memory and setting up their pointers is a quite tedious task so it makes sense to have 
// convenience classes for that purpose.

void initClapProcess(clap_process* p);

void initClapAudioBuffer(clap_audio_buffer* b);

//-------------------------------------------------------------------------------------------------

class ClapAudioBuffer
{

public:

  ClapAudioBuffer(uint32_t newNumChannels = 1, uint32_t newNumFrames = 1)
  {
    setSize(newNumChannels, newNumFrames);
  }

  void setSize(uint32_t newNumChannels, uint32_t newNumFrames)
  {
    numChannels = newNumChannels;
    numFrames   = newNumFrames;
    allocateBuffers();
  }

  /** Returns a const pointer to our wrapped C-struct. */
  const clap_audio_buffer* getWrappee() const { return &_buffer; }

  clap_audio_buffer* getWrappee() { return &_buffer; }


  uint32_t getNumChannels() const { return numChannels; }

  uint32_t getNumFrames() const { return numFrames; }

  float* getChannelPointer(uint32_t index) { return channelPointers[index]; }


private:

  void allocateBuffers();

  clap_audio_buffer _buffer;

  std::vector<std::vector<float>> data;
  std::vector<float*> channelPointers;

  uint32_t numChannels = 1;   // Should be at least 1. Redundant - stored already in _buffer.channel_count
  uint32_t numFrames   = 1;   // Should be at least 1

  // ToDo:
  //
  // -Maybe templatize such that it can be used for float and double.
};

//-------------------------------------------------------------------------------------------------

/** A processing buffer with one input and one output port for audio signals. A port can have 
multiple channels, though. ...TBC... */

class ClapProcessBuffer_1In_1Out  // Maybe remove the underscores ...but it looks ugly without them
{

public:

  //ClapProcessBuffer_1In_1Out(uint32_t numChannels, uint32_t numFrames)
  //  : inBuf(numChannels, numFrames), outBuf(numChannels, numFrames)
  //{
  //  updateWrappee();
  //}


  ClapProcessBuffer_1In_1Out(uint32_t numInChannels, uint32_t numOutChannels, uint32_t numFrames)
    : inBuf(numInChannels, numFrames), outBuf(numOutChannels, numFrames)
  {
    updateWrappee();
  }


  //-----------------------------------------------------------------------------------------------
  // \name Setup

  /** Adds a parameter value change event to our buffer of input events. */
  void addInputParamValueEvent(clap_id paramId, double value, uint32_t time)
  { inEvs.addParamValueEvent(paramId, value, time); }


  /** Cleasr out buffer of input events. */
  void clearInputEvents() { inEvs.clear(); }

  /** Sets up the map that determines how the input buffers are mapped to output buffers in case of 
  in-place processing. In general, we should not assume that ins[i] == outs[i] in case of in-place 
  processing but just that in[i] == outs[j] for some permutation map i -> j where i,j = 0..k-1 and
  k = min(m,n) where m,n are numbers of inputs and outputs respectively. We want to be able to do 
  tests in which we mock arbitrary layouts. This function can be used for this purpose. */
  void setInPlaceBufferLayout(const std::vector<uint32_t>& newInOutMap) 
  { inPlaceLayout = newInOutMap; }
  // Convention: when this vector is empty, we do out of place processing, i.e. input and output 
  // buffers have their own memory.


  //-----------------------------------------------------------------------------------------------
  // \name Inquiry

  /** Returns true, iff this process buffer uses in-place audio buffers, i.e. the same memory 
  buffers are used for input and for output simultaneously. */
  bool usesInPlaceAudioBuffers() const { return !inPlaceLayout.empty(); }

  /** Returns the pointer to the input buffer for the channel with given index. */
  float* getInChannelPointer(uint32_t index) { return inBuf.getChannelPointer(index); }

  /** Returns the pointer to the output buffer for the channel with given index. */
  float* getOutChannelPointer(uint32_t index) { return outBuf.getChannelPointer(index); }

  /** Returns the number of input channels. */
  uint32_t getNumInChannels() const { return inBuf.getNumChannels(); }

  /** Returns the number of output channels. */
  uint32_t getNumOutChannels() const { return outBuf.getNumChannels(); }

  /** Returns a pointer to the wrapped clap_process struct. */
  clap_process* getWrappee() { return &_process; }
  // Maybe try to return a const pointer?


private:

  void updateWrappee(); // ToDo: add documentation

  // Data members:
  clap_process          _process;       // The wrapped C-struct
  ClapAudioBuffer       inBuf;          // Input audio buffer
  ClapAudioBuffer       outBuf;         // Output audio buffer
  ClapInEventBuffer     inEvs;          // Input event buffer
  ClapOutEventBuffer    outEvs;         // Output event buffer
  std::vector<uint32_t> inPlaceLayout;  // Map betwen in and out buffers for in-place mode

  // ToDo: 
  // -Maybe make non-copyable, etc.
  // -Maybe make a more general class that has multiple I/O ports
  // -Allow usage of in-place buffers. Allow the channels to be shuffled/permuted arbitrarily for 
  //  making tests with "weird" in-place buffer layouts.
  // -Allow 64 bit buffers. Maybe even allow an arbitrary mix of 32 and 64 bit buffers. CLAP allows
  //  such a thing.
  // -When this is done, getIn/OutChannelPointer to getIn/OutChannelPointer32Bit and have analogous
  //  function for the 64 bit pointers.
};

//=================================================================================================
// Test Plugins
//
// Some plugins for testing and debugging things like correct behavior in version updates, etc.

/** A dummy class to simulate addition and re-ordering of parameters in an updated version of a 
plugin. We want to check, that state-recall still works with the new version. This "Gain 2" plugin
mocks an updated "StereoGain" plugin that has two parameters more and the old parameters in a 
different order (index-wise - not id-wise - the ids must remain stable) ...TBC... */

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

  ClapGain2(const clap_plugin_descriptor* desc, const clap_host* host);

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

//-------------------------------------------------------------------------------------------------

/** A simple plugin to distribute the 2 left/right channels (inL, inR) of a stereo signal into 3 
left/center/right output channels (outL, outC, outR). It uses the rule:

  outC = (inL + inR)  *  centerScaler
  outL =  inL - diffScaler * outC
  outR =  inR - diffScaler * outC

where the centerScaler and diffScaler are user parameters. The purpose is to to test an uncommon
channel configuration to verify that the framework can handle it correctly. */

class ClapChannelMixer2In3Out : public RobsClapHelpers::ClapPluginWithAudio
{

public:

  enum ParamId
  {
    kCenterScale,
    kDiffScale,

    numParams
  };

  ClapChannelMixer2In3Out(const clap_plugin_descriptor* desc, const clap_host* host);

  bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) 
    const noexcept override;

  void parameterChanged(clap_id id, double newValue) override;

  void processSubBlock32(const clap_process* process, uint32_t begin, uint32_t end) override;

  void processSubBlock64(const clap_process* process, uint32_t begin, uint32_t end) override;

  static const char* const features[6];
  static const clap_plugin_descriptor_t descriptor;

protected:

  // DSP coeffs:
  float centerScaler = 0.f;  // get rid - use parameters directly - especially in the double
  float diffScaler   = 0.f;  // ...precision implementation
  // Initial settings leave stereo signal as is
  // Maybe use double...actually - we already have the double parameters stored in the inherited
  // array ....maybe use these in the double precision processing function. Generally, when a 
  // plugin supports double, we should also use double precsison coeffs. But in order to not store
  // the coeffs redundantly, we will then need to retrieve and convert the value to float in
  // processSubBlock32

};

//-------------------------------------------------------------------------------------------------

/** Plugin with one stereo in/out for a "carrier" and one mono input (without any output) for the 
"modulator". Tests an I/O port configuration that can be useful in practice for various forms of
cross-synthesis such as vocoders, sidechain-compressors (aka duckers), etc.. We do a simple 
amplitude modulation here. Left and right channels of the carrier get amp-modulated by the same
monophonic modulation signal by an adjustable amount. */

class ClapAmplitudeModulator : public RobsClapHelpers::ClapPluginWithAudio
{

public:

  enum ParamId
  {
    kAmpMod,

    numParams
  };

  ClapAmplitudeModulator(const clap_plugin_descriptor* desc, const clap_host* host);



protected:

};



