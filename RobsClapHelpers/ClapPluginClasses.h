#pragma once

// This file contains some subclasses of ClapPlugin with certain common features. The intention is 
// that these common features can be factored out into this intermediate class such that your 
// actual plugin class does not need to derive directly from the general purpose ClapPlugin class 
// but from one of the more specialized classes here. Then you will need to have write less 
// boilerplate code in your actual plugin class.


//=================================================================================================

/** A subclass of ClapPlugin that implements handling of parameters including saving and recalling
them via the state extension. Subclasses should use addParameter() in their constructor to set up 
their parameters and override parameterChanged() to respond to parameter changes.

When adding a parameter, you need to assign an identifier (short: id) to it. When the host later 
wants to set or get the value of the parameter or convert it to a string or whatever, it will refer
to the parameter by this id. The way the id-system is handled by this class here assumes that the
ids are determined by an enum (that is supposedly defined somewhere inside your plugin subclass) 
and that the ids run continuously from 0 to N-1 where N is the number of parameters. An id, once 
assigned to a parameter, must remain stable from one plugin version to the next. That means, after 
a plugin is published, you cannot reorder the enum or delete entries from it or insert entries at 
arbitrary positions. You can, however, append new entries at the end for more parameters. You also 
*can* reorder the parameter-knobs presented by the host on the generic GUI because that ordering is
not determined by the id but rather by the index. When you use this class, the index will be 
determined by the order in which you add the parameters in your constructor. This may or may not 
match the order in the enum. But one invariant must always hold true: Let N be the number of 
parameters such that the index naturally runs from 0 to N-1. Then, the list of the identifiers must
be some permutation of the list of their corresponding indices. It is easy to mess this up which is
why I recommend to use some sort debug-assertion like:

  clapAssert(areParamsConsistent());

after adding your parameters in your constructor. Look at the examples ClapGain, ClapWaveShaper for
how this could look like. @see paramsInfo(). Messing this up could happen, for example, by 
forgetting to use one of your enum entries or to use one of your enum entries twice in your 
sequence of addParameter() calls. The consistency check basically checks, if each of your ids 
(== enum-entries) is used exactly once. This consistency may be violated in the middle of your 
sequence of addParameter() calls but after the last call, consistency should be established.

The reason why I have opted for this particular convention for mapping between parameter index and 
identifier is that it allows for a very simple O(1) access to all parameters without needing a 
complicated data-structure (like a std::map, say). Our mapping is basically a bijective function of 
the set { 0, ..., N-1 } to itself(!) which can be implemented by a simple pair of std::vector<int>
of length N. One vector for the forward mapping and one for the inverse mapping. I have actually 
written the class IndexIdentifierMap in Utilities.h for precisely that purpose but it turned out 
that at the moment, we don't even need that (yet). We may need it later when we need a O(1) mapping 
from id to index for some more functionality like retrieving the clap_param_info by id rather than 
by index. At the moment, we can get away without such a functionality, though.

At the moment, it is not recommended to derive your plugin class *directly* from the class
ClapPluginWithParams but if you do, you will need to implement process() and there you will need to
implement the interleaving of event-handling and audio-processing yourself - which really is 
tedious boilerplate stuff that should some day be handled by the baseclass. To avoid that, you 
should derive your plugin class from ClapPluginStereo32Bit where the interleaving is already 
handled and you only need to override processBlockStereo() which gets called for all the sub-blocks
between the events. As the name suggests, this class is restricted to 32-bit-stereo plugins, 
though. I'm still working on it... (ToDo: implement process and bring back the processSubBlock32/64
functions from the private repo). */

class ClapPluginWithParams : public ClapPlugin  // Maybe rename to ClapWithParams. The P in CLAP alreday stands for Plugin
{

public:

  ClapPluginWithParams(const clap_plugin_descriptor* desc, const clap_host* host)
    : ClapPlugin(desc, host) { }


  //-----------------------------------------------------------------------------------------------
  // \name Parameter handling

  /** Informs the host that we do implement the parameters extension. */
  bool implementsParams() const noexcept override { return true;  }

  /** Returns the number of parameters that this plugin has. */
  uint32_t paramsCount() const noexcept override { return (uint32_t) infos.size(); }   
  // values.size() == infos.size() unless soemthing is wrong

