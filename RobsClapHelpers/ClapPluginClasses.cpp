
#include <cassert>

#include "ClapPluginClasses.h"

//=================================================================================================
// class ClapPluginWithParams

bool ClapPluginWithParams::paramsInfo(uint32_t index, clap_param_info* info) const noexcept
{
  if(index >= (uint32_t) infos.size())
  {
    info->min_value     = 0.0;
    info->max_value     = 0.0;
    info->default_value = 0.0;
    info->flags         = 0;
    info->id            = 0;
    info->cookie        = nullptr;
    strcpy_s(info->name,   CLAP_NAME_SIZE, "ERROR! Param index out of range.");
    strcpy_s(info->module, CLAP_PATH_SIZE, "");
    return false;
  }
  else
  {
    *info = infos[index];
    return true;
  }

  // Notes:
  //
  // -If we end up in the first branch where we return false, the plugin will crash due to an 
  //  assert in the outlying wrapper code that asserts that we return true here.
}

bool ClapPluginWithParams::paramsValue(clap_id id, double* value) const noexcept
{
  int index = findParameter(id);
  //assert(index != -1);
  if(index != -1) 
  {
    *value = params[index].value;
    return true; 
  }
  else 
  {
    *value = 0.0;
    return false; 
  }
}

bool ClapPluginWithParams::paramsValueToText(
  clap_id paramId, double value, char* display, uint32_t size) noexcept
{
  sprintf_s(display, size, "%.3f", value);
  return true;

  // ToDo: 
  //
  // -Maybe let the ClapPluginParameter struct have a function pointer member that does the 
  //  conversion to string (by default assigned to some generic double-to-string function that may
  //  just call sprintf_s). Then use that function pointer here. It should be assigned in the 
  //  constructor of ClapPluginParameter
}

bool ClapPluginWithParams::paramsTextToValue(
  clap_id paramId, const char *display, double *value) noexcept
{
  *value = strtod(display, nullptr); // VERIFY!!
  return true;

  // ToDo:
  //
  // -Test this. Bitwig does not seem to allow to enter string values for parameters. Find a host 
  //  that does! ...ahh! With ctrl-click, we can enter a textual value!
  // -Figure out what happens in case of a parse-error. It would be nice to have a  parser that 
  //  returns a bool (true in case fo success, flase in case of failure).
}

bool ClapPluginWithParams::stateSave(const clap_ostream *stream) noexcept
{ 
  std::string stateString = getStateAsString();
  size_t size    = stateString.size();
  size_t written = 0;
  while(written < size)
    written += stream->write(stream, &stateString[written], size-written);
  return true;

  // Notes:
  //
  // -I have no idea, if what I'm doing here is even remotely as intended. It's just guesswork. 
  //  Needs thorough tests
}

bool ClapPluginWithParams::stateLoad(const clap_istream* stream) noexcept 
{ 
  size_t bufSize = 8192;   // May be tweaked - not yet sure what's best here
  //size_t bufSize  = 19;  // May be used for test/debug to actually make it a limiting factor.
  std::string total;       // Total string
  std::string buf;         // Partial string - rename to part..or chunk

  bool endReached = false;
  while(!endReached)
  {
    buf.resize(bufSize);
    size_t numBytes = stream->read(stream, &buf[0], bufSize);
    buf.resize(numBytes);
    total += buf;
    endReached = numBytes == 0;
  }

  return setStateFromString(total);

  // ToDo:
  //
  // -Maybe the bufSize should depend on the number of parameters. Ideally, it should be large 
  //  enough to hold the whole state string but not much larger. We can roughly predict this based
  //  on the number of parameters. If it's shorter, we'll need more calls to stream->read which is 
  //  not the end of the world, but still...
}

void ClapPluginWithParams::addParameter(clap_id id, const std::string& name, double minValue, 
  double maxValue, double defaultValue, clap_param_info_flags flags)
{
  assert(id == (clap_id) params.size());
  // It is currently not supported to add the parameters in arbitrary order. You must add them in 
  // the order of their ids and the ids must start at 0 and use contiguous numbers

  params.push_back(ClapPluginParameter(id, defaultValue));

  clap_param_info info;
  info.min_value     = minValue;
  info.max_value     = maxValue;
  info.default_value = defaultValue;
  info.flags         = flags;
  info.id            = id;
  info.cookie        = nullptr;
  strcpy_s(info.name,   CLAP_NAME_SIZE, name.c_str());
  strcpy_s(info.module, CLAP_PATH_SIZE, "");
  infos.push_back(info);

  // ToDo: 
  //
  // -Make sure that no param with given id exists already
  // -Also, no info with given id should exist already (write a findParamInfo function)
}

