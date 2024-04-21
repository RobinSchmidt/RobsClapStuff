
//=================================================================================================
// class ClapPluginWithParams

ClapPluginWithParams::~ClapPluginWithParams()
{
  for(size_t i = 0; i < formatters.size(); i++)
    delete formatters[i];
}

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
  // -Bitwig seems to call this function twice for each parameter when the plugin is plugged in 
  //  and also once for each parameter when the plugin is plugged out. I would have expected it to
  //  get called once for each parameter when the plugin is plugged in.
}

bool ClapPluginWithParams::paramsValue(clap_id id, double* value) const noexcept
{
  if(id < values.size())
  {
    *value = values[id];
    return true; 
  }
  else
  {
    *value = 0.0;
    return false; 
  }
}

bool ClapPluginWithParams::paramsValueToText(
  clap_id id, double value, char* display, uint32_t size) noexcept
{
  if(formatters[id] != nullptr)
    return formatters[id]->valueToText(value, display, size);
  else
    return toDisplay(value, display, size, 2);

  // ToDo: 
  //
  // -Remove the "if...". The pointers should never be nullptr. We should *always* assign some 
  //  valid object. Delete the toDisplay function. It's obsolete when we have the system with the 
  //  formatter finished
  // -Rename "display" to "text"
}

bool ClapPluginWithParams::paramsTextToValue(
  clap_id id, const char *display, double *value) noexcept
{
  //// Old:
  //*value = strtod(display, nullptr);
  //return true;

  // New:
  if(formatters[id] != nullptr)
    return formatters[id]->textToValue(display, value);
  else
  {
    *value = strtod(display, nullptr);
    return true;
  }

  // ToDo:
  //
  // -Like in ...ValueToText: remove "if..", rename "display"
}

void ClapPluginWithParams::paramsFlush(
  const clap_input_events* in, const clap_output_events* out) noexcept
{
  const uint32_t numEvents = in->size(in);
  for(uint32_t i = 0; i < numEvents; ++i)
  {
    const clap_event_header_t *hdr = in->get(in, i);
    processEvent(hdr);
  }

  // Notes:
  //
  // -I'm not sure if we should perhaps apply a sort of event filter here, i.e. call processEvent 
  //  only when the event is a parameter change and ignore other kinds of events (like note-events,
  //  etc.). I guess we will not receive any other kind of event here anyway and if we do, it 
  //  probably counts as host misbehavior? Figure out!
}

bool ClapPluginWithParams::stateSave(const clap_ostream *stream) noexcept
{ 
  std::string stateString = getStateAsString();
  size_t size    = stateString.size();
  size_t written = 0;
  while(written < size)
    written += stream->write(stream, &stateString[written], size-written);
  return true;

  // ToDo:
  //
  // -Maybe factor out the string-to-stream writing into a function 
  //  writeStringToStream(const clap_ostream *stream, const string& str). Likewise
  //  for readStreamToString(const clap_istream* stream) which should return a string
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
  double maxValue, double defaultValue, clap_param_info_flags flags, ValueFormatter* formatter)
{
  // Add a new clap_param_info to the end of our infos array:
  clap_param_info info;
  info.min_value     = minValue;
  info.max_value     = maxValue;
  info.default_value = defaultValue;
  info.flags         = flags;
  info.id            = id;
  info.cookie        = nullptr;                   // We currently don't use the cookie facility.
  strcpy_s(info.name,   CLAP_NAME_SIZE, name.c_str());
  strcpy_s(info.module, CLAP_PATH_SIZE, "");
  infos.push_back(info);
  // Factor out into a private pushParamInfo(id, name, minValue, maxValue, defaulValue, flags)

  // Adjust the size of values array if needed and initialize the new parameter with its default
  // value:
  size_t newSize = std::max((size_t) id+1, values.size());
  values.resize(newSize);
  values[id] = defaultValue;

  formatters.resize(newSize);

  // Old:
  //formatters[id] = nullptr;

  // New:
  if(formatter != nullptr)
    formatters[id] = formatter;
  else
    formatters[id] = new ValueFormatter;



  // ToDo: 
  //
  // -Instead of a single addParameter function have 2. One for numerical params that are to be
  //  displayed with precision and suffix and one for choice params. Maybe use:
  //  addFloatParameter, addChoiceParameter - just like juce::AudioProcessor
  // -But maybe keep this function and augment it with a parameter of type 
  //  pointer-to-ValueFormatter and instead of assigning a nullptr, use the passed object.
}

