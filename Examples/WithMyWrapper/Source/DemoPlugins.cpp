
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
  RobsClapHelpers::clapAssert(areParamsConsistent());

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

  RobsClapHelpers::clapAssert(areParamsConsistent());

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


  //// Test:
  //for(uint32_t n = 0; n < numFrames; ++n)
  //  outL[n] = outR[n] = 0.0;
}

void ClapToneGenerator::parameterChanged(clap_id id, double newValue)
{

}

void ClapToneGenerator::noteOn(int key, double vel)
{
  /*
  currentKey = key;
  double freq = RobsClapHelpers::pitchToFreq((double) currentKey);
  increment = freq / getSampleRate();
  */

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
  /*
  if(key == currentKey)
  {
    reset();            // Sets currentKey = 0 and phasor = 0.0
    increment  =  0.0;  // Not really relevant but for tidiness
  }
  */
}

/*

ToDo:
-The ClapToneGenerator currently has 3 failing tests in the clap-validator. It crashes in those 
 tests(!).
-It also crashes in Bitwig when playing clusters of notes.
-To find it, replace all the assert statements with some "clapAssert" which triggers a debug
 breakpoint and then attach to the BitwigHost process.
-It seems also to happen when playing two notes simultaneously - hit two notes with two fingers
 exactly simultaneously
-Commenting out our code in noteOn/Off doesn't help

-> Replace all asserts by some custom clapAssert or assume/ensure/verify that triggers a debug 
   breakpoint. Then let's see, if we can trigger it from Bitwig

...oookay - the debug-break triggers in ClapPluginStereo32Bit::process when it tries to call
processBlockStereo


ClapToneGenerator::processBlockStereo gets called with ridiculusly large numFrames from 
ClapPluginStereo32Bit::process. There, we had the following data whit it happened in a few trials:

eventIndex     =   1     1    1    1    1
frameIndex     =  61   426  474  449   71
nextEventFrame =  46   326  321  301   54
numEvents      =   2     2    3    2    2
numFrames      = 480   480  480  480  480

It looks like it happens whenever there are more than 1 event per block and/or when 
frameIndex > nextEventFrame. How can this happen? Are the event unordered? Write a unit test that
produces some test audio- and event buffers



*/