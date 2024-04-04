#pragma once

#include "../../../RobsClapHelpers/RobsClapHelpers.h"


/** A very simple gain plugin whose purpose is to demonstrate the usage of class 
ClapPluginStereo32Bit. The purpose of this class is enable writing of common stereo plugins with
32 bit processing with as little boilerplate code as possible. To write a clap plugin based on 
ClapPluginStereo32Bit, you need to do the following things:

(1) Declare your parameters in the Params enum.
(2) Implement the constructor. It should populate the inherited parameter array usig addParameter.
(3) Override setParameter to handle parameter changes.
(4) Override processBlockStereo to process (sub) blocks of audio sample frames.
(5) Fill the features array.
(6) Fill the pluginDescriptor.

...then you have to implement the factory and entry point...TBC...  */

class ClapGain : public RobsClapHelpers::ClapPluginStereo32Bit
{

  using Base = ClapPluginStereo32Bit; // For conveniently calling baseclass methods

public:

  /** Enumeration of the parameters. These values are used for the param_id and they are also the
  indices at which the parameters will be stored in our inherited parameters array. That means, 
  the values must start at 0 and count continuously up and the order must be stable and new 
  parameters can only be added at the end. That's pretty much the behavior we know from VST 1/2 
  when not using the "chunks" mechanism for the state. That's why I also use the convention with
  the k-prefix - if you know VST 1/2, that should all look rather familiar. */
  enum Params
  {
    kGain,
    kPan,
    //kMono,     // Maybe add this as a boolean parameter

    numParams
  };


  /** Constructor. It populates our inherited array of parameters using calls to addParameter. */
  ClapGain(const clap_plugin_descriptor *desc, const clap_host *host);

  /** This is a overriden callback that gets called from inside the process method of our basclass.
  This baseclass method is responsible for interleaving the calls to setParameter and calls to 
  processBlockStereo. Here, we just have to override both methods and do the appropriate actions 
  there and we get sample-accurate parameter automation for free. */
  void parameterChanged(clap_id id, double newValue) override;


  bool paramsValueToText(clap_id paramId, double value, char *display, 
    uint32_t size) noexcept override;




  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;
  // Maybe make this public to facilitate testing


  static const char* const features[4];

  static const clap_plugin_descriptor_t pluginDescriptor; 




protected:


  // Internal algorithm coefficients:
  float ampL = 1.f, ampR = 1.f;          // Gain factors for left and right channel

};

//=================================================================================================

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

  static const char* const features[3];

  static const clap_plugin_descriptor_t pluginDescriptor; 


  //-----------------------------------------------------------------------------------------------
  // \name WaveShaper specific stuff

  enum Shapes
  {
    kClip,
    kTanh,
    kAtan,
    kErf,

    // ...more to come...
    //kRatAbs,    // 1 / (1 + |x|)
    //kRatSqrt,   // 1 / sqrt(1 + x*x)
    //kRatSqr,    // 1 / (1 + x*x)

    numShapes
  };

  bool shapeToString(double val, char *display, uint32_t size);

  float applyDistortion(float x);


protected:

  // Internal algorithm parameters/coeffs:
  int   shape  = kClip;
  float inAmp  = 1.f;
  float outAmp = 1.f;
  float dc     = 0.f;

};


// ToDo: rename to ExampleClapPlugins and add some more plugins