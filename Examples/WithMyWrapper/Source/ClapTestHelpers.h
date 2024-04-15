#pragma once

#include "DemoPlugins.h"

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
  clap_event_param_value paramValue;
  clap_event_midi        midi;
  clap_event_note        note;
  // ...more to come...
};


class ClapEventBuffer
{

public:

  uint32_t getNumEvents() { return (uint32_t) events.size(); }

  const clap_event_header_t* getEventHeader(uint32_t index)
  {
    return &events[index].paramValue.header; 
    // It should not matter which field of the union we use. The header has always the same 
    // meaning. We use paramValue here, but midi or note should work just as well.

    // This will cause an access violation when index >= numEvents, in particular, when numEvents
    // is zero. Maybe in this case, we should return a pointer to some dummy header - as in the
    // null-object pattern?.
  }

  void clear() { events.clear(); }

  void addEvent(const ClapEvent& newEvent) { events.push_back(newEvent); }

  void addParamValueEvent(clap_id paramId, double value, uint32_t time);

private:

  std::vector<ClapEvent> events;

};


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
  // Maybe move into .cpp file

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

