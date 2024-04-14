
#include "DemoPlugins.h"

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

  // Notes:
  //
  // -One of the unit tests checks this feature list. So if you change it, you need to  update the
  //  unit test, too - or else it will fail. It's in runDescriptorReadTest in ClapPluginTests.cpp.
  // -It is important to have at least one of the main categories CLAP_PLUGIN_FEATURE_INSTRUMENT, 
  //  ..._AUDIO_EFFECT, ..._NOTE_EFFECT, ..._NOTE_DETECTOR, ..._ANALYZER set. Otherwise the clap 
  //  validator will complain. The other ones are optional and for further, finer specification.
  //
  // ToDo:
  //
  // -Maybe add feature CLAP_PLUGIN_FEATURE_STEREO
};


const clap_plugin_descriptor_t ClapGain::descriptor = 
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
  assert(areParamsConsistent());

  // Notes:
  //
  // -If the "automatable" flag is not set, Bitwig will not show a knob for the respective 
  //  parameter on the generated GUI. Apparently, knobs are only provided for automatable 
  //  parameters.
  // -The areParamsConsistent assertion checks that my convention for mapping back and forth 
  //  between parameter-index and parameter-identifier is obeyed. This will be ensured, if you use
  //  each of the identifiers from the ParamId enum once and only once in a call to addParameter.
  //
  //  ToDo: 
  //
  //  -Maybe pass numParams to the check and let it check, if the number of paramters matches. At 
  //   the momemt, the check may return a false positive, if param-ids from the end are missing 
  //   (I think -> verify -> maybe comment out the second addParameter call and see if the 
  //   assertion triggers - I think, it will not but should)
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
  float amp   = (float) RobsClapHelpers::dbToAmp(getParameter(kGain)); // dB to linear scaler
  float pan01 = (float) (0.5 * (getParameter(kPan) + 1.0));            // -1..+1  ->  0..1
  ampL = 2.f * (amp * (1.f - pan01));
  ampR = 2.f * (amp * pan01);
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

const clap_plugin_descriptor_t ClapWaveShaper::descriptor = 
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
  clap_param_info_flags choice      = automatable | CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;

  //reserveParameters(numParams);
  addParameter(kShape, "Shape",   0.0, numShapes-1, 0.0, choice);        // Clip, Tanh, etc.
  shapeNames = { "Clip", "Tanh", "Atan", "Erf" };

  addParameter(kDrive, "Drive", -20.0, +60.0,       0.0, automatable);   // In dB
  addParameter(kDC,    "DC",    -10.0, +10.0,       0.0, automatable);   // As raw offset
  addParameter(kGain,  "Gain",  -60.0, +20.0,       0.0, automatable);   // In dB

  assert(areParamsConsistent());

  // Notes:
  //
  // -I actually didn't want to make the shape automatable but when not setting the flag, the
  //  parameter doesn't appear at all in generic GUI that Bitwig provides. Apparently, Bitwig shows
  //  only knobs for automatable parameters. Why? Is this intentional? Probably not. Maybe report 
  //  this here:
  //  https://github.com/free-audio/interop-tracker/issues
  // -It is important that the order of the shape strings ("Clip", "Tanh", ...) matches the order 
  //  of the corresponding entries in the Shapes enum. There is a unit test that verifies this. If 
  //  we make the choices for enum parameters automatable, then the order of the initial section of 
  //  the enum must remain stable from version to version, i.e. we cannot insert new options at 
  //  arbitrary positions in later versions. New options can only be added at the end. That's why I
  //  actually didn't want to make the shape automatable, btw.
}

void ClapWaveShaper::parameterChanged(clap_id id, double newValue)
{
  using namespace RobsClapHelpers;
  switch(id)
  {
  case kShape: { shape  = (Shape)(int) round(  newValue); } break;  // use roundToInt(newValue)
  case kDrive: { inAmp  = (float)      dbToAmp(newValue); } break;
  case kDC:    { dc     = (float)              newValue;  } break;
  case kGain:  { outAmp = (float)      dbToAmp(newValue); } break;
  default:
  {
    // error("Unknown parameter id in ClapWaveShaper::setParameter");
  }
  }
}

bool ClapWaveShaper::paramsValueToText(clap_id id, double val, char *buf, uint32_t len) noexcept
{
  switch(id)
  {
  case kShape: { return toDisplay(val, buf, len, shapeNames); }
  case kDrive: { return toDisplay(val, buf, len, 2, " dB");   }
  case kGain:  { return toDisplay(val, buf, len, 2, " dB");   }
  }
  return Base::paramsValueToText(id, val, buf, len);  // Fall back to default if not yet handled
}

bool ClapWaveShaper::paramsTextToValue(
  clap_id id, const char* display, double* value) noexcept
{
  if(id == kShape)
    return toValue(display, value, shapeNames);
  return Base::paramsTextToValue(id, display, value);

  // Notes:
  //
  // -The clap-validator app requires that we can also correctly map from a display-string back to
  //  a value. I don't really know what the use-case for this mapping is in the case of choice 
  //  parameters, but to satisfy the validator, we need to implement it.
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

float ClapWaveShaper::applyDistortion(float x)
{
  using namespace RobsClapHelpers;                   // Needed for clip
  static const float pi2  = 1.5707963267948966192f;  // pi/2, needed for atan
  static const float pi2r = 1.f / pi2;               // Reciprocal of pi/2

  float y = inAmp * x + dc;                          // Intermediate
  switch(shape)
  {
  case kClip: y = clip(y, -1.f, +1.f); break;
  case kTanh: y = tanh(y);             break;
  case kAtan: y = pi2r * atan(pi2*y);  break;
  case kErf:  y = erf(y);              break;
  default:    y = 0.f;                 break;        // Error! Unknown shape. Return 0.
  }
  return outAmp * y;
}

//=================================================================================================


void ClapToneGenerator::noteOn(int16_t key, double vel)
{

}

void ClapToneGenerator::noteOff(int16_t key)
{

}