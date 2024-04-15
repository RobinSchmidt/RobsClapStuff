#pragma once

#include "../../../RobsClapHelpers/RobsClapHelpers.h"


/** A very simple gain plugin whose purpose is to demonstrate the usage of class 
ClapPluginStereo32Bit. The purpose of this class is to enable writing of common stereo plugins with
32 bit processing with only a small amount of boilerplate code. To write a clap plugin based on 
ClapPluginStereo32Bit, you need to do the following things:

(1) Derive your plugin class from RobsClapHelpers::ClapPluginStereo32Bit
(2) Declare your parameters in some sort of "ParamId" enum.
(3) Implement the constructor. It should populate the inherited parameter array using addParameter.
(4) Override parameterChanged to handle parameter changes.
(5) Override processBlockStereo to process (sub) blocks of audio sample frames.
(6) Fill the features array.
(7) Fill the pluginDescriptor.

To make one or more plugins visible to the host, you also have to implement the factory and entry 
point. This is shown in DemoPluginsEntryPoint.cpp. */

class ClapGain : public RobsClapHelpers::ClapPluginStereo32Bit
{

  using Base = ClapPluginStereo32Bit; // For conveniently calling baseclass methods

public:

  /** Enumeration of the parameters. These values are used for the param_id. The values must start 
  at 0 and count continuously up. The order must be stable between plugin versions (or else, you'll
  break state recall) and new parameters can only be added at the end. That's pretty much the 
  behavior we know from VST 1/2 when not using the "chunks" mechanism for the state. That's why I 
  also use the convention with the k-prefix - if you know VST 1/2, that should all look rather 
  familiar. The difference to the VST way of handling it is that here the order of the identifiers 
  does not necessarily correspond to the order of the knobs/sliders that the host-generated GUI 
  will present. That order is determined by the order of our addParameter() calls in our 
  constructor. Generally, each pameter has an index and an id. The index determines the order in
  host-generated GUI. In CLAP, the id can be any (unique) number. Here in this framework we use the
  convention that the ids must be a permutation of the indices. This makes the back-and-forth 
  mapping between indices and ids easy and efficient. */
  enum ParamId
  {
    kGain,
    kPan,
    //kMono,     // Maybe add this as a boolean parameter for a mono switch

    numParams
  };

  /** Constructor. It populates our inherited array of parameters using calls to addParameter. */
  ClapGain(const clap_plugin_descriptor *desc, const clap_host *host);

  /** This is an overriden callback that gets called from inside the process method of our 
  basclass. This baseclass method is responsible for interleaving the calls to setParameter and 
  calls to processBlockStereo to achieve sample-accurate automation of parameters. The call to 
  setParameter will, among other things, trigger a call to parameterChanged which our subclass 
  needs to override to take appropriate actions like recalculating internal DSP coefficients. */
  void parameterChanged(clap_id id, double newValue) override;

  /** Converts a parameter value to text for display on the generic GUI that the host provides for
  GUI-less plugins. */
  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;

  /** Overrides the sub-block processing function to do the actual signal processing. */
  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;


  // This is needed for our plugin descriptor:
  static const char* const features[4];
  static const clap_plugin_descriptor_t descriptor;
  // This is still somewhat unelegant - try to get rid!


protected:


  // Internal algorithm coefficients:
  float ampL = 1.f, ampR = 1.f;          // Gain factors for left and right channel

};

//=================================================================================================

/** A simple waveshaper with various shapes to choose from. */

class ClapWaveShaper : public RobsClapHelpers::ClapPluginStereo32Bit
{

  using Base = ClapPluginStereo32Bit;

public:

  //-----------------------------------------------------------------------------------------------
  // \name Boilerplate

  enum ParamId
  {
    kShape,     // Selects the waveshaping function
    kDrive,     // Input gain in dB
    kDC,        // A DC offset as raw value (added after the drive)
    kGain,      // Output gain for distorted signal in dB

    // Maybe add later:
    //kDry,  // +-100%
    //kWet,  // dito

    numParams
  };

  ClapWaveShaper(const clap_plugin_descriptor *desc, const clap_host *host);

  void parameterChanged(clap_id id, double newValue) override;

  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;

  bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;


  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;
  // ToDo: declare as noexcept...and maybe const, too? But nah! Generally, processing will change 
  // the state of a DSP algo (like storing past inputs and outputs in filters)

