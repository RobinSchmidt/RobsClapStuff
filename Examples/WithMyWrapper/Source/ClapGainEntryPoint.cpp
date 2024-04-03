
// Defines the entry point for the .dll

#include "ClapGain.h"


// The factory. It is responsible to inform the host about the number of plugins present in this
// .clap, to deliver (pointers to) the plugin-descriptors for all the plugins and to create the 
// plugin object itself and return a pointer to it.
static const clap_plugin_factory_t pluginFactory = 
{  
.get_plugin_count = [] (const clap_plugin_factory *factory) -> uint32_t 
{  
  return 1;   // This .dll/.clap contains a single plugin

  //return 2; // StereoGain, WaveShaper
},

.get_plugin_descriptor = [] (const clap_plugin_factory *factory, uint32_t index) 
-> const clap_plugin_descriptor_t * 
{ 
  // If multiple plugins are in this .dll, we need to switch based on the index here:
  return index == 0 ? &ClapGain::pluginDescriptor : nullptr;

  /*
  switch(index)
  {
  case 0:  return &ClapGain::pluginDescriptor;
  case 1:  return &ClapWaveShaper::pluginDescriptor;
  default: return nullptr;
  }
  */

},

.create_plugin = [] (const clap_plugin_factory *factory, const clap_host_t *host, 
                     const char *pluginID) -> const clap_plugin_t *  
{
  // If the clap versions of host and plugin are incompatible, return a nullptr:
  if (!clap_version_is_compatible(host->clap_version) 
    || strcmp(pluginID, ClapGain::pluginDescriptor.id)) 
  { 
    return nullptr; 
  }

  // If multiple plugins are in this .dll, we need to switch based on the pluginID here:
  // Create an object of type ClapGain and return its underlying C-struct to the caller:
  ClapGain* gain = new ClapGain(&ClapGain::pluginDescriptor, host);
  return gain->getPluginStructC();
  // ToDo: figure out and document how the ClapGain object gets destructed



  /*
  if( !clap_version_is_compatible(host->clap_version) )
    return nullptr;


  if(strcmp(pluginID, ClapGain::pluginDescriptor.id) == 0)
  {
    ClapGain* gain = new ClapGain(&ClapGain::pluginDescriptor, host);
    return gain->getPluginStructC();
  }
  if(strcmp(pluginID, ClapWaveShaper::pluginDescriptor.id) == 0)
  {
    ClapWaveShaper* shaper = new ClapWaveShaper(&ClapWaveShaper::pluginDescriptor, host);
    return shaper->getPluginStructC();
  }

  return nullptr;
  */
},
};

// Entry point:
extern "C" const clap_plugin_entry_t clap_entry = 
{
  .clap_version = CLAP_VERSION_INIT,
  .init = [] (const char *path) -> bool { return true; },
  .deinit = [] () {},
  .get_factory = [] (const char *factoryID) -> const void * 
   {
     return strcmp(factoryID, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &pluginFactory;
   },
};
