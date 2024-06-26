
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
// Events

void initEventHeader(clap_event_header_t* hdr, uint32_t time)
{
  hdr->size     = -1;    // uint32_t, still invalid - must be assigned by "subclass" initializer
  hdr->time     =  time; // uint32_t
  hdr->space_id =  0;    // uint16_t, 0 == CLAP_CORE_EVENT_SPACE?
  hdr->type     = -1;    // uint16_t, still invalid
  hdr->flags    =  0;    // uint32_t, 0 == CLAP_EVENT_IS_LIVE
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
  //  This looks like the C-way of passing explicitly the "this" pointer that is the implicit first
  //  parameter of C++ classes in member functions. So, they are basically "member-functions" of 
  //  the struct
}

void initClapOutEventBuffer(clap_output_events* b)
{
  b->ctx      = nullptr;  // void*
  b->try_push = nullptr;  // (const struct clap_output_events *list, const clap_event_header_t *ev)
                          // -> bool
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

void ClapEventBuffer::addParamValueEvent(clap_id paramId, double value, uint32_t time)
{
  ClapEvent ev;
  ev.paramValue = createParamValueEvent(paramId, value, time);
  events.push_back(ev);
}

//=================================================================================================
// Buffers

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

void initClapProcess(clap_process* p)
{
  p->steady_time         = 0;        // int64_t ...why not uin64_t? Can it be negative? 
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

//-------------------------------------------------------------------------------------------------

void ClapAudioBuffer::allocateBuffers()
{
  data.resize(numChannels);
  channelPointers.resize(numChannels);
  for(uint32_t c = 0; c < numChannels; c++)
  {
    data[c].resize(numFrames);
    channelPointers[c] = &data[c][0];
  }

  _buffer.channel_count = numChannels;
  _buffer.data32 = &channelPointers[0];

  // These are always the same at the moment:
  _buffer.data64        = nullptr;
  _buffer.constant_mask = 0;
  _buffer.latency       = 0;
}

//-------------------------------------------------------------------------------------------------

void ClapProcessBuffer_1In_1Out::updateWrappee()
{
  _process.audio_inputs        = inBuf.getWrappee();
  _process.audio_inputs_count  = 1;
  _process.audio_outputs       = outBuf.getWrappee();
  _process.audio_outputs_count = 1;
  _process.frames_count        = inBuf.getNumFrames();    // == outBuf.getNumFrames()
  _process.in_events           = inEvs.getWrappee();
  _process.out_events          = outEvs.getWrappee();

  // Maybe we have to do something more elaborate here later:
  _process.steady_time = 0;
  _process.transport   = nullptr;
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

ClapGain2::ClapGain2(const clap_plugin_descriptor* desc, const clap_host* host)  
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

//-------------------------------------------------------------------------------------------------

const char* const ClapChannelMixer2In3Out::features[6] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_UTILITY, 
  CLAP_PLUGIN_FEATURE_MIXING, 
  CLAP_PLUGIN_FEATURE_MASTERING,
  CLAP_PLUGIN_FEATURE_SURROUND,
  NULL 
};

const clap_plugin_descriptor_t ClapChannelMixer2In3Out::descriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.ChannelMixer2In3Out",
  .name         = "Channel Mixer, 2 In, 3 Out",
  .vendor       = "",
  .url          = "",
  .manual_url   = "",
  .support_url  = "",
  .version      = "0.0.0",
  .description  = "Distribute stereo signal to 3 channels (left, center, right)",
  .features     = ClapChannelMixer2In3Out::features,
};

ClapChannelMixer2In3Out::ClapChannelMixer2In3Out(
  const clap_plugin_descriptor* desc, const clap_host* host)
  : RobsClapHelpers::ClapPluginWithAudio(desc, host)
{
  clap_param_info_flags automatable = CLAP_PARAM_IS_AUTOMATABLE;

  addParameter(kCenterScale, "CenterScale", -1.0, +1.0, 0.0, automatable);
  addParameter(kDiffScale,   "DiffScale",   -1.0, +1.0, 0.0, automatable);
}

bool ClapChannelMixer2In3Out::audioPortsInfo(uint32_t index, bool isInput, 
  clap_audio_port_info* info) const noexcept
{
  if(isInput)
  {
    info->channel_count = 2;    // 2 inputs
    info->id            = 0;    // 
    info->in_place_pair = 0;    // matches id -> allow in-place processing
    info->port_type     = CLAP_PORT_STEREO;
    info->flags         = CLAP_AUDIO_PORT_IS_MAIN;
    strcpy_s(info->name, CLAP_NAME_SIZE, "Stereo In");
  }
  else
  {
    info->channel_count = 3;    // 3 outputs
    info->id            = 0;    // 
    info->in_place_pair = 0;    // matches id -> allow in-place processing
    info->port_type     = nullptr;
    info->flags         = CLAP_AUDIO_PORT_IS_MAIN;
    strcpy_s(info->name, CLAP_NAME_SIZE, "Left/Center/Right Out");
  }

  return true; 

  // Notes:
  //
  // -We allow in place processing, although the number of ins and out differs. We actually don't
  //  care. If the host wants to use 2 of the 3 output channels in in-place mode, it can do so and
  //  it doesn't really matter how the host uses the channels.
  // -I think, the way in-place processing with different numbers of ins and outs should work is:
  //  Let there be m input and n output channels and let k = min(m,n). Then, in-place processing 
  //  means that out[0] == in[0], out[1] == in[1], ..., out[k-1] == in[k-1]. When there are more
  //  ins than outs, i.e. m > n, then not all of the in channels will be used for output, i.e. 
  //  some I/O buffers (from k upwards) remain unmodified and still contain the input after process
  //  returns. When there are more outs then ins, i.e. m < n, then all inputs will be replaced and 
  //  there will also be additional outputs. This is the situation here.
  // -However, although this might be the most obvious way to do in-place processing with m != n, 
  //  we should never depend on that. We should be able to handle other strategies as well. For a
  //  2 in, 2 out plugin, we should also be able to handle weird stuff like out[0] == in[1] and
  //  out[1] == in[0], etc. I actually think, when doing this, my current implementation of the
  //  StereoGainDemo would fail. The general way to do this safely is to just use temp variables. 
  //  Never use any of the outputs for an intermediate result that is used in subsequent output 
  //  compuations - and it doesn't matter if that "intermediate" also happens to be one of the 
  //  outputs as is the case here. ...but wait - that would imply that for an in-place "Bypass" 
  //  plugin, we would actually have to do some copying...hmmm....yeah...well, the example actually
  //  do that: they first do a "fetch the inputs", then "compute", then "write the outputs". Maybe
  //  bring this topic up on KVR or GitHub.
  //
  // ToDo:
  //
  // -Maybe use CLAP_PORT_SURROUND for the output port_type. But that requires the surround 
  //  extension. Can we just assign an arbitrary string? It's a cont char*. Maybe try it.
  // -We may need to set more flags: CLAP_AUDIO_PORT_SUPPORTS_64BITS, 
  //  CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE ...apparently, clap can mix different sample 
  //  formats in a single buffer
  //
}

void ClapChannelMixer2In3Out::parameterChanged(clap_id id, double newValue)
{
  centerScaler = (float) getParameter(kCenterScale);
  diffScaler   = (float) getParameter(kDiffScale);
}

void ClapChannelMixer2In3Out::processSubBlock32(
  const clap_process* p, uint32_t begin, uint32_t end)
{
  // Retrieve pointers:
  const float* inL = &(p->audio_inputs[0].data32[0][0]);
  const float* inR = &(p->audio_inputs[0].data32[1][0]);
  float* outL = &(p->audio_outputs[0].data32[0][0]);
  float* outC = &(p->audio_outputs[0].data32[1][0]);
  float* outR = &(p->audio_outputs[0].data32[2][0]);

  // Do the channel mixing:
  float center;                                   // Temporary to facilitate in-place operation.
  for(uint32_t n = begin; n < end; n++)
  {
    center  = centerScaler * (inL[n] + inR[n]);   // Compute center signal
    outL[n] = outL[n] - diffScaler * center;      // Compute new left signal
    outR[n] = outR[n] - diffScaler * center;      // Compute new right signal
    outC[n] = center;
  }

  // Notes:
  //
  // -The reason why we introduce the temporary "center" variable instead of just assigning outC[n]
  //  as first step and then reading from it again in the computation of outL, outR is that we 
  //  announce in our audioPortsInfo implementation that we allow in-place processing. When the 
  //  host indeed decides to make use of that capability, then we should not write to any output
  //  when we intend to read from it again in the computation of some other output. If the host 
  //  wants to use, for example, outL == inL, outC == inR, outR == something else (which would be 
  //  plausible), then it would not work without the temporary [I think - verify this 
  //  experimenatlly! Make a unit test that tests in-place processing an tempoarily modify the code
  //  to assign outC[n] first and use it instead of "center" to compute outL[n], outR[n]].
  //
  // ToDo:
  //
  // -Maybe check, if the data has the right format - but if we want to do that in every plugin, 
  //  that means a lot of boilerplate
  // -Maybe make a corresponding 3 in, 2 out mixer. It should distribute the center signal equally
  //  to left and right with some gain factor - maybe 0.5 could be appropriate. Figure out the 
  //  required scaling factors for a 2 Ch -> 3 Ch -> 2 Ch roundtrip that ensure an identity 
  //  roundtrip. I guess, there should be a 1-parametric familiy of solutions?
}

void ClapChannelMixer2In3Out::processSubBlock64(
  const clap_process* process, uint32_t begin, uint32_t end)
{
  RobsClapHelpers::clapError("Not yet implemented");
}



/*=================================================================================================

ToDo:

-Write a test function  bool testInPlaceProcessing(clap_plugin* plugin, int numSamples)  that takes 
 a pointer to a plugin and a number of samples for the test buffers. Let the plugin have one input 
 port with m channels and one output port with n channels. Generate an m-channel noise input 
 signal. Record the n-channel output signal using out-of-place buffers. That is the reference 
 signal. Now process the same block with all possible ways in which ins and outs could overlap and
 after each process call, compare the result to the reference signal.



*/