void ClapPluginWithParams::addFloatParameter(clap_id id, const std::string& name, 
  double minVal, double maxVal, double defaultVal, clap_param_info_flags flags, 
  int precision, const std::string& suffix)
{
  addParameter(id, name, minVal, maxVal, defaultVal, flags, 
    new ValueFormatterWithSuffix(precision, suffix));
}

void ClapPluginWithParams::addChoiceParameter(clap_id id, const std::string& name, 
  double minVal, double maxVal, double defaultVal, clap_param_info_flags flags,
  const std::vector<std::string>& choices)
{
  addParameter(id, name, minVal, maxVal, defaultVal, flags, 
    new ValueFormatterForChoice(choices));


  // ToDo:
  //
  // -We should perhaps automatically add the flas CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM. For
  //  choice parameters, these are always needed
}

void ClapPluginWithParams::setParameter(clap_id id, double newValue)
{
  if((size_t) id < values.size())
  {
    values[id] = newValue;
    parameterChanged(id, newValue);
  }
  else
  {
    //clapAssert(false);  // Host tries to set a parameter with invalid id.
  }

  // ToDo:
  //
  // -Maybe add assertions that the newValue is within the allowed range. And/or maybe clip it to
  //  the allowed range. But that would require to figure out the range which would require to look
  //  up where the corresponding parameter info is stored in our infos array which would currently
  //  be O(numParams) - we want setParameter to be O(1). We could make that lookup O(1) though by
  //  bringing in the IndexIdentifierMap. ...but that doesn't seem justified for the purpose of 
  //  clipping.
}

double ClapPluginWithParams::getParameter(clap_id id) const
{
  if((size_t) id < values.size())
    return values[id];
  else
  {
    //clapAssert(false);  // Host tries to retrieve a parameter with invalid id.
    return 0.0;
  }
}

void ClapPluginWithParams::setAllParametersToDefault()
{
  for(size_t i = 0; i < infos.size(); ++i)
    setParameter(infos[i].id, infos[i].default_value);
}

bool ClapPluginWithParams::areParamsConsistent()
{
  if(infos.size() != values.size())
    return false;

  // Check, if each id in 0...numParams-1 occurs exactly once in our infos array:
  for(size_t i = 0; i < infos.size(); i++)
  {
    clap_id id = (clap_id) i;
    size_t cnt = std::count_if(infos.begin(), infos.end(),
                               [&](const clap_param_info& info){ return info.id == id; });
    if(cnt != 1)
      return false;
  }

  return true;

  // ToDo:
  //
  // -Check, if this is the right capture mode for the lambda
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
      clapAssert(infoOK);
      bool valueOK = paramsValue(info.id, &value);
      clapAssert(valueOK);
      s += std::to_string(info.id) + ':';
      s += std::string(info.name)  + ':';   // Maybe store the name optionally
      s += toStringExact(value)    + ',';   // Roundtrip-safe double-to-string conversion
    }
    s[s.size()-1] = ']';                    // Replace last comma with closing bracket
  }
   
  return s;

  // Notes:
  //
  // -We use a custom double-to-string conversion function to ensure roundtrip safety, i.e. exact
  //  restoring of the number when the string is parsed in state recall. Using std::to_string with
  //  floats or doubles just isn't good enough for that.
  //
  // ToDo:
  //
  // -Maybe store some optional information like the host with which it was saved
  // -Maybe store the parameter names optionally. Maybe have a "verbose" flag to control this. But
  //  this will also complicate the implementation of setStateFromString - so maybe don't.
  // -Maybe give the function a bool parameter "includeDefaultValues" or "skipDefaultValues" that 
  //  decides whether or not the state string should also store parameter values when they are at 
  //  their default value. But this has the problem of potential false state recall when default
  //  values are changed later - so maybe don't.
}

