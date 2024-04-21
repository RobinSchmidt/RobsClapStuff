
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
  clap_param_info_flags flags = CLAP_PARAM_IS_AUTOMATABLE;

  //reserveParameters(numParams);

  addFloatParameter(kGain, "Gain", -40.0, +40.0, 0.0, flags, 2, " dB");
  addFloatParameter(kPan,  "Pan",   -1.0,  +1.0, 0.0, flags, 3, "");

  RobsClapHelpers::clapAssert(areParamsConsistent());

  // Notes:
  //
  // -If the "automatable" flag is not set, Bitwig will not show a knob for the respective 
  //  parameter on the generated GUI. Apparently, knobs are only provided for automatable 
  //  parameters.
  // -The 2 and 3 argumens after the "flags" give the number of decimal digits after the dot 
  // -The strings " dB" and "" give a suffix for a physical unit to be appended to the formatted
  //  value. It can be empty, of course. You can also pass nothing at all in this case instead of
  //  an empty string.
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
  clap_param_info_flags flags = CLAP_PARAM_IS_AUTOMATABLE;
  //clap_param_info_flags choice      = flags | CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;

  //reserveParameters(numParams);
  addChoiceParameter(kShape, "Shape", 0, numShapes-1, 0, flags, 
    { "Clip", "Tanh", "Atan", "Erf" });
  addFloatParameter( kDrive, "Drive", -20.0, +60.0, 0.0, flags, 2, " dB");
  addFloatParameter( kDC,    "DC",    -10.0, +10.0, 0.0, flags, 3, "");
  addFloatParameter( kGain,  "Gain",  -60.0, +20.0, 0.0, flags, 2, " dB");
  RobsClapHelpers::clapAssert(areParamsConsistent());
 
  // We would actually like to call it like this:
  //addChoiceParameter(kShape, "Shape", "Clip", automatable, { "Clip", "Tanh", "Atan", "Erf" });
  // ...or maybe not?

  // Notes:
  //
  // -I actually didn't want to make the shape automatable but when not setting the flag, the
  //  parameter doesn't appear at all in generic GUI that Bitwig provides. Apparently, Bitwig shows
  //  only knobs for automatable parameters. Why? Is this intentional? Probably not. Maybe report 
  //  this here:
  //  https://github.com/free-audio/interop-tracker/issues
  // -The addChoiceParameter function will automatically also set the flags CLAP_PARAM_IS_STEPPED, 
  //  CLAP_PARAM_IS_ENUM so we don't need to do it here.
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

const char* const ClapToneGenerator::features[3] = 
{ 
  CLAP_PLUGIN_FEATURE_INSTRUMENT,
  CLAP_PLUGIN_FEATURE_SYNTHESIZER, 
  NULL 
};

const clap_plugin_descriptor_t ClapToneGenerator::descriptor = 
{
  .clap_version = CLAP_VERSION_INIT,
  .id           = "RS-MET.ToneGeneratorDemo",
  .name         = "ToneGeneratorDemo",
  .vendor       = vendorRsMet,
  .url          = urlRsMet,
  .manual_url   = urlRsMet,
  .support_url  = urlRsMet,
  .version      = version,
  .description  = "MIDI-controlled sinusoidal tone generator",
  .features     = ClapToneGenerator::features,
};

ClapToneGenerator::ClapToneGenerator(const clap_plugin_descriptor *desc, const clap_host *host) 
  : ClapSynthStereo32Bit(desc, host) 
{

  // ToDo:
  //
  // -Add parameters
}

bool ClapToneGenerator::activate(
  double newSampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
{
  reset();
  return true;

  // ToDo:
  //
  // -Maybe we should check, if the passed sampleRate is valid (i.e. > 0) and return false if not?
  //  A sampleRate < 0 is clearly a bug in the host though.
}

void ClapToneGenerator::deactivate() noexcept
{
  reset();

  // Notes:
  //
  // -It may be a bit redundant to call reset() in activate() and deactivate() but it doesn't hurt
  //  and is a bit cleaner to be in a well defined state after every activation and deactivation.
  //  But if only one of the calls shall remain, I think, it should be the one in deactivate. 
  //  Before the very first activation, the plugin will be in its initial state anyway
}

void ClapToneGenerator::reset() noexcept
{
  phasor = 0.0;
  currentKey = -1;
}

void ClapToneGenerator::processBlockStereo(
  const float* inL, const float* inR, float* outL, float* outR, uint32_t numFrames)
{
  for(uint32_t n = 0; n < numFrames; ++n)
    outL[n] = outR[n] = getSample();
}

void ClapToneGenerator::parameterChanged(clap_id id, double newValue)
{
  // We don't have any parameters yet, but this function is a mandatory override (i.e. purely 
  // virtual) ...and we want to add parameters later anyway...
}

void ClapToneGenerator::noteOn(int key, double vel)
{
  currentKey = key;
  double freq = RobsClapHelpers::pitchToFreq((double) currentKey);
  increment = freq / getSampleRate();

  // ToDo:
  //
  // -Use vel and ampByVel to compute an amplitude scaler.
  // -Maybe factor out an calcIncrement function that takes into account more variables like a 
  //  detune parameter and pitch-bend
  // -Maybe clip the increment to the range 0..0.5. That should translate to frequencies in
  //  0..sampleRate/2
  // -What if sampleRate == 0? That should be the case only in deactivated state in which we 
  //  probably should not receive calls to noteOn anyway, but still...
}

void ClapToneGenerator::noteOff(int key)
{
  if(key == currentKey)
  {
    reset();          // Sets currentKey = -1 and phasor = 0.0
    increment = 0.0;  // Not really relevant (I think) but let's be tidy
  }
}
