
// This file defines the entry point for the shared library (i.e. .dll, .so, etc.). But clap plugins
// use the extension .clap. We just need to rename the .dll accordingly.

#include "DemoPlugins.h"


// This is the plugin factory. It is responsible to inform the host about the number of plugins 
// present in this .clap plugin library, to deliver (pointers to) the plugin-descriptors for all 
// the plugins and to create the plugin object itself and return a pointer to it.
static const clap_plugin_factory_t pluginFactory = 
{  
.get_plugin_count = [] (const clap_plugin_factory *factory) -> uint32_t 
{  
  return 2; // Two plugins in this library: StereoGainDemo, WaveShaperDemo
},

.get_plugin_descriptor = [] (const clap_plugin_factory *factory, uint32_t index) 
-> const clap_plugin_descriptor_t * 
{ 
  switch(index)
  {
  case 0:  return &ClapGain::descriptor;
  case 1:  return &ClapWaveShaper::descriptor;
  default: return nullptr;
  }
},

.create_plugin = [] (const clap_plugin_factory *factory, const clap_host_t *host, 
                     const char *pluginID) -> const clap_plugin_t *  
{
  // If the supported clap versions of host and plugin are incompatible, return a nullptr:
  if( !clap_version_is_compatible(host->clap_version) )
    return nullptr;


  // Now we check the ID of the plugin that the host has requested and create the appropriate 
  // C++ wrapper object and return a pointer to its wrapped C-struct to the host:

  // StereoGainDemo:
  if(strcmp(pluginID, ClapGain::descriptor.id) == 0)
  {
    ClapGain* gain = new ClapGain(&ClapGain::descriptor, host);
    return gain->clapPlugin();
  }

  // WaveShaperDemo:
  if(strcmp(pluginID, ClapWaveShaper::descriptor.id) == 0)
  {
    ClapWaveShaper* shaper = new ClapWaveShaper(&ClapWaveShaper::descriptor, host);
    return shaper->clapPlugin();
  }

  // The host has passed a pluginID that could not be matched to any of the plugins in this 
  // library, so we return a nullptr:
  return nullptr;


  // ToDo:
  // 
  // -Figure out and document how the objects get destructed. Maybe place a debug breakpoint into
  //  the destructor. OK - when plugging a plugin out, ClapPlugin::clapDestroy() gets called. 
  //  This has the line  delete &self;  at the bottom which explicitly calls the destructor of the 
  //  ClapPlugin class. If the subclass has its own destructor, that one gets called before
  //  the baseclass destructor.
  //
  // -Document why we need to pass a descriptor to the constructor. The plugin itself surely 
  //  already knows what kind of plugin it is. It feels a bit like writing code like: 
  //    Dog* dog = new Dog("Dog");  // Why do we need to tell the Dog() constructor that we are 
  //                                // constructing a "Dog"? The constructor already knows this.
  //  Maybe it's for a sanity check inside the constructor? It would make sense in a context like:
  //    Animal* dog = new Animal("Dog");
  //  but I can't see, how a situation like this could occur. -> Figure out!
},
};

// Entry point:
extern "C" const clap_plugin_entry_t clap_entry = 
{
  .clap_version = CLAP_VERSION_INIT,
  .init         = [] (const char *path) -> bool { return true; },
  .deinit       = [] () {},
  .get_factory  = [] (const char *factoryID) -> const void * 
   { 
     return strcmp(factoryID, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &pluginFactory; 
   },

  // ToDo:
  //
  // -Explain how this entry point works on the ABI level.
  // -Document typical use cases for the init/deinit functions. See also here:
  //  https://github.com/free-audio/clap-saw-demo-imgui/blob/main/src/clap-saw-demo-pluginentry.cpp
  //  They are empty dummy functions there, too. They are required to be fast and it's not allowed
  //  to do any user intercation or GUI stuff there. See:  clap/include/clap/entry.h
};