int ClapPluginWithParams::findParameter(clap_id id) const
{
  for(int i = 0; i < (int)params.size(); i++) 
  {
    if(params[i].id == id)
      return i; 
  }
  return -1;
}

void ClapPluginWithParams::setParameter(clap_id id, double newValue)
{
  int index = findParameter(id);
  if(index != -1)
    params[index].value = newValue;

  // ToDo:
  //
  // -Maybe get rid of that search - just assume that the id matches the index. I guess, if we have
  //  a lot of automation going on, that simplification can be of great benefit
}

std::string ClapPluginWithParams::getStateAsString() const
{
  std::string s;

  // Store some general info:
  s += "CLAP Plugin State\n\n";
  s += "Identifier: " + std::string(getPluginIdentifier()) + '\n';
  s += "Version: "    + std::string(getPluginVersion())    + '\n';
  s += "Vendor: "     + std::string(getPluginVendor())     + '\n';

  // Store the parameters:
  uint32_t numParams = paramsCount();
  if(numParams > 0)
  {
    clap_param_info info;
    double value = 0.0;
    s += "Parameters: [";
    for(uint32_t i = 0; i < numParams; ++i)
    {
      bool infoOK  = paramsInfo(i, &info);
      assert(infoOK);
      bool valueOK = paramsValue(info.id, &value);
      assert(valueOK);
      s += std::to_string(info.id) + ':';
      s += std::string(info.name)  + ':';   // Maybe store the name optionally
      s += std::to_string(value)   + ',';   // is too imprecise - makes validation fail
    }
    s[s.size()-1] = ']';                    // Replace last comma with closing bracket
  }
   
  return s;

  // ToDo:
  //
  // -Maybe store some optional information like the host with which it was saved
  // -Maybe store the parameter names optionally. Maybe have a "verbose" flag to control this
  // -Maybe use a better double-to-string conversion function. to_string includes trailing zeros 
  //  and I'm also not sure about how many digits it will generally store for parameters which need
  //  very fine resolution
}

bool ClapPluginWithParams::setStateFromString(const std::string& stateStr)
{
  if(stateStr.empty())
    return false;


  // ToDo (IMPORTANT):
  //
  // setAllParamsToDefault();
  //
  // This will be important when a plugin later adds more parameters for which no values are stored
  // in the state. In such cases, we want to set them to their default values. Currently, they will 
  // just be left at whatever values they are currently at - i.e. the state recall will just leave
  // them alone instead of setting to default.

  // Extract the substring that contains the parameters, i.e. anything in between '[' and ']',
  // excluding the opening bracket and including the closing bracket 
  // excluding the brackets themselves:
  size_t start, end;
  start = stateStr.find("Parameters: [", 0) + 13;
  end   = stateStr.find(']', start);
  std::string paramsStr = stateStr.substr(start, end-start+1);
  paramsStr[paramsStr.size()-1] = ',';  
    // Replace closing ']' by ',' to avoid the need for a special case for last param.

  uint32_t numParamsFound = 0;
  size_t i = 0;                            // Index into paramsStr
  while(i < paramsStr.size())
  {
    size_t j = paramsStr.find(':', i+1);
    size_t k = paramsStr.find(':', j+1);
    size_t m = paramsStr.find(',', k+1);

    std::string idStr  = paramsStr.substr(i,   j-i  );
    std::string valStr = paramsStr.substr(k+1, m-k-1);

    clap_id id  = atoi(idStr.c_str());
    double  val = atof(valStr.c_str());    // Maybe use strtod as in the function above
    setParameter(id, val);

    numParamsFound++;
    i = m+1;
  }

  return true;

  // ToDo:
  //
  // -Check if the "Identifier" in the stateString matches our identifier. If not, it means that
  //  the host has called our state recall with the wrong kind of state (or that we changed our 
  //  plugin identifier between save and recall - which we probably should never do)
  // -Detect parse errors and return false in such cases
  // -Check if numParamsFound matches the number of parameters of the plugin. If not maybe it 
  //  means that it's an old state and parameters have been added since then . Maybe we should 
  //  find at least as many params as paramsCount returns. For new parameters, we want to 
  //  initialize them to default values. Maybe call an initParameters or resetParameters or
  //  setParametersToDefaults before doing the actual state recall.
}


std::vector<std::string> ClapPluginWithParams::getFeatures()
{
  const clap_plugin_descriptor* desc = getPluginDescriptor();
  std::vector<std::string> features;
  int i = 0;
  while(desc->features[i] != nullptr)
  {
    features.push_back(std::string(desc->features[i]));
    i++;
  }
  return features;
}


//=================================================================================================
// class ClapPluginStereo32Bit

