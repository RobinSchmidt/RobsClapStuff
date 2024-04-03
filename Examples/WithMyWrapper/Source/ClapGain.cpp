
#include "ClapGain.h"

// Some common strings for the plugin descriptors:
const char urlRsMet[]    = "https://rs-met.com";
const char vendorRsMet[] = "RS-MET";
const char version[]     = "2024.04.03";           // I use the YYYY.MM.DD format for versioning

//=================================================================================================
// StereoGainDemo

const char* const ClapGain::features[4] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_UTILITY, 
  CLAP_PLUGIN_FEATURE_MIXING, 
  NULL 
};
// Note that one of the unit tests checks this feature list. So if you change it, you need to 
// update the unit test, too. It's in runDescriptorReadTest in ClapPluginTests.cpp.

const clap_plugin_descriptor_t ClapGain::pluginDescriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.StereoGainDemo",
  .name         = "StereoGainDemo",
  .vendor       = vendorRsMet,
  .url          = urlRsMet,
  .manual_url   = urlRsMet,
  .support_url  = urlRsMet,
  .version      = version,
  .description  = "A simple gain to demonstrate writing a clap plugin.",
  .features     = ClapGain::features,

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
  //  also less error prone because in the code above, we must manually make sure that the "4" in 
  //  features[4] matches the number of initializers in the list. See:
  //  https://stackoverflow.com/questions/33270731/error-c4576-in-vs2015-enterprise
  //  https://github.com/swig/swig/issues/1851
  //  ...just removing "(const char *[])" gives a "too many initializers" error.
  //
  // -Maybe reanme it to NoGuiGain or similar. Rationale: I may later want to make versions with 
  //  GUIs of the same plugins. I'd like to have a collection of simple GUI-less plugins (like the 
  //  mda or airwindows plugins) in a single .dll and then I want to have a collection of plugins 
  //  with GUI (including ToolChain). Maybe call them RS-MET-PluginsWithGUI.clap and 
  //  RS-MET-PluginsNoGUI.clap. The plugins may overlap in functionality - but they should have 
  //  different ids nonetheless
  //
  // -For the version: define that in a central place so it can be used in several plugins. When 
  //  releasing new versions of plugins, we want to set this in one place and not for every plugin
  //  separately which is tedious and error prone. Do the same thing also for the vendor field and
  //  the various url fields
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
  //float amp   = (float) pow(10.0, (1.0/20)*params[kGain].value);       // dB to linear scaler
  float amp   = (float) RobsClapHelpers::dbToAmp(params[kGain].value); // dB to linear scaler
  float pan01 = (float) (0.5 * (params[kPan].value + 1.0));            // -1..+1  ->  0..1
  ampL = 2.f * (amp * (1.f - pan01));
  ampR = 2.f * (amp * pan01);

  // Notes:
  //
  // -One could use an optimized dB2amp formula that uses exp instead of pow. Maybe make a small
  //  library with such helper functions like amp2dB, dB2amp, pitch2freq, freq2pitch, etc.
}

bool ClapGain::paramsValueToText(clap_id paramId, double value, char *display, 
  uint32_t size) noexcept
{
  if(paramId == kGain)
  {
    int pos = sprintf_s(display, size, "%.2f", value);
    // For Bitwig, it is pointless to try to show more than 2 decimal digits after the point 
    // because they will be zero anyway - even with fine-adjustment using shift. ..However, we can
    // textually enter values with finer resolution

    // This is ugly - maybe use strcpy:
    display[pos+0] = ' ';
    display[pos+1] = 'd';
    display[pos+2] = 'B';
    display[pos+3] = '\0';
    // Also: factor this out into a function decibelsToString(double db, char* str, uint32_t size)
    // It needs to be made safe and unit tested - it should always zero-terminate the string and 
    // never write beyond size-1. Maybe make a function doubleToStringWithSuffix


    return true;
  }



  return Base::paramsValueToText(paramId, value, display, size);  // Preliminary

  // ToDo: 
  //
  // -Append a "dB" unit to the number in case of the "Gain" parameter - done - but this needs to
  //  be refactored
}

//=================================================================================================
// WaveShaperDemo

const char* const ClapWaveShaper::features[3] = 
{ 
  CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
  CLAP_PLUGIN_FEATURE_DISTORTION, 
  NULL 
};

const clap_plugin_descriptor_t ClapWaveShaper::pluginDescriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.WaveShaperDemo",
  .name         = "WaveShaperDemo",
  .vendor       = vendorRsMet,
  .url          = urlRsMet,
  .manual_url   = urlRsMet,
  .support_url  = urlRsMet,
  .version      = version,
  .description  = "A simple waveshaper",
  .features     = ClapWaveShaper::features,
};

ClapWaveShaper::ClapWaveShaper(const clap_plugin_descriptor *desc, const clap_host *host) 
  : ClapPluginStereo32Bit(desc, host) 
{
  clap_param_info_flags automatable = CLAP_PARAM_IS_AUTOMATABLE;
  clap_param_info_flags choice      = CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;

  addParameter(kShape, "Shape",   0.0,   5.0, 0.0, choice     );   // Clip, Tanh, etc.
  addParameter(kDrive, "Drive", -20.0, +60.0, 0.0, automatable);   // In dB
  addParameter(kDC,    "DC",    -10.0, +10.0, 0.0, automatable);   // As raw offset
  addParameter(kGain,  "Gain",  -60.0, +20.0, 0.0, automatable);   // In dB

  // ToDo:
  //
  // -Check, if we can tell the host that the shape parameter is a choice parameter.
  // -Override the value-to-text function
  // -DC needs greater range. And/or maybe it's more convenient to apply DC before the drive?
}

void ClapWaveShaper::setParameter(clap_id id, double newValue)
{
  Base::setParameter(id, newValue);

  using namespace RobsClapHelpers;

  switch(id)
  {
  case kShape: { shape  = (int)           newValue;  } break;
  case kDrive: { inAmp  = (float) dbToAmp(newValue); } break;
  case kDC:    { dc     = (float)         newValue;  } break;
  case kGain:  { outAmp = (float) dbToAmp(newValue); } break;
  default:
  {
    // error("Unknown parameter id in ClapWaveShaper::setParameter");
  }
  }
}




void ClapWaveShaper::processBlockStereo(
  const float* inL, const float* inR, float* outL, float* outR, uint32_t numFrames)
{
  for(uint32_t n = 0; n < numFrames; ++n)
  {
    outL[n] = applyDistortion(inL[n]);
    outR[n] = applyDistortion(inR[n]);
  }
}






//=================================================================================================
/*

ToDo

-Add waveshaper shapes: clip, tanh, atan, asinh, erf, x / (1 + |x|), x / sqrt(1 + x*x), 
 x / (1 + x^2), sin, cbrt



*/