#pragma once


/** A C++ wrapper class around the C-struct for a CLAP plugin. To implement a CLAP plugin using 
this wrapper, you need to subclass it and then override certain member functions to implement the
specific functionality of your plugin.

...TBC...

ToDo:
-order the functions according to which extension they belong to

*/

class ClapPlugin
{

public:

  //-----------------------------------------------------------------------------------------------
  // \name Lifetime and core functionality

  /** Constructor of the plugin C++ wrapper. It will assign all the function pointers in our 
  "_plugin" member variable (which is the underlying C-struct) to point to our static member 
  functions that provide the glue. ...TBC...  */
  ClapPlugin(const clap_plugin_descriptor *desc, const clap_host *host);
  // ToDo: document what to do with the parameters. I think the descriptor should be filled out by 
  // the constructor and the host pointer can be stored away for later requests to the host?

  virtual ~ClapPlugin() = default;

  /** Must be called after creating the plugin. If init returns false, the host must destroy the 
  plugin instance. If init returns true, then the plugin is initialized and in the deactivated 
  state. Unlike in `plugin-factory::create_plugin`, in init you have complete access to the host 
  and host extensions, so clap related setup activities should be done here rather than in 
  create_plugin. C-API: clap_plugin.init [main-thread]  */
  virtual bool init() noexcept { return true; }

  /** Activate and deactivate the plugin. In this call the plugin may allocate memory and prepare 
  everything needed for the process call. The process's sample rate will be constant and process's 
  frame count will included in the [min, max] range, which is bounded by [1, INT32_MAX]. Once 
  activated the latency and port configuration must remain constant, until deactivation. Returns 
  true on success. C-API: clap_plugin.activate [main-thread & !active]  */
  virtual bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
  { return true; }


  virtual void deactivate() noexcept {}

  virtual bool startProcessing() noexcept { return true; }

  virtual void stopProcessing() noexcept {}

  virtual void reset() noexcept {}

  virtual clap_process_status process(const clap_process *process) noexcept 
  {
    return CLAP_PROCESS_SLEEP;
  }

  virtual const void *extension(const char *id) noexcept { return nullptr; }

  bool isActive() const noexcept { return _isActive; }

  bool isProcessing() const noexcept { return _isProcessing; }

  double sampleRate() const noexcept 
  {
    //clapAssert(_isActive && "sample rate is only known if the plugin is active");
    //clapAssert(_sampleRate > 0);
    return _sampleRate;
  }

  bool isBeingDestroyed() const noexcept { return _isBeingDestroyed; }
 


  //-----------------------------------------------------------------------------------------------
  // \name Audio Ports


  virtual bool implementsAudioPorts() const noexcept { return false; }
  // rename to hasAudioPorts, return true by default

  virtual uint32_t audioPortsCount(bool isInput) const noexcept { return 0; }
  // rename to getNumChannels, maybe write two functions getNumInputs/getNumOutputs - dispatch from
  // this function to these, if needed. Ot maybe getNumAudioIns/getNumAudioOuts - because we can 
  // also receive and send other data like notes and controller data
  // NO! I think, this is the number of busses!

  virtual bool audioPortsInfo(
    uint32_t index, bool isInput, clap_audio_port_info *info) const noexcept 
  { return false; }
  // You should fill out the info, I guess.



  //-----------------------------------------------------------------------------------------------
  // \name Note Ports

  virtual bool implementsNotePorts() const noexcept { return false; }
  // maybe rename to hasNotePorts (or maybe hasNoteIns?)

  virtual uint32_t notePortsCount(bool isInput) const noexcept { return 0; }
  // rename to getNumNotePorts or getNumNoteInputs/getNumNoteOutputs

  virtual bool notePortsInfo(uint32_t index, bool isInput, clap_note_port_info *info) const noexcept 
  {
    return false;
  }
  // rename to getNotePortInfo





  //-----------------------------------------------------------------------------------------------
  // \name Parameters
  // 
  // I think, it works as follows:
  // The paramIndex determines the order in which the knobs are shown on the generic GUI. To set or
  // automate a parameter, however, the parameter's id is used which may or may not match the 
  // index. That makes the system a bit more flexible when more parameters should be added or the
  // order of parameters should be changed in a later version of the plugin. The id must remain 
  // stable from version to version but the index may change.


  virtual bool implementsParams() const noexcept { return false; }


  /** Returns the number of parameters. C-API: clap_plugin_params.count [main-thread] */
  virtual uint32_t paramsCount() const noexcept { return 0; }
  // ToDo: figure out and document how a plugin can inform the host when its number of parameters 
  // has changed.


  virtual bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept 
  { return false; }
 


