

// The given line numbers above the functions give the lines where the corresponding function is
// defined in clap/helpers/plugin.hxx on which this wrapper is based.


// Static class members (maybe move to bottom):

// Line 47, audio ports
const clap_plugin_audio_ports ClapPlugin::_pluginAudioPorts = 
{
  clapAudioPortsCount, 
  clapAudioPortsInfo 
};

const clap_plugin_state ClapPlugin::_pluginState = 
{
  clapStateSave,
  clapStateLoad
};

// Line 84
const clap_plugin_latency ClapPlugin::_pluginLatency = 
{
  clapLatencyGet
};

// Line 64:
const clap_plugin_params ClapPlugin::_pluginParams = 
{
  clapParamsCount,
  clapParamsInfo,
  clapParamsValue,
  clapParamsValueToText,
  clapParamsTextToValue,
  clapParamsFlush
};

// Line 89:
const clap_plugin_note_ports ClapPlugin::_pluginNotePorts = 
{
  clapNotePortsCount,
  clapNotePortsInfo
};


//-------------------------------------------------------------------------------------------------
// Constructor:

ClapPlugin::ClapPlugin(const clap_plugin_descriptor* desc, const clap_host* host) : _host(host)
{
  // Set the data members of the C-struct:
  _plugin.plugin_data      = this;
  _plugin.desc             = desc;

  // Set the function pointers in the C-struct:
  _plugin.init             = ClapPlugin::clapInit;
  _plugin.destroy          = ClapPlugin::clapDestroy;
  _plugin.get_extension    = ClapPlugin::clapExtension;
  _plugin.process          = ClapPlugin::clapProcess;
  _plugin.activate         = ClapPlugin::clapActivate;
  _plugin.deactivate       = ClapPlugin::clapDeactivate;
  _plugin.start_processing = ClapPlugin::clapStartProcessing;
  _plugin.stop_processing  = ClapPlugin::clapStopProcessing;
  _plugin.reset            = ClapPlugin::clapReset;
  _plugin.on_main_thread   = ClapPlugin::clapOnMainThread;

  // desc->name = "Test"; 
  // Nope! We are not supposed to fill out the descriptor here - it's const. So - what are we 
  // supposed to do with it? Just store it away and that's it? Or may we should check it - if the 
  // host requests the right kind of plugin tobe created? Maybe we can check the version number and
  // later do state conversion, if necessary (if the version of the state corresponds to an older
  // version of the plugin? But where should we fill out the descriptor then? Hmm....in 
  // create-an-actual-plugin.cc there is getDescription()
}

//-------------------------------------------------------------------------------------------------
// Basic clap plugin callbacks:

bool ClapPlugin::clapInit(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin, false);
  assert(!self._wasInitialized);   // init is supposed to be done only once
  self._wasInitialized = true;

  //self._host.init();  
  // doesn't compile - probably because I have replaced the HostProxy with a regular clap_host
  // -> Figure out what this call to HostProxy::init is supposed to do

  self.ensureMainThread("clap_plugin.init");
  return self.init();
}

// Line 203
void ClapPlugin::clapDestroy(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin, false);
  self.ensureMainThread("clap_plugin.destroy");
  self._isBeingDestroyed = true;
  if(self._isGuiCreated) 
  {
    assert(false);    // Host forgot to destroy the gui
    //clapGuiDestroy(plugin);  // Uncomment later again - maybe implement an empty stub
  }
  if(self._isActive) 
  {
    assert(false);    // Host forgot to deactivate before destroying
    clapDeactivate(plugin);
  }
  assert(!self._isActive);
  //self.runCallbacksOnMainThread();   // What is this supposed to do?
  delete &self;
}
// Supposed to be called on the main thread after calling clapDeactivate. Maybe we should also 
// include a virtual member function destroy and call self.destroy before calling delete &self?

