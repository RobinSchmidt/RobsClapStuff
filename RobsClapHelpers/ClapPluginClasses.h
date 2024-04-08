#pragma once

// This file contains some subclasses of ClapPlugin with certain common features. The intention is 
// that these common features can be factored out into this intermediate class such that you actual
// plugin class does not need to derive ffrom the general purpose clapPlugin class but from one
// the classes here. Then you will need to have write less boilerplate code in your actual plugin 
// class.



/** A class to represent the current state (i.e. the value) of a parameter for a clap plugin. */

struct ClapPluginParameter
{
  ClapPluginParameter(clap_id newID, double newValue) : id(newID), value(newValue) {}

  double  value = 0.0;
  clap_id id    = 0;
};
// ToDo: maybe have a function pointer parameter for converting to string



//=================================================================================================

/** A subclass of ClapPlugin that implements handling of parameters and state. The state handling 
will simply store and recall all the values the parameters. Subclasses should use addParameter
in their constructor.

ToDo:
-Hmm...I think, we need to bring back the processSubBlock32/64 functions and implement process

*/

class ClapPluginWithParams : public ClapPlugin
{

public:

  ClapPluginWithParams(const clap_plugin_descriptor* desc, const clap_host* host)
    : ClapPlugin(desc, host) { }



  //-----------------------------------------------------------------------------------------------
  // \name Parameter handling

  bool implementsParams() const noexcept override { return true;  }

  uint32_t paramsCount() const noexcept override { return (uint32_t) params.size(); }  


  bool paramsInfo(uint32_t index, clap_param_info* info) const noexcept override;


  bool paramsValue(clap_id paramId, double *value) const noexcept override;

  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;

  bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;

  /** Overrides the paramsFlush method to call processEvent for all the passed input events. */
  void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override;

  /** Adds a parameter with given identifier, name, etc. to the plugin. This is supposed to be 
  called in the constructor of your subclass to create and set up all the parameters that you want
  to expose to the host. */
  void addParameter(clap_id identifier, const std::string& name, double minValue, double maxValue, 
    double defaultValue, clap_param_info_flags flags);
  // ToDo: maybe include a path/module string (e.g. Osc2/WaveTable/Spectrum/ )

  /** Tries to find a parameter with given id in our params array and returns its index when the id
  was found or -1 when the id is not found. */
  int findParameter(clap_id id) const;

  /** Sets all the parameters to their default values by calling setParameter for each. */
  void setAllParametersToDefault();

  /** Sets the parameter with the given id to the new value. After storing the new value in our 
  params array, this will invoke a call to parameterChanged which your subclass should override, if
  it needs to respond to parameter change events. */
  void setParameter(clap_id id, double newValue);
  // Why ot virtual? Ah - because it's not supposed to be overriden. If a plugin wants to respond
  // to paremeter changes, it needs to override parameterChanged. Maybe decaler it final.

  /** Returns the current value of the parameter with the given id. If the id doesn't exist, it 
  will return zero. */
  double getParameter(clap_id id) const;

  /** Subclasses should override this to respond to parameter changes. For example, they may want 
  to recalculate some coefficients for the DSP algorithm when a parameter was changed. It has been 
  made purely virtual because in most cases, you will really want to override this and would be a 
  mistake not to. If you have one of those rare an atypical special cases where you don't need to 
  respond to parameter changes, you can just override it with an empty implementation. */
  virtual void parameterChanged(clap_id id, double newValue) = 0;

  /** Function to produce a string from a parameter value for display on the host-generated GUI. 
  You may pass a desired "precision", i.e. number of decimal digits after the dot and an optional
  suffix which can be used for displaying a physical unit such as " Hz" or " dB". If you want a 
  space between the number and the unit, you need to explcitly include that space in the suffix. It
  returns a bool to report success or failure. */
  bool toDisplay(double value, char* destination, int size, int precision,
    const char* suffix = nullptr)
  {
    return toStringWithSuffix(value, destination, size, precision, suffix) > 0;
  }

  /** This function can be used for converting a choice/enum parameter to a display string, 
  assuming that you keep a string-array with the textual representations of the choices around. The
  convention to convert the value into a strings is as follows: the "value" will be rounded to an
  integer and the result is used as index into the "strings" array. */
  bool toDisplay(double value, char* destination, int size,
    const std::vector<std::string>& strings)
  {
    return copyString(strings, (int) round(value), destination, size);
  }

