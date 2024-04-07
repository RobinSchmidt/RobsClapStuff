
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

  // Note that one of the unit tests checks this feature list. So if you change it, you need to 
  // update the unit test, too - or else it will fail. It's in runDescriptorReadTest in 
  // ClapPluginTests.cpp.
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

  // Notes:
  //
  // -It is important to add the parameters in the correct order, i.e. in the same order as they 
  //  appear in the enum. The id given to the parameter will also equal the enum value and will 
  //  equal the index at which it is stored in our inherited params array. Later, this restriction
  //  may (or may not) be lifted. Doing so will require more complex code in ClapPluginWithParams. 
  //  At the moment, we only have a simple impementation.
  // -If the "automatable" flag is not set, Bitwig will not show a knob for the respective 
  //  parameter on the generated GUI. Apparently, knobs are only provided for automatable 
  //  parameters.
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

  addParameter(kShape, "Shape",   0.0, numShapes-1, 0.0, choice);        // Clip, Tanh, etc.
  shapeNames = { "Clip", "Tanh", "Atan", "Erf" };

  addParameter(kDrive, "Drive", -20.0, +60.0,       0.0, automatable);   // In dB
  addParameter(kDC,    "DC",    -10.0, +10.0,       0.0, automatable);   // As raw offset
  addParameter(kGain,  "Gain",  -60.0, +20.0,       0.0, automatable);   // In dB

  // Notes:
  //
  // -I actually didn't want to to make the shape automatable but when not setting the flag, the
  //  parameter doesn't appear at all in generic GUI that Bitwig provides. Apparently, Bitwig shows
  //  only knobs for automatable parameters. ...why?
  // -It is important that the order of the shape strings matches the order of the corresponding 
  //  entries in the Shapes enum. There is a unit test that verifies this.
}

void ClapWaveShaper::parameterChanged(clap_id id, double newValue)
{
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

bool ClapWaveShaper::paramsValueToText(clap_id id, double val, char *buf, uint32_t len) noexcept
{
  using namespace RobsClapHelpers;
  switch(id)
  {
  case kShape: { return copyString(shapeNames, (int) val, buf, len);           }
  case kDrive: { return toDisplay(                   val, buf, len, 2, " dB"); }
  case kGain:  { return toDisplay(                   val, buf, len, 2, " dB"); }
  }
  return Base::paramsValueToText(id, val, buf, len);  // Fall back to default if not yet handled

  // ToDo:
  // 
  // -Figure out if it is better to use some sort of roundToInt function instead of just doing
  //  "(int) val" for the shape. This usage should be consistent with what we do in 
  //  parameterChanged. Maybe test the behavior also when we don't set the flags
  //  CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;  Maybe some hosts ignore these flags and maybe
  //  in those, we may get better behavior when we round? I guess, when we do truncation, the last
  //  option will be reached only in total hard-right position of the knob?
  // -Maybe make an overload of the toDisplay function that we can call like:
  //  toDisplay(val, buf, len, shapeNames)
}

bool ClapWaveShaper::paramsTextToValue(
  clap_id id, const char* display, double* value) noexcept
{
  if(id == kShape)
  {
    // Factor this out into a function  bool toValue(display, value, shapeNames)
    using namespace RobsClapHelpers;
    int shapeIndex = findString(shapeNames, display);
    if(shapeIndex == -1)
    {
      *value = 0.0;
      return false;
    }
    else
    {
      *value = (double) shapeIndex;
      return true;
    }
  }


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