  /** Fills out the passed clap_param_info struct with the data for the parameter with the given 
  index. Note that the "index" is not the same thing as the "identifier" or "id", for short. The 
  index is a number that runs from 0 to numParams-1 and determines the order in which the 
  parameters will be presented on the host-generated GUI. The id is a number that the plugin itself
  may assign to its parameters. The id is actually a field in the info struct. When the host wants 
  to set a parameter, it will use this id to identify the parameter. The index is just used here to 
  inquire the info (I guess, once, when the plugin is loaded (VERIFY!)). The id is used whenever a 
  parameter is set. */
  bool paramsInfo(uint32_t index, clap_param_info* info) const noexcept override;

  /** Assigns the output variable "value" to the value of the parameter with the given parameter 
  id and reports success or failure in the return value. In case of failure (i.e. when the id 
  doesn't exist), the "value" will be assigned to zero. */
  bool paramsValue(clap_id paramId, double *value) const noexcept override;

  /** Provides a default implementation of mapping parameter values to strings. The default 
  implementation will show the value with 2 decimal digits after the dot. If you want a different
  formatting, you will need to override this function in your subclass. */
  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;

  /** This override implements a default implementation for mapping a parameter string to a value
  using strtod(). If you want more a customized string-to-value mapping for (some of) your 
  parameters, you may override this. Note that the clap-validator application requires a plugin to
  be able to correctly map back-and-forth between values and strings in both directions. So in 
  particular in the case of choice/enum parameters, you really will need to override this if you 
  want these tests to pass (for continuous numeric parameters, you may get away with using the 
  default implementation). */
  bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;

  /** Overrides the paramsFlush method to call processEvent for all the passed input events. */
  void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override;

  /** Adds a parameter with given identifier, name, etc. to the plugin. This is supposed to be 
  called in the constructor of your subclass to create and set up all the parameters that you want
  to expose to the host. */
  void addParameter(clap_id identifier, const std::string& name, double minValue, double maxValue, 
    double defaultValue, clap_param_info_flags flags);
  // ToDo: maybe include a path/module string (e.g. Osc2/WaveTable/Spectrum/ )

  /** Sets all the parameters to their default values by calling setParameter for each. */
  void setAllParametersToDefault();

  /** Sets the parameter with the given id to the new value. After storing the new value in our 
  params array, this will invoke a call to parameterChanged which your subclass should override, if
  it needs to respond to parameter change events. The method is not virtual because it is not 
  intended to be overriden - responses to parameter changes should be handled by overriding
  parameterChanged. */
  void setParameter(clap_id id, double newValue);

  /** Returns the current value of the parameter with the given id. If the id doesn't exist, it 
  will return zero. */
  double getParameter(clap_id id) const;

  /** Subclasses should override this to respond to parameter changes. For example, they may want 
  to recalculate some coefficients for the DSP algorithm when a parameter was changed. It has been 
  made purely virtual because in most cases, you will really want to override this and it would be 
  a bug if you don't. If you have one of those rare and atypical special cases where you don't need
  to respond to parameter changes, you can just override it with an empty implementation. */
  virtual void parameterChanged(clap_id id, double newValue) = 0;

  /** Function to produce a string from a parameter value for display on the host-generated GUI. 
  You may pass a desired "precision", i.e. number of decimal digits after the dot and an optional
  suffix which can be used for displaying a physical unit such as " Hz" or " dB". If you want a 
  space between the number and the unit, you need to explicitly include that space in the suffix. 
  It returns a bool to report success or failure. */
  bool toDisplay(double value, char* destination, int size, int precision,
    const char* suffix = nullptr)
  {
    return toStringWithSuffix(value, destination, size, precision, suffix) > 0;
  }
  // ToDo: Explain how it could fail and why the CLAP-API cares and what is supposed to happen in
  // case of a failure. I think, it fails when the "size" is insufficient to hold the formatted 
  // string, etc.

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
  parameters. The CLAP-API requires that plugins can correctly convert back-and-forth between 
  numeric values and their display strings. It's not enough to go one way, i.e. from value to 
  string. The clap-validator app will also check roundtrips. For enum/choice parameters, that 
  requires finding an index of a string. */
  bool toValue(const char* displayString, double* value, const std::vector<std::string>& strings)
  {
    int i = findString(strings, displayString);
    if(i == -1) { *value = 0.0;        return false; }
    else        { *value = (double) i; return true;  }
  }

  /** This is a self-check for internal consistency. It is recommended to verify this after all 
  your addParameter calls in some sort of assertion in debug builds to catch bugs in your parameter
  setup code. */
  bool areParamsConsistent();
  // Maybe try to find a more descriptive name - "consistent" is a bit too general and vague. On 
  // the other hand, client code should probably not really have to care about what exactly 
  // "consistency" means. 


  //-----------------------------------------------------------------------------------------------
  // \name State handling