bool ClapPluginStereo32Bit::audioPortsInfo(
  uint32_t index, bool isInput, clap_audio_port_info* info) const noexcept
{
  info->channel_count = 2;    // Doesn't matter if input or output. The answer is always 2.
  info->id            = 0;    // We can assign an id to a port based on the index (?)
  info->in_place_pair = 0;    // If this matches the id, in-place processing is allowed
  info->port_type     = CLAP_PORT_STEREO;
  info->flags         = CLAP_AUDIO_PORT_IS_MAIN;

  // Write the port names:
  if(isInput) strcpy_s(info->name, CLAP_NAME_SIZE, "Stereo In");
  else        strcpy_s(info->name, CLAP_NAME_SIZE, "Stereo Out");
  return true; 

  // Notes:
  //
  // -To support 64 bit processing, we would need to use CLAP_AUDIO_PORT_SUPPORTS_64BITS with 
  //  info->flags. Presumably, if we do not set this flag, our audio process function will never be
  //  called with 64 bit buffers - right?
}

clap_process_status ClapPluginStereo32Bit::process(const clap_process *p) noexcept
{
  // Check number of input and output ports/busses:
  if(  p->audio_inputs_count  != 1 
    || p->audio_outputs_count != 1)
    return CLAP_PROCESS_ERROR;

  // Check number of input and output channels:
  if(  p->audio_inputs[0].channel_count  != 2 
    || p->audio_outputs[0].channel_count != 2)
    return CLAP_PROCESS_ERROR;

  // Check that we are asked for doing single precision:
  if(isDoublePrecision(p))
    return CLAP_PROCESS_ERROR;


  // Process the sub-blocks with interleaved event handling:
  const uint32_t numFrames      = p->frames_count;
  const uint32_t numEvents      = p->in_events->size(p->in_events);
  uint32_t       eventIndex     = 0;
  uint32_t       nextEventFrame = numEvents > 0 ? 0 : numFrames;
  uint32_t       frameIndex     = 0;
  while(frameIndex < numFrames)
  {
    // Handle all events that happen at the frame i:
    while(eventIndex < numEvents && nextEventFrame == frameIndex) {
      const clap_event_header_t *hdr = p->in_events->get(p->in_events, eventIndex);
      if (hdr->time != frameIndex) {
        nextEventFrame = hdr->time; 
        break;  
      }
      processEvent(hdr);              // Handle the event
      ++eventIndex;
      if(eventIndex == numEvents) {
        nextEventFrame = numFrames; 
        break;                        // We reached the end of the event list
      } 
    }

    // Process the sub-block until the next event:
    processBlockStereo(
      &p->audio_inputs[0].data32[0][frameIndex],   
      &p->audio_inputs[0].data32[1][frameIndex],
      &p->audio_outputs[0].data32[0][frameIndex],  
      &p->audio_outputs[0].data32[1][frameIndex],
      nextEventFrame - frameIndex);
    frameIndex += nextEventFrame;
  }
  // This code needs verification. I copy/pasted it from  plugin-template.c  and made some edits
  // which I think, are appropriate. But it needs to be verified. Maybe create a test project
  // That does some unit tests. Also compare the code to the nakst example. It gives reasonable
  // results in Bitwig though - but that doesn't replace unit testing.

  return CLAP_PROCESS_CONTINUE;

  // Notes:
  //
  // -Returning CLAP_PROCESS_CONTINUE_IF_NOT_QUIET is another option that might be suitable for 
  //  certain kinds of processors. If you want to do this in your subclass, just override process,
  //  call this baseclass method here and return whatever other return value you want to return to 
  //  the host
}

void ClapPluginStereo32Bit::processEvent(const clap_event_header_t* hdr)
{
  if(hdr->type == CLAP_EVENT_PARAM_VALUE)
  {
    const clap_event_param_value* paramValueEvent = (const clap_event_param_value*) hdr;

    clap_id param_id = paramValueEvent->param_id;
    double  value    = paramValueEvent->value;
    //void*   cookie   = paramValueEvent->cookie; // We currently don't use the cookie facility.
    setParameter(param_id, value);
  }

  // Notes:
  //
  // -If you want to handle other kinds of events that we ignore here, you can override the 
  //  processEvent method, implement your handling for the other kinds of events and if the event 
  //  is not of that kind, just call this basesclass method to get the default behavior for the 
  //  event types that we handle here.
}

//=================================================================================================

/*

ToDo:

-To lift the restriction in ClapPluginParameter that the ids must match the array indices, maybe
 introduce a bool member variable "idMatchesIndex" that swaps between those two modes. When id and
 index match, parameter lookup is greatly simplified because we do not need to search for the id.
 On the other hand, allowing a mismatch allows us to change the order of the parameters later 
 without breaking state recall.


*/