// Line 440
const void* ClapPlugin::clapExtension(const clap_plugin *plugin, const char *id) noexcept 
{
  auto &self = from(plugin);

  self.ensureInitialized("extension");
  if(!strcmp(id, CLAP_EXT_STATE)       && self.implementsState())      return &_pluginState;
  if(!strcmp(id, CLAP_EXT_LATENCY)     && self.implementsLatency())    return &_pluginLatency;
  if(!strcmp(id, CLAP_EXT_AUDIO_PORTS) && self.implementsAudioPorts()) return &_pluginAudioPorts;
  if(!strcmp(id, CLAP_EXT_PARAMS)      && self.implementsParams())     return &_pluginParams;
  if(!strcmp(id, CLAP_EXT_NOTE_PORTS)  && self.implementsNotePorts())  return &_pluginNotePorts;

  return self.extension(id);
}
// ToDo: explain what this function does and how it is used.


// Line 417:
clap_process_status ClapPlugin::clapProcess(
  const clap_plugin *plugin, const clap_process *process) noexcept 
{
  auto &self = from(plugin);

  self.ensureInitialized("process");
  self.ensureAudioThread("clap_plugin.process");
  assert(self._isActive);
  assert(self._isProcessing);
  if(! (self._isActive && self._isProcessing) )
    return CLAP_PROCESS_ERROR;

  return self.process(process);
}
// Called on the audio thread


// Line 263:
bool ClapPlugin::clapActivate(const clap_plugin *plugin, double sample_rate, 
  uint32_t minFrameCount, uint32_t maxFrameCount) noexcept 
{
  auto &self = from(plugin);

  // Sanity checks:
  self.ensureInitialized("activate");
  self.ensureMainThread("clap_plugin.activate");
  assert(!self._isActive);                // Before activation, we should be in inactive state
  assert(self._sampleRate == 0);          // Sample rate member should be 0 in inactive state
  assert(sample_rate > 0);                // New sample rate should be > 0
  assert(minFrameCount >= 1);             // Block size should be at least 1 sample
  assert(maxFrameCount <= INT32_MAX);     // Block size should not exceed 2^32-1 (verify!)
  assert(minFrameCount <= maxFrameCount); // Block size min must be <= max

  // Call the actual activation function. Depending on the type of plugin, this may take some time 
  // (buffers may need to be allocated, lookup tables generated etc.), so we have a flag that is 
  // set to true dureing this process:
  self._isBeingActivated = true;
  if(!self.activate(sample_rate, minFrameCount, maxFrameCount)) 
  {
    // The activation has failed. Set us back into not-being-activated state and report failure:
    self._isBeingActivated = false;
    assert(!self._isActive);
    assert(self._sampleRate == 0);
    return false;                         
  }
  self._isBeingActivated = false;

  // Update state and report success:
  self._isActive = true;
  self._sampleRate = sample_rate;
  return true;
}

// Line 337:
void ClapPlugin::clapDeactivate(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);
  self.ensureInitialized("deactivate");
  self.ensureMainThread("clap_plugin.deactivate");
  assert(self._isActive);      // Plugin was deactivated twice
  if(!self._isActive)
    return;
  self.deactivate();
  self._isActive = false;
  self._sampleRate = 0;
}

// Line 355. Supposed to be called after activate but before process.
bool ClapPlugin::clapStartProcessing(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);

  self.ensureInitialized("start_processing");
  self.ensureAudioThread("clap_plugin.start_processing");

  assert(self._isActive);       // Plugin hsould be activated before starting processing
  assert(!self._isProcessing);  // clap_plugin.start_processing() was called twice
  if(self._isProcessing)
    return true;

  self._isProcessing = self.startProcessing();
  return self._isProcessing;
}

// Line 378
void ClapPlugin::clapStopProcessing(const clap_plugin *plugin) noexcept 
{

  auto &self = from(plugin);

  self.ensureInitialized("stop_processing");
  self.ensureAudioThread("clap_plugin.stop_processing");
  assert(self._isActive);     // Host called stop_processing() on a deactivated plugin
  assert(self._isProcessing); // Host called stop_processing() twice

  self.stopProcessing();
  self._isProcessing = false;
}

