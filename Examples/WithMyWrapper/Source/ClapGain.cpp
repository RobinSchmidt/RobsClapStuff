
#include "ClapGain.h"


const char* const ClapGain::features[4] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_UTILITY, 
  CLAP_PLUGIN_FEATURE_MIXING, 
  NULL 
};
// Note that one of the unit tests checks the feature list. So if you change it, you need to update
// the unit test, too. It's in runDescriptorReadTest in ClapPluginTests.cpp.

const clap_plugin_descriptor_t ClapGain::pluginDescriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.StereoGain",   // rename to "Gain" ...maybe have a NoGui qualifier..NoGuiGain
  .name         = "StereoGain",
  .vendor       = "RS-MET",
  .url          = "https://rs-met.com",
  .manual_url   = "https://rs-met.com",
  .support_url  = "https://rs-met.com",
  .version      = "2024.04.01",           // I use the YYYY.MM.DD format for versioning
  .description  = "A simple gain to demonstrate writing a clap plugin.",
  .features     = ClapGain::features,
  //.features = (const char *[]) { CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_MIXING, NULL },


  // ToDo: 
  //
  // -Try to use a syntax like:
  //
  //  .features = (const char *[]) { CLAP_PLUGIN_FEATURE_UTILITY, 
  //                                 CLAP_PLUGIN_FEATURE_MIXING, NULL },
  //
  //  like it was originally in the nakst example (on which this code is based). But with this 
  //  syntax, it doesn't compile in Visual Studio 2019. Figure out why. It would be cleaner to have 
  //  everything in one place and not litter the class declaration with the features array. It's 
  //  also less error prone because in the code above, we must manually make sure that the "3" in 
  //  features[3] matches the number of initializers in the list.
  //
  // -Maybe reanme it to NoGuiGain or similar. Rationale: I may later want to make versions with 
  //  GUIs of the same plugins. I'd like to have a collection fo simple GUI-less plugins (like the 
  //  mda or airwindows plugins) in a single .dll and then I want to have a collection of plugins 
  //  with GUI (including ToolChain). Maybe call them RS-MET-PluginsWithGUI.clap and 
  //  RS-MET-PluginsNoGUI.clap. The plugins may overlap in functionality - but they should have 
  //  different ids nonetheless
  //
  // -For the version: define that in a central place so it can be used in several plugins. When 
  //  releasing new versions of plugins, we want to set this in one place and not for every plugin
  //  separately which is tedious and error prone.
};



ClapGain::ClapGain(const clap_plugin_descriptor *desc, const clap_host *host) 
  : ClapPluginStereo32Bit(desc, host) 
{
  // ToDo: Maybe one of these checks could make sense:
  // assert(*desc ==  pluginDescriptor);
  // assert( desc == &pluginDescriptor);
  // The former would just assert that the content of the descriptor matches while the second 
  // actually asserts that a pointer to our own static member is passed, i.e. the second check 
  // would be stronger. 


  // Add the parameters:
  clap_param_info_flags flags = CLAP_PARAM_IS_AUTOMATABLE;
  //flags |= CLAP_PARAM_REQUIRES_PROCESS;  // Note sure, if we need this -> figure out!
  // ...naaah - it's probably not needed

  addParameter(kGain, "Gain", -40.0, +40.0, 0.0, flags);  // in dB
  addParameter(kPan,  "Pan",   -1.0,  +1.0, 0.0, flags);  // -1: left, 0: center, +1: right

  // Notes:
  //
  // -It is important to add the parameters in the correct order, i.e. in the same order as they 
  //  appear in the enum. The id given to the parameter will also equal the enum value and will 
  //  equal the index at which it is stored in our inherited params array. Later, this restriction
  //  may (or may not) be lifted. Doing so will require more complex code in ClapPluginWithParams. 
  //  At the moment, we only have a simple impementation.
  // -Maybe we should also set the CLAP_PARAM_REQUIRES_PROCESS flag. The clap doc says that when 
  //  this flag is set, the parameter events will be passed via process. Without the flag, that 
  //  seem not to be ensured - although Bitwig seems to do it anyway. The plugin works even without
  //  the flag - but maybe that's only so in Bitwig? Figure out!
}

void ClapGain::processBlockStereo(const float* inL, const float* inR, float* outL, float* outR,
  uint32_t numFrames)
{
  for(uint32_t n = 0; n < numFrames; ++n)
  {
    outL[n] = ampL * inL[n];
    outR[n] = ampR * inR[n];
  }
}

void ClapGain::setParameter(clap_id id, double newValue)
{
  // This stores the new value in our inherited params array:
  Base::setParameter(id, newValue);

  // Compute the internal algorithm coeffs from the user parameters:
  float amp   = (float) pow(10.0, (1.0/20)*params[kGain].value); // dB to linear scaler
  float pan01 = (float) (0.5 * (params[kPan].value + 1.0));      // -1..+1  ->  0..1
  ampL = 2.f * (amp * (1.f - pan01));
  ampR = 2.f * (amp * pan01);

  // Verify formulas!

  // Notes:
  //
  // -One could use an optimized dB2amp formula that uses exp instead of pow. Maybe make a small
  //  library with such helper functions like amp2dB, dB2amp, pitch2freq, freq2pitch, etc.
}






//=================================================================================================
/*

ToDo


It currently fails 2 tests in the clap-validator (and passes 13, skips 6):

- param-set-wrong-namespace: Sends events to the plugin with the 'CLAP_EVENT_PARAM_VALUE' event 
  tyep but with a mismatching namespace ID. Asserts that the plugin's parameter values don't 
  change. 
  FAILED: Sending events with type ID 5 (CLAP_EVENT_PARAM_VALUE) and namespace ID 0xb33f to 
  the plugin caused its parameter values to change. This should not happen. The plugin may not be 
  checking the event's namespace ID.

- state-reproducibility-flush: Randomizes a plugin's parameters, saves its state, recreates the 
  plugin instance, sets the same parameters as before, saves the state again, and then asserts that 
  the two states are identical. The parameter values are set updated using the process function to 
  create the first state, and using the flush function to create the second state.  
  FAILED: 'clap_plugin_params::flush()' has been called with random parameter values, but the 
  plugin's reported parameter values have not changed.


-> We need check the parameter evet's namespace ID (whatever that is)
-> We need to implement clap_plugin_params::flush()






-About double-string-double roundtrips (move to other file):
 https://stackoverflow.com/questions/29200635/convert-float-to-string-with-precision-number-of-decimal-digits-specified
 https://stackoverflow.com/questions/332111/how-do-i-convert-a-double-into-a-string-in-c
 https://en.cppreference.com/w/cpp/utility/to_chars
 https://github.com/nothings/stb/blob/master/stb_sprintf.h
 https://cplusplus.com/reference/cstdlib/strtod/
 https://stackoverflow.com/questions/2302969/convert-a-float-to-a-string
 https://stackoverflow.com/questions/62661223/sprintf-formatting-problem-for-doubles-with-high-precision
 https://github.com/google/double-conversion
 https://github.com/jk-jeon/Grisu-Exact
 https://github.com/ulfjack/ryu
 https://possiblywrong.wordpress.com/2015/06/21/floating-point-round-trips/

 https://en.cppreference.com/w/cpp/types/numeric_limits/max_digits10 Has very short function








*/