  /** Tries to find the "displayString" in the array of "strings". If it was found, the index where
  it was found will be assigned to the "value" and "true" will be returned. If it was not found,
  "value" will be assigned to zero and "false" will be returned. The purpose of the this function 
  is to facilitate conversion of display-strings to numeric parameter values. That means, it 
  implements the reverse mapping to the toDisplay() function for the choice/enum/string 
  parameters. */
  bool toValue(const char* displayString, double* value, const std::vector<std::string>& strings)
  {
    int i = findString(strings, displayString);
    if(i == -1) { *value = 0.0;        return false; }
    else        { *value = (double) i; return true;  }
  }


  //-----------------------------------------------------------------------------------------------
  // \name State handling

  /** Overrides the implementsStae function to inform the host that we indeed implement the state 
  extension. */
  bool implementsState() const noexcept override { return true; }

  /** Overrides stateSave to write the values of all our parameters in a simple textual format into
  the stream. It also stores information about the plugin and version which may be used on recall
  to do some checks and facilitate conversions when the format has changed between plugin 
  versions. */
  bool stateSave(const clap_ostream *stream) noexcept override;

  /** Restores the values of all of our parameters from a stream that was supposedly created 
  previously via the stateSave function. If the stream does not have values for all of our 
  parameters stored (perhaps because the state was saved with an older version of the plugin which
  had less parameters), then the missing ones will be assigned to their default values. */
  bool stateLoad(const clap_istream* stream) noexcept override;

  /** This creates a string that represents the state which given by the values of all of our 
  parameters. */
  virtual std::string getStateAsString() const;
  // ToDo: document the format of the string

  /** Restores the state, i.e. the values of all parameter, from the given string which was 
  presumably created by calling getStateAsString at some time before. */
  virtual bool setStateFromString(const std::string& stateString);



  //-----------------------------------------------------------------------------------------------
  // \name Misc

  //std::vector<std::string> getFeatures();
  // This might actually go into the baseclass. Functionality-wise, it belongs there. But then the 
  // baseclass already gets coupled to std::string which might be undesirable ...we'll see....
  // ...but now that we have a unity build system, it doesn't really matter anymore. <string> is
  // available already in the baseclass.


protected:

  /** This is called from within our implementation of process to handle one event at a time. In 
  our implementation here, we currently handle only parameter change events (by calling 
  setParameter which you may override, if you want to respond to parameter changes). */
  virtual void processEvent(const clap_event_header_t* hdr);
  // Maybe make public


private:

  std::vector<ClapPluginParameter> params;
  std::vector<clap_param_info>     infos;

};


//=================================================================================================

/** This class can be used as baseclass for clap plugins that have stereo in/out and want to do 
their processing in 32 bit. That's the most common case (for me, at least) which is why it makes 
sense for me to factor out a baseclass for this case to reduce the amount of boilerplate for this
sort of plugin. */

class ClapPluginStereo32Bit : public ClapPluginWithParams
{

public:

  ClapPluginStereo32Bit(const clap_plugin_descriptor* desc, const clap_host* host)
    : ClapPluginWithParams(desc, host) { }


  //-----------------------------------------------------------------------------------------------
  // \name Overrides

  /** Yes - we implement the audio ports extension. */
  bool implementsAudioPorts() const noexcept { return true; }

  /** Declare 1 input and 1 output port/bus. */
  uint32_t audioPortsCount(bool isInput) const noexcept override { return 1; }   // 1 in, 1 out

  /** Fills out the info declaring the in/out ports as having 2 channels, being the main ports and
  NOT supporting 64 bit processing. It also declares that in-place processing is allowed, assigns
  names "Stereo In", "Stereo Out" to the channels, gives the port the id zero and declares it as 
  a stereo pair. */
  bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) 
    const noexcept override;

  /** We override the process callback to handle the passed events and wheneve an event is 
  received ve call processEvent with that event. In between the events we call processBlockStereo
  with the pointers an numFrames variables adjusted to the correct sub-buffer to be processed. */
  clap_process_status process(const clap_process *process) noexcept override;
  // baseclass needs also an implementation of this




  //-----------------------------------------------------------------------------------------------
  // \name Callbacks to override by your subclass

  /** Your subclass must override this function to process one stereo block of in/out samples of
  length numFrames. */
  virtual void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) = 0;



protected:





};
