#pragma once

#include "../../../RobsClapHelpers/RobsClapHelpers.h"


/** A very simple gain plugin whose purpose is to demonstrate the usage of class 
ClapPluginStereo32Bit. The purpose of this class is enable writing of common stereo plugins with
32 bit processing with as little boilerplate code as possible. To write a clap plugin based on 
ClapPluginStereo32Bit, you need to do the following things:

(1) Derive your plugin class from RobsClapHelpers::ClapPluginStereo32Bit
(2) Declare your parameters in some sort of "Params" enum.
(3) Implement the constructor. It should populate the inherited parameter array using addParameter.
(4) Override parameterChanged to handle parameter changes.
(5) Override processBlockStereo to process (sub) blocks of audio sample frames.
(6) Fill the features array.
(7) Fill the pluginDescriptor.

...then you have to implement the factory and entry point...TBC...  */

class ClapGain : public RobsClapHelpers::ClapPluginStereo32Bit
{

  using Base = ClapPluginStereo32Bit; // For conveniently calling baseclass methods
  // try to get rid - i think, we don't call baseclass methods anymore

public:

  /** Enumeration of the parameters. These values are used for the param_id and they are also the
  indices at which the parameters will be stored in our inherited parameters array. That means, 
  the values must start at 0 and count continuously up. The order must be stable between plugin 
  versions (or else, you'll break state recall) and new parameters can only be added at the end. 
  That's pretty much the behavior we know from VST 1/2 when not using the "chunks" mechanism for 
  the state. That's why I also use the convention with the k-prefix - if you know VST 1/2, that 
  should all look rather familiar. */
  enum Params
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

  enum Params
  {
    kShape,     // Selects the waveshaping function
    kDrive,     // Input gain in dB
    kDC,        // A DC offset as raw value (added after the drive)
    kGain,      // Output gain for distorted signal in dB

    //kInvert,    // Invert distorted signal (before mixing with original)
    //kMix,

    // alternative:
    //kDry,  // +-100%
    //kWet,  // dito

    numParams
  };

  ClapWaveShaper(const clap_plugin_descriptor *desc, const clap_host *host);

  void parameterChanged(clap_id id, double newValue) override;

  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;

  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;
  // ToDo: declare as noexcept...and maybe const, too? But nah! Generally, processing will change 
  // the state of a DSP algo (like storing past inputs and outputs in filters)

  static const char* const features[3];
  static const clap_plugin_descriptor_t descriptor;


  //-----------------------------------------------------------------------------------------------
  // \name WaveShaper specific stuff

  enum Shapes
  {
    kClip,
    kTanh,
    kAtan,
    kErf,         // Error function. Antiderivative of Gaussian bell curve.

    // More ideas:
    //kRatAbs,    // 1 / (1 + |x|)      -> very cheap sigmoid but not smooth at 0 (I think)
    //kRatSqrt,   // 1 / sqrt(1 + x*x)  -> see "Random cheap sigmoid" thread on KVR
    //kRatSqr,    // 1 / (1 + x*x)      -> not a sigmoid, falls back to zero, some sort of fold

    numShapes
  };
  // The functions are all normalized to produce outputs in -1..+1 and have unit slope at the 
  // origin. This is achieved by scaling input and output appropriately.

  bool shapeToString(double val, char *display, uint32_t size);

  float applyDistortion(float x);
  // ToDo: declare as noexcept


protected:

  // Internal algorithm parameters/coeffs:
  int   shape  = kClip;
  float inAmp  = 1.f;
  float outAmp = 1.f;
  float dc     = 0.f;

};