bool ClapPluginWithParams::setStateFromString(const std::string& stateStr)
{
  setAllParametersToDefault();

  if(stateStr.empty())
    return false;

  // Extract the substring that contains the parameters, i.e. anything in between '[' and ']',
  // excluding the opening bracket and including the closing bracket. The closing bracket ']' is 
  // included and then replaced by a comma ',' to avoid the need for a special case for last 
  // parameter in the subsequent loop:
  size_t start, end;
  start = stateStr.find("Parameters: [", 0) + 13;
  end   = stateStr.find(']', start);
  std::string paramsStr = stateStr.substr(start, end-start+1);
  paramsStr[paramsStr.size()-1] = ',';         // Replace closing bracket with comma to avoid 
                                               // special case for last parameter

  uint32_t numParamsFound = 0;
  size_t i = 0;                                // Index into paramsStr
  while(i < paramsStr.size())
  {
    size_t j = paramsStr.find(':', i+1);
    size_t k = paramsStr.find(':', j+1);
    size_t m = paramsStr.find(',', k+1);

    std::string idStr  = paramsStr.substr(i,   j-i  );
    std::string valStr = paramsStr.substr(k+1, m-k-1);

    clap_id id  = std::atoi(idStr.c_str());
    double  val = std::atof(valStr.c_str());   // Maybe use strtod as in the function above?
    setParameter(id, val);

    numParamsFound++;
    i = m+1;
  }

  return true;

  // Notes:
  //
  // -The call to setAllParametersToDefault() at the beginning is important when the state string 
  //  does not contain values for all of our parameters. This may happen if the state was created 
  //  with an older version of the plugin that did not yet have certain parameters because they 
  //  were added later. In such a case, the parameters which have no value in the state should be 
  //  set to their default value. Without the call at the beginning, they would just be left at 
  //  whatever values they are currently at - which is wrong behavior.
  //
  // ToDo:
  //
  // -Check if the "Identifier" in the stateString matches our identifier. If not, it means that
  //  the host has called our state recall with the wrong kind of state (or that we changed our 
  //  plugin identifier between save and recall - which we probably should never do)
  // -Detect parse errors and return false in such cases
}

