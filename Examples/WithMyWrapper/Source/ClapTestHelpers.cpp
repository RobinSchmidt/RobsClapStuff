
#include "ClapTestHelpers.h"

//=================================================================================================
// Streams

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

//=================================================================================================
// Buffers

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

void initEventHeader(clap_event_header_t* hdr, uint32_t time)
{
  hdr->size     = -1;    // uint32_t, still invalid - must be assigned by "subclass" initializer
  hdr->time     =  time; // uint32_t
  hdr->space_id =  0;    // uint16_t, 0 == CLAP_CORE_EVENT_SPACE?
  hdr->type     = -1;    // uint16_t, still invalid
  hdr->flags    =  0;    // uint32_t, 0 == CLAP_EVENT_IS_LIVE
}


clap_event_param_value createParamValueEvent(clap_id paramId, double value, uint32_t time)
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







//=================================================================================================
// ClapGain2

const char* const ClapGain2::features[5] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_UTILITY, 
  CLAP_PLUGIN_FEATURE_MIXING, 
  CLAP_PLUGIN_FEATURE_MASTERING,            // Was not present in the old version
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