  /** Overrides the implementsState function to inform the host that we indeed implement the state 
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

  /** This creates a string that represents the state which is given by the values of all of our 
  parameters. */
  virtual std::string getStateAsString() const;
  // ToDo: document the format of the string

  /** Restores the state, i.e. the values of all parameters, from the given string which was 
  presumably created by calling getStateAsString at some time before. */
  virtual bool setStateFromString(const std::string& stateString);


  //-----------------------------------------------------------------------------------------------
  // \name Processing


  //clap_process_status process(const clap_process *process) noexcept override;
  // UNDER CONSTRUCTION - Nah - goes into ClapPluginWithAudio


  /** This is called from within our implementation of process to handle one event at a time. In 
  our implementation here, we currently handle only parameter change events by calling 
  setParameter which in turn will trigger a call to the purely virtual parameterChanged() callback
  which you need to override, to implement your responses to parameter changes. */
  virtual void processEvent(const clap_event_header_t* hdr);
  // ...well...actually, we do not yet have an implementation of process() here in this class. We 
  // have one in ClapPluginStereo32Bit, though.


private:

  std::vector<double>          values;  // Current values, indexed by id
  std::vector<clap_param_info> infos;   // Parameter informations, indexed by index

};

//=================================================================================================

/** 

*/

class ClapPluginWithAudio : public ClapPluginWithParams
{

  using Base = ClapPluginWithParams;  // For conveniently calling baseclass methods
  using Base::Base;                   // For inheriting baseclass constructor(s)

public:

  /** Yes - we implement the audio ports extension. */
  bool implementsAudioPorts() const noexcept { return true; }

  /** Declares 1 input and 1 output port/bus. That's the most common case and therefore a good 
  default. Subclasses can override this if they have a different configuration of I/O ports. */
  uint32_t audioPortsCount(bool isInput) const noexcept override { return 1; }

  /** The method is actually inherited and not purely virtual in the baseclass. The baseclass 
  implementation is empty and returns false. We re-declare it as purely virtual here because 
  subclasses with audio ports will *really* need to override this. */
  bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) 
    const noexcept = 0;
  // I'm not sure, if it's legal standard C++ to "abstractify" an inherited method like that. It 
  // seems to work in Visual Studio, though. If it doesn't work with other compilers, we can 
  // provide a default implementation that returns false, writes error messages into the port 
  // names and triggers a debug-break. Then the missing override will be caught at runtime which 
  // is the next best thing.

  clap_process_status process(const clap_process *process) noexcept override;
  // !!!NEEDS TESTS!!!


protected:

  void handleProcessEvents(const clap_process* process, uint32_t frameIndex, uint32_t numFrames,
    uint32_t &eventIndex, uint32_t numEvents, uint32_t &nextEventFrame);
  // ToDo: 
  // -Use pointers for the ouptut variables - not references. Rationale: It's obvious at the 
  //  call site that the values can be modified by the function.
  // -Make protected


  // To be overriden by subclasses. The default implementations here do nothing:
  virtual void processSubBlock32(const clap_process* process, uint32_t begin, uint32_t end);
  virtual void processSubBlock64(const clap_process* process, uint32_t begin, uint32_t end);
  // To iterate over the sample frames (typically in the innermost loop), you should use "begin" 
  // as the first sample frame index to be processed and the "end" is *one after* the last
  // sample frame index to be processed, so the "end" itself is excluded. The loop over the samples
  // would look like:
  //
  //   for(uint32_t n = begin; n < end; n++)
  //   {
  //      // process sample frame with index n
  //   }
  //
  // That's pretty much the same semantics of "end" as with standard library iterators. Of course,
  // the processing functions will also need to iterate over the ports and channels (typically in
  // outer loops around the loop over the sample frames)

};

//=================================================================================================

/** This class can be used as baseclass for clap plugins that have stereo in/out and want to do 
their processing in 32 bit. That's the most common case (for me, at least) which is why it makes 
sense for me to factor out a baseclass for this case to reduce the amount of boilerplate for this
sort of plugin. 

At the moment, it is recommended to derive your plugin class from this class and not from its 
baseclass ClapPluginWithParams because the baseclass does not yet implement the event-handling. */

class ClapPluginStereo32Bit : public ClapPluginWithAudio
{

  using Base = ClapPluginWithAudio;   // For conveniently calling baseclass methods
  using Base::Base;                   // For inheriting baseclass constructor(s)

public:

  //-----------------------------------------------------------------------------------------------
  // \name Overrides

