
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

  // Note that one of the unit tests checks this feature list. So if you change it, you need to 
  // update the unit test, too - or else it will fail. It's in runDescriptorReadTest in 
  // ClapPluginTests.cpp.
};

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
  .description  = "Stereo gain and panning",
  .features     = ClapGain::features,
};

ClapGain::ClapGain(const clap_plugin_descriptor *desc, const clap_host *host) 
  : ClapPluginStereo32Bit(desc, host) 
{
  // Flags for our parameters - they are automatable:
  clap_param_info_flags automatable = CLAP_PARAM_IS_AUTOMATABLE;

  // Add the parameters:
  addParameter(kGain, "Gain", -40.0, +40.0, 0.0, automatable);  // in dB
  addParameter(kPan,  "Pan",   -1.0,  +1.0, 0.0, automatable);  // -1: left, 0: center, +1: right

  // Notes:
  //
  // -It is important to add the parameters in the correct order, i.e. in the same order as they 
  //  appear in the enum. The id given to the parameter will also equal the enum value and will 
  //  equal the index at which it is stored in our inherited params array. Later, this restriction
  //  may (or may not) be lifted. Doing so will require more complex code in ClapPluginWithParams. 
  //  At the moment, we only have a simple impementation.
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

void ClapGain::parameterChanged(clap_id id, double newValue)
{
  // This stores the new value in our inherited params array and needs to be called:
  //Base::setParameter(id, newValue);
  // not needed anymore

  float amp   = (float) RobsClapHelpers::dbToAmp(getParameter(kGain)); // dB to linear scaler
  float pan01 = (float) (0.5 * (getParameter(kPan) + 1.0));            // -1..+1  ->  0..1
  ampL = 2.f * (amp * (1.f - pan01));
  ampR = 2.f * (amp * pan01);

  // ToDo:
  //
  // -Maybe avoid having to call the baseclass method by letting the baseclass method calling a
  //  new function parameterChanged(id, newValue). That is safer because calling the baseclass 
  //  method is easy to forget.
  // -Maybe instead of accessing the parameters via params[kGain].value, use a function 
  //  getParameter(kGain), etc. That looks cleaner.
}

bool ClapGain::paramsValueToText(clap_id id, double val, char *buf, uint32_t len) noexcept
{
  switch(id)
  {
  case kGain: { return toDisplay(val, buf, len, 2, " dB"); }
  case kPan:  { return toDisplay(val, buf, len, 3       ); } 
  }
  return Base::paramsValueToText(id, val, buf, len);
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
  .description  = "Waveshaper with various shapes",
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
  // -Override the value-to-text function
  // -Maybe it's more convenient to apply DC before the drive? With quite high drive, modulating DC
  //  is like PWM. But the "good" range for DC depends on the amount of drive, if DC is applied 
  //  after the drive (higher drive allower for a higher DC range with making the signal disappear)
  //  That is not so nice. It would be better, if the good range for DC would be independent of 
  //  drive.
}

void ClapWaveShaper::parameterChanged(clap_id id, double newValue)
{
  //Base::setParameter(id, newValue);

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

-Try to use a syntax for the features field like:
 
   .features = (const char *[]) { CLAP_PLUGIN_FEATURE_UTILITY, 
                                  CLAP_PLUGIN_FEATURE_MIXING, NULL },
 
 like it was originally in the nakst example (on which this code is based). But with this 
 syntax, it doesn't compile in Visual Studio 2019. Figure out why. It would be cleaner to have 
 everything in one place and not litter the class declaration with the features array. It's 
 also less error prone because in the code above, we must manually make sure that the "4" in 
 features[4] matches the number of initializers in the list. See:
 https://stackoverflow.com/questions/33270731/error-c4576-in-vs2015-enterprise
 https://github.com/swig/swig/issues/1851
 https://en.cppreference.com/w/c/language/struct_initialization
 ...just removing "(const char *[])" gives a "too many initializers" error.

-Figure out if in the constructors one of these checks could make sense:
   assert(*desc ==  pluginDescriptor);
   assert( desc == &pluginDescriptor);
 The former would just assert that the content of the descriptor matches while the second 
 actually asserts that a pointer to our own static member is passed, i.e. the second check 
 would be stronger. 

-Check, if we need the CLAP_PARAM_REQUIRES_PROCESS flag for the parameters. I don't thinks so, 
 though - but figure out and document under which circumstances this will be needed. The clap doc 
 says that when this flag is set, the parameter events will be passed via process. Without the 
 flag, that seem not to be ensured - they may be passed via flush... - although Bitwig seems to do
 it via process anyway. Or does it? Check in the debugger! The plugin works even without the flag.
 But maybe that's only so in Bitwig? Figure out!

-A way to reduce the boilerplate even more would be to let the ClapPluginParameter class have two
 function pointers for the conversion to string and for a callback that can be called whenever the
 value changes. That would require making the value private and provide a setValue function that
 sets the value and then calls the callback. Then, instead of overriding setParameter and
 paramsValueToText, we would just assign these two additional members in the constructor. But for 
 this callback mechanism to really reduce boilerplate, we assume that appropriate callback target
 functions already exist anyway - if they don't, they would have to written which is again more 
 boilerplate. However, when using classes from my rapt/rosic DSP libraries, these functions do 
 indeed already exist (like setCutoff, setResonance, etc.). For my own use cases, that might be 
 fine but in order to be more generally useful, that approach might be a bit too opinionated.


*/