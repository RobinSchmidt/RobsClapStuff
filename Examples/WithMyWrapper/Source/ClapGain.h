#pragma once

//#include "ClapPluginClasses.h"
#include "../../../RobsClapHelpers/ClapPluginClasses.h"


class ClapGain : public ClapPluginStereo32Bit
{

public:

  enum Params
  {
    kGain,
    kPan,
    //kMono,     // Maybe add this as a boolean parameter

    numParams
  };


  using Base = ClapPluginStereo32Bit; // For conveniently calling baseclass methods

  ClapGain(const clap_plugin_descriptor *desc, const clap_host *host);



  static const clap_plugin_descriptor_t pluginDescriptor; 


  void setParameter(clap_id id, double newValue) override;



protected:

  void processBlockStereo(const float* inL, const float* inR, float* outL, float* outR, 
    uint32_t numFrames) override;




  // Internal algorithm coefficients:
  float ampL = 1.f, ampR = 1.f;          // Gain factors for left and right channel

};