// Line 401
void ClapPlugin::clapReset(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);

  self.ensureInitialized("reset");
  self.ensureAudioThread("clap_plugin.reset");
  assert(self._isActive); // Host called clap_plugin.reset() on a deactivated plugin

  self.reset();
}

// Line 227
void ClapPlugin::clapOnMainThread(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);
  self.ensureInitialized("on_maint_thread");
  self.ensureMainThread("clap_plugin.on_main_thread");
  //self.runCallbacksOnMainThread();  // What is this?
  self.onMainThread();                // ...and this?
}
// What is the purpose of this? In plugin.h in clap-main/include/clap the doc says about
// on_main_thread: "Called by the host on the main thread in response to a previous call to:
// host->request_callback(host);" ...soo - that means if we, inside our plugin, somewhere call
// _host->request_callback(); then onMainThread() will be called? What is the purpose of this?
// Maybe that the audio thread can trigger some updates of the gui via this mechanism?

//-------------------------------------------------------------------------------------------------
// Callbacks for extensions:


// Line 674, audio ports
uint32_t ClapPlugin::clapAudioPortsCount(const clap_plugin *plugin, bool is_input) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_audio_ports.count");
  return self.audioPortsCount(is_input);
}
bool ClapPlugin::clapAudioPortsInfo(const clap_plugin *plugin, uint32_t index, bool is_input,
  clap_audio_port_info *info) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_audio_ports.info");
  uint32_t count = clapAudioPortsCount(plugin, is_input);
  assert(index < count);    // Index out of range
  if(index >= count) 
    return false;
  return self.audioPortsInfo(index, is_input, info);
}

// Line 587, State load/save
bool ClapPlugin::clapStateSave(const clap_plugin *plugin, const clap_ostream *stream) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_state.save");
  return self.stateSave(stream);
}
bool ClapPlugin::clapStateLoad(const clap_plugin *plugin, const clap_istream *stream) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_state.load");
  return self.stateLoad(stream);
}

// Line 507, report latency to host:
uint32_t ClapPlugin::clapLatencyGet(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_latency.get");
  assert(self._isActive);        // Sample rate unknown if plug not active
  return self.latencyGet();

  // ToDo:
  // -Verify the  assert(self._isActive)  error check. In the original code, it is different and 
  //  looks strange. It raises an error if(!self._isActive && !self._isBeingActivated). So the 
  //  error is raised only when it's not active and also not being activated. That seems to suggest
  //  that it's ok to inquiry the latency during the activation process? But that seems wrong to me 
  //  because the sample rate is correctly assigned only after the activation has finished. 
  //  -> Figure this out. Maybe file a bug report.
}

// Line 776:
uint32_t ClapPlugin::clapParamsCount(const clap_plugin *plugin) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_params.count");
  return self.paramsCount();
}
bool ClapPlugin::clapParamsInfo(const clap_plugin *plugin,  uint32_t param_index, 
  clap_param_info *param_info) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_params.info");
  assert(param_index < clapParamsCount(plugin));              // Parameter index out of range
  const auto res = self.paramsInfo(param_index, param_info);  // result?
  assert(res);                                                // Parameter info retrieval failed
  return res;
}
bool ClapPlugin::clapParamsValue(
  const clap_plugin *plugin, clap_id paramId, double *value) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_params.value");
  const bool succeed = self.paramsValue(paramId, value);
  return succeed;

  // Notes:
  // -The original code had some more error checks that ensured that the paramId is valid and the
  //  value is within the expected range. 
}
bool ClapPlugin::clapParamsValueToText(const clap_plugin* plugin, clap_id param_id, double value,
  char* display, uint32_t size) noexcept
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_params.value_to_text");
  return self.paramsValueToText(param_id, value, display, size);

  // Notes:
  // -The original code had a lot more error checks
}
bool ClapPlugin::clapParamsTextToValue(const clap_plugin* plugin, clap_id param_id,
  const char* display, double* value) noexcept
{
  auto &self = from(plugin);

  self.ensureMainThread("clap_plugin_params.text_to_value");
  assert(display != nullptr);
  assert(value   != nullptr);


  if(!self.paramsTextToValue(param_id, display, value))
    return false;
  return true;
  // Can't we just do: return self.paramsTextToValue(param_id, display, value)?

  // Notes:
  // -The original code had a lot more error checks
}
void ClapPlugin::clapParamsFlush(const clap_plugin* plugin, const clap_input_events* in,
  const clap_output_events* out) noexcept
{
  auto &self = from(plugin);
  self.ensureParamThread("clap_plugin_params.flush");  // What is the "ParamThread"?
  assert(in  != nullptr);
  assert(out != nullptr);
  self.paramsFlush(in, out);

  // Notes:
  // -The original code had a lot more error checks
}