  virtual bool paramsValue(clap_id paramId, double *value) const noexcept { return false; }
  // Was originally not declared as const in Alex's wrapper. Why? Maybe ask on GitHub. I changed it
  // to const because I want to call it from other const functions.

  // ...also for the  paramsValueToText, paramsTextToValue functions



  virtual bool paramsValueToText(
    clap_id paramId, double value, char *display, uint32_t size) noexcept 
  {
    return false;
  }


  virtual bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept 
  {
    return false;
  }


  /** Maps to clap_plugin_params.flush. [active ? audio-thread : main-thread]

  Flushes a set of parameter changes. This method must not be called concurrently to 
  clap_plugin->process(). If the plugin is processing, then the process() call will already achieve
  the parameter update (bi-directional), so a call to flush isn't required, also be aware that the 
  plugin may use the sample offset in process(), while this information would be lost within 
  flush(). */
  virtual void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept {}








  //-----------------------------------------------------------------------------------------------
  // \name State

  virtual bool implementsState() const noexcept { return false; }
  // rename to hasStateRecall


  virtual bool stateSave(const clap_ostream *stream) noexcept { return false; }
  // rename to getState
  // Maybe provide a baseclass implementation that cycles through the parameters and stores them in
  // a binary blob - like was done with .fxp when the "chunks" mechanism wasn't used


  virtual bool stateLoad(const clap_istream* stream) noexcept { return false; }
  // { return false; }





  //-----------------------------------------------------------------------------------------------
  // \name Latency

  virtual bool implementsLatency() const noexcept { return false; }

  virtual uint32_t latencyGet() const noexcept { return 0; }

  // ToDo: add implementsTail/tailGet



  //-----------------------------------------------------------------------------------------------
  // \name GUI


  virtual bool implementsGui() const noexcept { return false; }
  // rename to hasGui. A subclass ClapPluginWithGui could add more gui related functions
















  //-----------------------------------------------------------------------------------------------
  // \name Misc

  virtual void onMainThread() noexcept {}
  // What is this?




  //-----------------------------------------------------------------------------------------------
  // My own functions that were not part of the original wrapper:

  /** Returns a pointer to the underlying C-struct of the clap plugin. This is needed for the glue 
  code during instatiation. */
  //const clap_plugin* getPluginStructC() const { return &_plugin; }
  // get rid - is redundant with clapPlugin() - but that other one is not const. But maybe we can 
  // make it const


  /** Returns a pointer ot the underlying struct in th C-API. This is needed for the glue code 
  during instatiation. */
  const clap_plugin *clapPlugin() const noexcept { return &_plugin; }

  /** Returns a pointer to the plugin descriptor. This is needed for inquiries inside subclasses 
  for reflection purposes, e.g. when a plugin (sub)class wants to inquire its own plugin ID, 
  etc. */
  const clap_plugin_descriptor* getPluginDescriptor() const { return clapPlugin()->desc; }

  /** Returns true, iff the "process" object wants the signals to be processed in double 
  precision. */
  static bool isDoublePrecision(const clap_process* process)
  { return process->audio_inputs[0].data64 != nullptr; }
  // ToDo:
  // -Rename to wantsDoublePrecision ..or maybe hasDoublePrecision
  // -Verify if this is the right way to figure it out
  // -Maybe we should first ensure that process->audio_inputs is not a nullptr?

  static bool hasSinglePrecision(const clap_process* process)
  {
    return process->audio_inputs[0].data32 != nullptr;
  }



  /** Returns the plugin identifier. This is a string that uniquely identifies a plugin. Among 
  other things, it is used by DAWs to recall the plugin chains on their tracks. */
  const char* getPluginIdentifier() const { return getPluginDescriptor()->id; }

  const char* getPluginVersion()    const { return getPluginDescriptor()->version; }

  const char* getPluginVendor()     const { return getPluginDescriptor()->vendor; }

  double getSampleRate() const { return _sampleRate; }

  std::vector<std::string> getFeatures();



protected:


  //-----------------------------------------------------------------------------------------------
  // \name Checks


  void ensureMainThread(const char *method) const noexcept;

  void ensureAudioThread(const char *method) const noexcept;

  void ensureParamThread(const char *method) const noexcept;  
  // When the plugin is active, the parameters are supposed to be received on the audio thread and
  // in inactive state on the main thread.

  //void runCallbacksOnMainThread();
  // What is this?


private:


  clap_plugin       _plugin;   // Our member of the struct-type from the C-API
  const clap_host*  _host;     // A pointer to our host
  // Was initially a declared as HostProxy<h, l> _host; I guess that HostProxy is the C++ wrapper
  // for the clap_host struct? So maybe we should make a wrapper class ClapHost and let our host be
  // of that type?

