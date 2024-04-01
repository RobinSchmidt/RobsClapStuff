#pragma once

//#include "../../../clap/include/clap/clap.h"   // Only the stable API, no drafts
#include "../clap/include/clap/clap.h"   // Only the stable API, no drafts

class ClapPlugin
{

public:

  //-----------------------------------------------------------------------------------------------
  // \name Lifetime

  /** */
  ClapPlugin(const clap_plugin_descriptor *desc, const clap_host *host);
  // ToDo: document what to do with the parameters. I think the descriptor should be filled out by 
  // the constructor and the host pointer can be stored away for later requests to the host?

  virtual ~ClapPlugin() = default;




  //-----------------------------------------------------------------------------------------------
  // \name Setup


  virtual bool stateLoad(const clap_istream* stream) noexcept { return false; }
  // { return false; }
  // rename to setState


  virtual void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept {}
  // rename to setParameters


  //-----------------------------------------------------------------------------------------------
  // \name Inquiry



  virtual bool implementsState() const noexcept { return false; }
  // rename to hasStateRecall


  virtual bool stateSave(const clap_ostream *stream) noexcept { return false; }
  // rename to getState
  // Maybe provide a baseclass implementation that cycles through the parameters and stores them in
  // a binary blob - like was done with .fxp when the "chunks" mechanism wasn't used

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


  virtual bool implementsNotePorts() const noexcept { return false; }
  // maybe rename to hasNotePorts (or maybe hasNoteIns?)

  virtual uint32_t notePortsCount(bool isInput) const noexcept { return 0; }
  // rename to getNumNotePorts or getNumNoteInputs/getNumNoteOutputs

  virtual bool notePortsInfo(uint32_t index, bool isInput, clap_note_port_info *info) const noexcept 
  {
    return false;
  }
  // rename to getNotePortInfo

  virtual bool implementsParams() const noexcept { return false; }
  // rename to hasParameters



  virtual uint32_t paramsCount() const noexcept { return 0; }
  // rename to getNumParameters

  virtual bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept 
  { return false; }


  virtual bool paramsValue(clap_id paramId, double *value) const noexcept { return false; }
  // rename to getParameterValue ...it meant to a getter, right?
  // Why is this not const?
  // ..OK - I changed it to const - maybe ask on github
  // ...also for the  paramsValueToText, paramsTextToValue functions

  virtual bool paramsValueToText(
    clap_id paramId, double value, char *display, uint32_t size) noexcept 
  {
    return false;
  }
  // rename to parameterValueToText

  virtual bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept 
  {
    return false;
  }
  // rename to parameterTextToValue

  virtual bool implementsGui() const noexcept { return false; }
  // rename to hasGui. A subclass ClapPluginWithGui could add more gui related functions


  virtual bool implementsLatency() const noexcept { return false; }

  virtual uint32_t latencyGet() const noexcept { return 0; }

  // ToDo: add implementsTail/tailGet


  virtual const void *extension(const char *id) noexcept { return nullptr; }
  // rename to getExtension


  /** Returns a pointer ot the underlying struct in th C-API. */
  const clap_plugin *clapPlugin() noexcept { return &_plugin; }
  // rename to getPluginStructC


  bool isActive() const noexcept { return _isActive; }

  bool isProcessing() const noexcept { return _isProcessing; }

  double sampleRate() const noexcept 
  {
    //assert(_isActive && "sample rate is only known if the plugin is active");
    //assert(_sampleRate > 0);
    return _sampleRate;
  }

  bool isBeingDestroyed() const noexcept { return _isBeingDestroyed; }



  //-----------------------------------------------------------------------------------------------
  // \name Processing


  virtual bool init() noexcept { return true; }
  // Will be called on the main thread


  virtual bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
  { return true; }

  virtual void deactivate() noexcept {}

  virtual bool startProcessing() noexcept { return true; }

  virtual void stopProcessing() noexcept {}

  virtual clap_process_status process(const clap_process *process) noexcept 
  {
    return CLAP_PROCESS_SLEEP;
  }
  // Maybe make purely virtual

  virtual void reset() noexcept {}


  //-----------------------------------------------------------------------------------------------
  // \name Misc

  virtual void onMainThread() noexcept {}
  // What is this?




  //-----------------------------------------------------------------------------------------------
  // My own functions that were not part of the original wrapper:

  /** Returns a pointer to the underlying C-struct of the clap plugin. This is needed for the glue 
  code during instatiation. */
  const clap_plugin* getPluginStructC() const { return &_plugin; }
  // make const

  /** Returns a pointer to the plugin descriptor. This is needed for inquiries inside subclasses 
  for reflection purposes, e.g. when a plugin (sub)class wants to inquire its own plugin ID, 
  etc. */
  const clap_plugin_descriptor* getPluginDescriptor() const
  {
    return getPluginStructC()->desc;
  }

  bool isDoublePrecision(const clap_process* process)
  { return process->audio_inputs[0].data64 != nullptr; }
  // ToDo:
  // -Verify if this is the right way to figure it out
  // -Maybe we should first ensure that process->audio_inputs is not a nullptr

  const char* getPluginIdentifier() const { return getPluginDescriptor()->id; }

  const char* getPluginVersion()    const { return getPluginDescriptor()->version; }

  const char* getPluginVendor()     const { return getPluginDescriptor()->vendor; }



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



  //-----------------------------------------------------------------------------------------------
  // \name Internals

  // Some internal functions:

  static ClapPlugin& from(const clap_plugin *plugin, bool requireInitialized = true) noexcept;
  // Why is this needed? Ah - I see - it wraps this casting business.


  void ensureInitialized(const char *method) const noexcept;





  // Make uncopyable and unmovable:
  ClapPlugin(const ClapPlugin &) = delete;
  ClapPlugin(ClapPlugin &&) = delete;
  ClapPlugin &operator=(const ClapPlugin &) = delete;
  ClapPlugin &operator=(ClapPlugin &&) = delete;

};