// Line 1133:
uint32_t ClapPlugin::clapNotePortsCount(const clap_plugin *plugin, bool is_input) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_note_port.count");
  return self.notePortsCount(is_input);
}


bool ClapPlugin::clapNotePortsInfo(const clap_plugin *plugin, uint32_t index, bool is_input,
  clap_note_port_info *info) noexcept 
{
  auto &self = from(plugin);
  self.ensureMainThread("clap_plugin_note_ports.info");
  auto count = clapNotePortsCount(plugin, is_input);
  assert(index < count);
  if(index >= count)
    return false;
  return self.notePortsInfo(index, is_input, info);
}

//-------------------------------------------------------------------------------------------------
// Misc:

// Line 1749 in clap/helpers/plugin.hxx
ClapPlugin& ClapPlugin::from(const clap_plugin *plugin, bool requireInitialized) noexcept 
{
  assert(plugin != nullptr);
  assert(plugin->plugin_data != nullptr);  // The host must never change this pointer!
                                           // It's owned by the plugin (I think)

  auto &self = *static_cast<ClapPlugin*>(plugin->plugin_data);
  assert(self._wasInitialized || requireInitialized == false);

  return *static_cast<ClapPlugin*>(plugin->plugin_data); // Can we just return self?

  //return *static_cast<ClapPlugin*>(plugin->plugin_data);  // OLD
}

// Line 191
void ClapPlugin::ensureInitialized(const char *method) const noexcept 
{
  assert(_wasInitialized);
}

// Line 1728
void ClapPlugin::ensureMainThread(const char *method) const noexcept 
{
  //if(!_host.canUseThreadCheck() || _host.isMainThread())
  //  return;
  //else
  //  assert(false);  // Wrong thread

  // Notes:
  // -This is a stub. An actual implementation would have to inquire the host if this is the main
  //  thread. But apparently our _host member has no such checking facilities unless we use the 
  //  HostProxy class (which we currently don't do)
}

void ClapPlugin::ensureAudioThread(const char* method) const noexcept
{
  //if(!_host.canUseThreadCheck() || _host.isAudioThread())
  //  return;
  //else
  //  assert(false);  // Wrong thread
}

void ClapPlugin::ensureParamThread(const char *method) const noexcept 
{
  if(isActive())
    ensureAudioThread(method);
  else
    ensureMainThread(method);
}



std::vector<std::string> ClapPlugin::getFeatures()
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

/*

Notes:

-Maybe renaming methods is a bad idea. We may wnat to swap later to the original wrapper mostly
 because of the misbehavior checking. That could make life easier by catching bugs earlier. In 
 this case, it will be much easier when we didn't rename any methods. It will also be more helpful 
 to users of the official wrapper. This here is kind of a stripped down variant suitable for 
 learning. Or maybe introduce alias names? But nah.


ToDo:

-[DONE] Maybe bring back the misbhevaior handlers but in a less verbose way like in
 rsAssert(condition, message) which triggers debug breaks. I think, the intention is to use maximal
 checking level in debug builds and minimal or none in release builds?

-Maybe wrap this class into a namespace clap::RobsHelpers or clap::RobsTools or maybe just 
 RobsClapTools, RobsClapStuff, RobsClapHelpers

-Maybe implement the tail-length extension, too. The goal is to implement all the extensions that 
 are needed ro recreate a VST 2.4 like feature set.

*/