  // state
  double _sampleRate       = 0;
  bool   _wasInitialized   = false;
  bool   _isBeingDestroyed = false;
  bool   _isActive         = false;
  bool   _isBeingActivated = false;
  bool   _isProcessing     = false;
  bool   _isGuiCreated     = false;


  //-----------------------------------------------------------------------------------------------
  // \name Boiler plate code for the glueing

  // Interfaces/extensions
  static const clap_plugin_audio_ports _pluginAudioPorts;
  static const clap_plugin_state       _pluginState;
  static const clap_plugin_params      _pluginParams;
  static const clap_plugin_note_ports  _pluginNotePorts;
  static const clap_plugin_latency     _pluginLatency;


  // Static member fuctions to be assigned to the function pointers in the C-struct, i.e. the glue 
  // between C-API and the C++ wrapper:

  // Basic clap plugin callbacks:
  static bool clapInit(const clap_plugin *plugin) noexcept;
  static void clapDestroy(const clap_plugin *plugin) noexcept;
  static bool clapActivate(const clap_plugin *plugin, double sample_rate, uint32_t minFrameCount,
                           uint32_t maxFrameCount) noexcept;
  static void clapDeactivate(const clap_plugin *plugin) noexcept;
  static bool clapStartProcessing(const clap_plugin *plugin) noexcept;
  static void clapStopProcessing(const clap_plugin *plugin) noexcept;
  static void clapReset(const clap_plugin *plugin) noexcept;
  static clap_process_status clapProcess(const clap_plugin *plugin, 
                                         const clap_process *process) noexcept;
  static void clapOnMainThread(const clap_plugin *plugin) noexcept;
  static const void *clapExtension(const clap_plugin *plugin, const char *id) noexcept;

  // Callbacks for extensions:
  static bool clapStateSave(const clap_plugin *plugin, const clap_ostream *stream) noexcept;
  static bool clapStateLoad(const clap_plugin *plugin, const clap_istream *stream) noexcept;

  static uint32_t clapLatencyGet(const clap_plugin *plugin) noexcept;

  static uint32_t clapAudioPortsCount(const clap_plugin *plugin, bool is_input) noexcept;
  static bool clapAudioPortsInfo(const clap_plugin *plugin, uint32_t index, bool is_input,
    clap_audio_port_info *info) noexcept;

  static uint32_t clapParamsCount(const clap_plugin *plugin) noexcept;
  static bool clapParamsInfo(const clap_plugin *plugin, uint32_t param_index, 
                             clap_param_info *param_info) noexcept;
  static bool clapParamsValue(const clap_plugin *plugin, clap_id param_id, double *value) noexcept;
  static bool clapParamsValueToText(const clap_plugin *plugin, clap_id param_id, double value,
                                    char *display, uint32_t size) noexcept;
  static bool clapParamsTextToValue(const clap_plugin *plugin, clap_id param_id, const char *display,
                                    double *value) noexcept;
  static void clapParamsFlush(const clap_plugin *plugin, const clap_input_events *in, 
    const clap_output_events *out) noexcept;

  static uint32_t clapNotePortsCount(const clap_plugin *plugin, bool is_input) noexcept;
  static bool clapNotePortsInfo(const clap_plugin *plugin, uint32_t index, bool is_input,
    clap_note_port_info *info) noexcept;
  // Maybe move these into the subclasses that actually implement them. Let the class ClapPlugin
  // be only a bare bones plugin without any extensions. That will require to override the
  // extension() method and also a re-implementation of the "from" method...hmm...I'm not sure, if
  // that is a good idea. It would lead to more boilerplate, I think. So maybe, having all that
  // glue stuff here (and providing default implementations) is better. 


  //-----------------------------------------------------------------------------------------------
  // \name Internals

  /** A convenience function to cast an underlying C-struct representing a plugin into its C++ 
  wrapper object. This method is used all throughout the glue code in an idiomatic pattern like:
  
    void ClapPlugin::clapDoStuff(const clap_plugin* plugin,  ...)
    {
      auto &self = from(plugin);
      self.doStuff(...);
    }

  where clapDoStuff is some static member function that maps to a call to a non-static member
  function .doStuff() on the C++ wrapper of the "plugin" object. The "self" represents the "this"
  pointer of the wrapper object ...TBC... */
  static ClapPlugin& from(const clap_plugin *plugin, bool requireInitialized = true) noexcept;
  // ToDo: try to explain this better


  /** Asserts that our "_wasInitialized" member is true. */
  void ensureInitialized(const char *method) const noexcept;


  // Make uncopyable and unmovable:
  ClapPlugin(const ClapPlugin &) = delete;
  ClapPlugin(ClapPlugin &&) = delete;
  ClapPlugin &operator=(const ClapPlugin &) = delete;
  ClapPlugin &operator=(ClapPlugin &&) = delete;

};