void ClapPluginWithParams::processEvent(const clap_event_header_t* hdr)
{
  if(hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
    return;

  if(hdr->type == CLAP_EVENT_PARAM_VALUE)
  {
    const clap_event_param_value* paramValueEvent = (const clap_event_param_value*) hdr;

    const clap_id param_id = paramValueEvent->param_id;
    const double  value    = paramValueEvent->value;
    //const void* cookie   = paramValueEvent->cookie; // We currently don't use the cookie facility
    setParameter(param_id, value);
  }

  // Notes:
  //
  // -If you want to handle other kinds of events that we ignore here, you can override the 
  //  processEvent method, implement your handling for the other kinds of events and if the event 
  //  is not of that kind, just call this basesclass method to get the default behavior for the 
  //  event types that we handle here.
  // -The first check of the space_id is required to make the validator pass all tests and it is
  //  also what the official plugin-template.c and the nakst example do. I don't really know the
  //  purpose of that, though. What is an "event space" anyhow? What other event spaces besides the
  //  "core event space" exist? -> Figure out and document!
  //
  // ToDo:
  //
  // -When we handle more types of events, we should use a switch statement. See the 
  //  plugin-template.c. The nakst example also uses an if statement, though.
}

//=================================================================================================
// class ClapPluginWithAudio

clap_process_status ClapPluginWithAudio::process(const clap_process* p) noexcept
{
  bool useFloat64 = isDoublePrecision(p);

  // Process the sub-blocks with interleaved event handling:
  const uint32_t numFrames      = p->frames_count;
  const uint32_t numEvents      = p->in_events->size(p->in_events);
  uint32_t       frameIndex     = 0;
  uint32_t       eventIndex     = 0;
  uint32_t       nextEventFrame = numEvents > 0 ? 0 : numFrames;
  while(frameIndex < numFrames)
  {
    // Handle all events that happen at the current frame:
    handleProcessEvents(p, frameIndex, numFrames, eventIndex, numEvents, nextEventFrame);

    // Process the sub-block until the next event. This is a call to the overriden implementation
    // in the subclass in a sort of "template method" pattern:
    if(useFloat64)
      processSubBlock64(p, frameIndex, nextEventFrame);
    else
      processSubBlock32(p, frameIndex, nextEventFrame);

    // Advance frameIndex by the length of the sub-block that we just processed:
    frameIndex += nextEventFrame - frameIndex;
  }

  return CLAP_PROCESS_CONTINUE;
}

void ClapPluginWithAudio::handleProcessEvents(const clap_process* p, uint32_t frameIndex, 
  uint32_t numFrames, uint32_t& eventIndex, uint32_t numEvents, uint32_t& nextEventFrame)
{
  // This event handling code originates from  plugin-template.c  from the CLAP repo:
  while(eventIndex < numEvents && nextEventFrame == frameIndex) 
  {
    const clap_event_header_t *hdr = p->in_events->get(p->in_events, eventIndex);
    if(hdr->time != frameIndex) 
    {
      nextEventFrame = hdr->time; 
      break;  
    }
    processEvent(hdr);              // Call to virtual event-handler function
    ++eventIndex;
    if(eventIndex == numEvents) 
    {
      nextEventFrame = numFrames; 
      break;                        // We reached the end of the event list
    } 
  }

  // Notes:
  //
  // -The call to virtual function processEvent() is where the actual event handling action 
  //  happens. Unless subclasses override processEvent, the implementation from our baseclass
  //  ClapPluginWithParams will be called. This implementation handles only parameter-change events
  //  and the handling will spawn a callback to parameterChanged(). If subclasses wnat to handle 
  //  other types of events as well, they will need to override processEvent.
  // -This event handling code had been adapted from plugin-template.c from the CLAP repo
}

void ClapPluginWithAudio::processSubBlock32(const clap_process* p, uint32_t begin, uint32_t end)
{
  // For 1 input and 1 output port with the same number of channels, the code to simply copy the 
  // data from input to output buffer could look something like:
  //
  // uint32_t numChannels = p->audio_inputs[0].channel_count;
  // clapAssert(numChannels == p->audio_outputs[0].channel_count);
  // for(uint32_t c = 0; c < numChannels; c++)
  //   for(uint32_t n = begin; n < end; n++)
  //     p->audio_outputs[0].data32[c][n] = p->audio_inputs[0].data32[c][n];
  //
  // ToDo: check, if this code actually works
}

void ClapPluginWithAudio::processSubBlock64(const clap_process* p, uint32_t begin, uint32_t end)
{
  // The exact same considerations as for processSubBlock32 apply just that here you would use
  // the .data64[c][n] buffers instead of .data32[c][n] and do all the DSP in double precision.
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
  //
  // ToDo:
  //
  // -I think strcpy_s is a Microsoft compiler specific function? Figure out! If so, replace it to 
  //  make the code portable.
  // -Maybe remove the spaces in "Stereo In" etc. In the clap-saw-demo.
  // -ClapSaw demo uses an id of 1 for its 1st note port but an id of 0 fro ist 1st audio port:
  //  https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.cpp#L303
}

clap_process_status ClapPluginStereo32Bit::process(const clap_process *p) noexcept
{
  //auto inEvents = RobsClapHelpers::extractInEvents(p); // For inspection in debugger

  // Catch situations where the host asks us to process a buffer with an unsupported config:
  if(!isProcessConfigSupported(p))
    return CLAP_PROCESS_ERROR;

  // Process the sub-blocks with interleaved event handling:
  const uint32_t numFrames      = p->frames_count;
  const uint32_t numEvents      = p->in_events->size(p->in_events);
  uint32_t       frameIndex     = 0;
  uint32_t       eventIndex     = 0;
  uint32_t       nextEventFrame = numEvents > 0 ? 0 : numFrames;
  while(frameIndex < numFrames)
  {
    // Handle all events that happen at the current frame:
    handleProcessEvents(p, frameIndex, numFrames, eventIndex, numEvents, nextEventFrame);

    // Process the sub-block until the next event. This is a call to the overriden implementation
    // in the subclass in a sort of "template method" pattern:
    uint32_t subBlockLength = nextEventFrame - frameIndex;
    processBlockStereo(
      &p->audio_inputs[0].data32[0][frameIndex],
      &p->audio_inputs[0].data32[1][frameIndex],
      &p->audio_outputs[0].data32[0][frameIndex],
      &p->audio_outputs[0].data32[1][frameIndex],
      subBlockLength);
    frameIndex += subBlockLength;
  }

  return CLAP_PROCESS_CONTINUE;

  // Notes:
  //
  // -Returning CLAP_PROCESS_CONTINUE_IF_NOT_QUIET is another option that might be suitable for 
  //  certain kinds of processors. If you want to do this in your subclass, just override process,
  //  call this baseclass method here and return whatever other return value you want to return to 
  //  the host
  // -Maybe, if the number of inputs or outputs is less than 1, we should not return 
  //  CLAP_PROCESS_ERROR but rather CLAP_PROCESS_SLEEP like here:
  //  https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.cpp#L349
  // -We could actually also have implemented 
}



//=================================================================================================

bool ClapSynthStereo32Bit::notePortsInfo(
  uint32_t index, bool isInput, clap_note_port_info* info) const noexcept
{
  if(isInput)
  {
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
    info->preferred_dialect  = CLAP_NOTE_DIALECT_CLAP;
    strcpy_s(info->name, CLAP_NAME_SIZE, "Note In");
    //strncpy(info->name, "Note In", CLAP_NAME_SIZE);  // From clap-saw-demo - compile error in VS
    return true;
  }
  return false;

  // ToDo:
  //
  // -Maybe support MIDI 2.0 later
  // -Maybe use midi as preferred dialect...not sure about this, though
  // -Figure out if there is a reason why clap-saw-demo uses 1 rather than 0 for the id, see here:
  //  https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.h#L152
  //  https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.cpp#L318
  //  Using 1-based indexing for the note-ports is inconsistent with the 0-based indexing of the 
  //  audio ports. If not, use 0. I guess, it doesn't matter - it's probably up to the plugin how 
  //  to specify its ids just like with the parameter ids. But I guess, the assigned ids must 
  //  remain stable in updates. Zero seems to work in Bitwig, so unless there's a good reason to 
  //  use 1, I'd rather use 0.
}

void ClapSynthStereo32Bit::handleMidiEvent(const uint8_t data[3])
{
  // Convert all messages to corresponding messages on MIDI channel 1:
  uint8_t status = data[0] & 0xf0;

  if( status == 0x90 || status == 0x80 )
  {
    // Respond to note-on and note-off on channel 1

    // Bitwise and with 0x7f=01111111 sets the first bit of an 8-bit word to zero, allowing
    // only numbers from 0 to 127 to pass unchanged, higher numbers (128-255) will be mapped
    // into this range by subtracting 128:
    uint8_t key = data[1] & 0x7f;
    uint8_t vel = data[2] & 0x7f;

    // Call noteOn or noteOff which subclasses will override:
    if(status == 0x80 || vel == 0)    // 0x80: "proper" note-off, vel=0: "running status" note-off
      noteOff((int)key);
    else
      noteOn((int)key, (double)vel / 127.0);
  }
  else if( (status) == 0xb0 && (data[1] == 0x7b) )
  {
    // Respond to "all notes off" event:
    // (status=0xb0: controller on ch 1, midiData[1]=0x7b: control 123: all notes off):
    for(int i = 0; i <= 127; i++)
      noteOff(i);
  }

  // Notes:
  //
  // -The code for the midi event handling was adopted from Open303VST::handleEvent.
  // -Subclasses can override this function if they want to to handle more types of midi messages
  //
  // ToDo:
  //
  // -Maybe implement responses to control-change and pitch-wheel events by calling hooks that 
  //  subclasses can override - just like with noteOn/Off - but provide empty default 
  //  implementations for those other event handlers. Subclasses may want ignore these types of 
  //  events. See Open303VST::handleEvent for how to identify these events and convert the data 
  //  into more easily digestible formats.
  // -Maybe handle all-notes-off events by a different callback/hook like allNotesOff where the 
  //  default implementation just does what we do above but subclasses can override it. Rationale:
  //  the "all-notes-off" event is (I think) some sort of more aggressive "reset" command like 
  //  "midi-panic" (if I remember correctly), so subclasses may want to handle such an event in
  //  a more aggressive way - like making a full blown midi status reset or something. Such events
  //  are typically used to deal with hanging notes.
  // -We should probably not convert all messages to channel-1 messages. That's good enough for my
  //  personal use cases but for a published library, we may want to retain the channel info.
  // -Implement unit tests to test the midi responses.
}

void ClapSynthStereo32Bit::processEvent(const clap_event_header_t* hdr)
{
  if(hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
    return;

  switch(hdr->type)
  {
  case CLAP_EVENT_NOTE_ON: {
    const clap_event_note* note = (const clap_event_note*) hdr;
    noteOn(note->key, note->velocity);
  } break;
  case CLAP_EVENT_NOTE_OFF: {
    const clap_event_note* note = (const clap_event_note*) hdr;
    noteOff(note->key);
  } break;
  case CLAP_EVENT_MIDI: {
    const clap_event_midi* midi = (const clap_event_midi*)(hdr);
    handleMidiEvent(midi->data); // midi->data is the array of 3 midi bytes.
  } break;
  default: {
    Base::processEvent(hdr);     // Baseclass implementation handles parameter changes.
  }
  }

  // Notes:
  //
  // -Note-on/off events may arrive in different flavors: CLAP_EVENT_NOTE_ON/OFF, 
  //  CLAP_EVENT_MIDI, CLAP_EVENT_MIDI2. We need to be able to handle the ..._NOTE_ON/OFF and the 
  //  _MIDI variants because we said that we support these "dialects" in our implementation of 
  //  notePortsInfo(). We didn't say to support MIDI2, so we ignore these types of events here.
  // -There are also NOTE_CHOKE and NOTE_END event types. 
  // -If I get it right, the NOTE_CHOKE event is meant for ducking a voice by another voice like in
  //  mutually exclusive sounds like open and closed hihats in a drum machine. But it will also be 
  //  sent when the user double-clicks on the stop button in the DAW which should stop all sounds 
  //  immediately (like in "midi-panic"?). Soo...maybe we should respond to such choke events by
  //  a hard switch-off and reset?
  // -The NOTE_END event is an event type to be sent from the plugin to the host to inform the host
  //  that a note has ended such that the host can turn off any modulators for that voice. Maybe we
  //  should have a member function noteEnded/reportNoteEndToHost/notifyHostNoteEnded that 
  //  subclasses can call and such a call will have the effect of scheduling sending of such 
  //  note-end events to the host?
  //
  // See also:
  //
  // https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.cpp#L492
}