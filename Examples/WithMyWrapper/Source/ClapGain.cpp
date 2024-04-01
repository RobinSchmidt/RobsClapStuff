
#include "ClapGain.h"


const char* const ClapGainFeatures[3] = { CLAP_PLUGIN_FEATURE_UTILITY, 
CLAP_PLUGIN_FEATURE_MIXING, NULL };
// ToDo: try to get rid of this or at least, move it as member into ClapGain

const clap_plugin_descriptor_t ClapGain::pluginDescriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id = "RS-MET.StereoGain",   // rename to "Gain" ...maybe have a NoGui qualifier..NoGuiGain
  .name = "StereoGain",
  .vendor = "RS-MET",
  .url = "https://rs-met.com",
  .manual_url = "https://rs-met.com",
  .support_url = "https://rs-met.com",
  .version = "2024.04.01",                // I use the YYYY.MM.DD format for versioning
  .description = "A simple gain to demonstrate writing a clap plugin.",
  .features = ClapGainFeatures,
  //.features = ClapGain::features,
};
// ToDo: try to use a syntax like:
//
// .features = (const char *[]) { CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_MIXING, NULL },
//
// like it was originally in the nakst example (on which this code is based). But with this syntax,
// it doesn't compile in Visual Studio 2019. Figure out why. It would be cleaner to have everything
// in one place and not litter the global namespace with the features array. It's also less error 
// prone because in the code above, we must manually make sure that the "3" in features[3] matches 
// the number of initializers in the list.


ClapGain::ClapGain(const clap_plugin_descriptor *desc, const clap_host *host) 
  : ClapPluginStereo32Bit(desc, host) 
{
  // Add the parameters:
  clap_param_info_flags flags = CLAP_PARAM_IS_AUTOMATABLE;
  addParam(kGain, "Gain", -40.0, +40.0, 0.0, flags);  // in dB
  addParam(kPan,  "Pan",   -1.0,  +1.0, 0.0, flags);  // -1: hard left, 0: center, +1: hard right

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
  ampL = amp * (1.f - pan01);
  ampR = amp * pan01;

  // Verify formulas!

  // Notes:
  //
  // -One could use an optimized dB2amp formula that uses exp instead of pow. Maybe make a small
  //  library with such helper functions like amp2dB, dB2amp, pitch2freq, freq2pitch, etc.
}






//=================================================================================================
/*

ToDo

-Bitwig gives an error about failure to save the state - this is expected because we don't 
 implement state saving yet. Ah - it was because implementsState returned true but stateSave
 returned false. ..might be fixed now (-> test it!)


*/