  static const char* const features[3];
  static const clap_plugin_descriptor_t descriptor;


  //-----------------------------------------------------------------------------------------------
  // \name WaveShaper specific stuff

  enum Shape
  {
    kClip,        // Hard clipper at -1, +1.
    kTanh,        // Hyperbolic tangent.
    kAtan,        // Arctangent, normalized to range -1..+1 and unit slope at origin.
    kErf,         // Error function. Antiderivative of Gaussian bell curve.

    // More ideas:
    //kRatAbs,    // x / (1 + |x|)      -> very cheap sigmoid but not smooth at 0 (I think)
    //kRatSqrt,   // x / sqrt(1 + x*x)  -> see "Random cheap sigmoid" thread on KVR
    //kRatSqr,    // x / (1 + x*x)      -> not a sigmoid, falls back to zero, some sort of fold

    numShapes
  };
  // The functions are all normalized to produce outputs in -1..+1 and have unit slope at the 
  // origin. This is achieved by scaling input and output appropriately.


  float applyDistortion(float x);
  // ToDo: declare as noexcept, maybe inline

protected:

  // Internal algorithm parameters/coeffs:
  Shape shape  = kClip;
  float inAmp  = 1.f;
  float outAmp = 1.f;
  float dc     = 0.f;

  // Holds the strings for the shape names for GUI display:
  std::vector<std::string> shapeNames;

};

//=================================================================================================

/** UNDER CONSTRUCTION (kinda works already but is still lacking some features)

A simple tone generator to demonstrate usage of class ClapSynthStereo32Bit. It demonstrates how
to respond to midi note events and how to deal with a sample-rate dependent and stateful DSP 
algorithm. To handle all this correctly, we need to override a few more methods like de/activate, 
reset, noteOn/Off, etc. */

class ClapToneGenerator : public RobsClapHelpers::ClapSynthStereo32Bit
{

  using Base = ClapSynthStereo32Bit;

public:

  //-----------------------------------------------------------------------------------------------
  // \name Boilerplate

  ClapToneGenerator(const clap_plugin_descriptor *desc, const clap_host *host);

  bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) 
    noexcept override;

  void deactivate() noexcept override;

  void reset() noexcept override;

  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;

  void parameterChanged(clap_id id, double newValue) override;

  void noteOn( int key, double velocity) override;

  void noteOff(int key) override;

  static const char* const features[3];
  static const clap_plugin_descriptor_t descriptor;


  //-----------------------------------------------------------------------------------------------
  // \name ToneGenerator specific stuff

  inline float getSample()
  {
    // Produce silence when no note is being held:
    if(currentKey == -1)
      return 0.f;

    // Compute output:
    static const double pi2 = 6.2831853071795864769;  // 2*pi
    float out = sin((float) (pi2 * phasor));          // Our phasor is in 0..1, sin wants 0..2pi

    // Update state:
    phasor += increment;
    if(phasor > 1.0)
      phasor -= 1.0;

    // Return output:
    return out;

    // ToDo:
    //
    // -Check, if it is safe to use "if" rather than "while" for the wrap-around. With extremely 
    //  high frequencies, we may need a "while". But that happens only above (twice?) the Nyquist 
    //  limit. Can we get such high frequencies? That should probably be made impossible anyway.
    // -Check, if we need also a wrap around for phasor < 0.0. This may happen only when the 
    //  increment is negative. Make sure that this doesn't occur.
    // -If we don't need any of that, document, why not i.e. explain why the situations mentioned 
    //  above cannot occur. There must be a safety net on a higher level (like clipping frequencies
    //  to 0..sampleRate/2 in noteOn, for example - I think that corresponds to clipping the 
    //  increment to 0..0.5).
    // -Maybe make the thing stereo by giving the user a stereo phase parameter
  }


protected:

  // Parameters:
  double increment  = 0.0;   // Per sample increment for our phasor

  // ToDo:
  //float amplitude   = 1.0;
  //float stereoPhase = 0.0;
  //float detune      = 0.0;
  //float startPhase  = 0.0;
  //float ampByKey    = 0.0;
  //float ampByVel    = 0.0;
  //float pitchWheel  = 0.0;

  // State:
  double phasor     = 0.0;   // Current sine phase in 0..1
  int    currentKey = -1;    // Currently held note. -1 means "none".

};