  /** Fills out the info declaring the in/out ports as having 2 channels, being the main ports and
  NOT supporting 64 bit processing. It also declares that in-place processing is allowed, assigns
  names "Stereo In", "Stereo Out" to the channels, gives the port the id zero and declares it as 
  a stereo pair. */
  bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) 
    const noexcept override;

  /** We override the process callback to handle the passed events and whenever an event is 
  received we call processEvent with that event. In between the events we call processBlockStereo
  with the pointers and numFrames variables adjusted for the sub-buffer to be processed. */
  clap_process_status process(const clap_process *process) noexcept override;

  //-----------------------------------------------------------------------------------------------
  // \name Callbacks to override by your subclass

  /** Your subclass must override this function to process one stereo block of in/out samples of
  length "numFrames". */
  virtual void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) = 0;

  //-----------------------------------------------------------------------------------------------
  // \name Inquiry

  /** A function to sanity-check a process buffer. We do this check at the start of our process
  function and return an error if it fails. This is a safety precaution just in case that some
  misbehaving host requests us to process buffers in an unsupported format or that we ourselves 
  falsely claimed to support a format which we actually can't handle. */
  static inline bool isProcessConfigSupported(const clap_process* p) noexcept
  {
    return p->audio_inputs_count             == 1  // Number of input ports must be 1
      &&   p->audio_outputs_count            == 1  // Number of output ports must be 1
      &&   p->audio_inputs[0].channel_count  == 2  // Number of input channels must be 2
      &&   p->audio_outputs[0].channel_count == 2  // Number of output channels must be 2
      &&   hasSinglePrecision(p);
  }

};

//=================================================================================================

/** UNDER CONSTRUCTION. ...seems to work already, though - needs some clean up and documentation

This class can serve as baseclass for instrument plugins, provided that they want to work with 
stereo I/O for audio in 32 bit. In addition to the baseclass ClapPluginStereo32Bit, this class
handles muscial events like note-on/off. Subclasses should respond to such events by overriding
the appropriate virtual functions such as noteOn, noteOff, etc. ...TBC... */

class ClapSynthStereo32Bit : public ClapPluginStereo32Bit
{

  using Base = ClapPluginStereo32Bit;   // For convenience
  using Base::Base;                     // Inherit baseclass constructor(s)

public:


  bool implementsNotePorts() const noexcept override { return true; }

  //bool implementsNotePorts() const noexcept override { return false; }
  // Test - OK - this fixes the crash - but that doesn't really help much


  uint32_t notePortsCount(bool isInput) const noexcept override { return isInput ? 1 : 0; }
  // One input note port, no output note ports. Do we need an output port when we want to send
  // NOTE_END events to the host? Figure out!

  bool notePortsInfo(uint32_t index, bool isInput,
    clap_note_port_info *info) const noexcept override;

  /** This is hook function that subclasses should override to respond to noteOn events. The key is
  in 0..127 like in MIDI 1.0, the velocity in 0..1 because this is the way, the CLAP_EVENT_NOTE_ON
  dialect communicates velocity via the clap_event_note struct. MIDI velocities in 1..127 can be
  easily translated by dividing by 127.0 but the CLAP velocity has higher resolution which we may 
  want to take advantage of someday.
  */
  virtual void noteOn(int key, double velocity) = 0;
  // Maybe have also optional parameters for channel, port_index, note_id (defaulting to -1 
  // indicating "unspecified" or "all")
  // The data type int16_t is taken from the clap-saw-example. I think it might be for 
  // compatibility with MIDI 2.0. See:
  // https://www.kvraudio.com/forum/viewtopic.php?t=567879
  // https://midi.org/midi-2-0-scope-a-development-and-test-tool-for-midi-2-0-messages
  //
  // ..but no - the clap-saw-demo uses int:
  // https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo.cpp#L771
  //
  // ...so maybe I should also use int. I already do in most of my other midi handling code anyway
  // so for easy compatibility, this would be the best type anyway.
  // ...maybe use int32_t instead of int to make it the same on all platforms?
  // Ahh - it was in clap_event_note - the note-key field there is of type int16_t
  // ...maybe settle for another format that is compatible with midi 1 and midi 2. Is that even
  // possible? Maybe we can support the microtuning extension somehow? That would be very cool!
  // Maybe note on events should have an additional data field for the frequency. We'll see...


  virtual void noteOff(int key) = 0;
  // Maybe let noteOff also have an (optional) off-velocity



  virtual void handleMidiEvent(const uint8_t midiDataBytes[3]);




  void processEvent(const clap_event_header_t* hdr) override;




protected:


};



