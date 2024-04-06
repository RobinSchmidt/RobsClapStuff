Under construction - not yet finished

This is an example project in which I write my own C++ wrapper around the clap C-API following the
example code for the wrapper from here:

  https://github.com/free-audio/clap-helpers/blob/main/include/clap/helpers/plugin.hh
  https://github.com/free-audio/clap-helpers/blob/main/include/clap/helpers/plugin.hxx

but leaving out all the stuff that I do not need and simplifying the code and using conventions that
make it more look and feel like a mix of AudioEffectX from the VST-SDK and my own coding style.

----------------------------------------------------------------------------------------------------
Debugging workflow

The Visual Studio project file for the demo plugins contains post-build events like:

  copy "$(TargetPath)" "C:\Temp\CLAP\$(TargetName).clap" /y

to copy the produced .clap file into:

  C:\Temp\CLAP

For this step to work, this directory must exist on your drive. If it doesn't, the copy command will 
fail. You will still get your resulting .clap in the usual build results folder, though. This step 
is meant for making the edit-build-test cycle quicker and more convenient. You will also need to 
point your DAW to this folder to scan for plugins. I didn't use the standard clap plugin folder:

  C:\Program Files\Common Files\CLAP

because for writing files into this folder, one needs admin rights, so the copy command will 
typically fail with an "Access denied" message when trying to directly copying the clap into that 
folder (unless you are logged in as admin, I guess). These post-build steps exist in the "Debug" and 
in the "Release" configuration, so whether you will have a debug or release version in that folder 
for testing will depend on which version you built last. By the way, the plural in "plugins" is not 
a typo. The DemoPlugins.vcxproj project produces multiple plugins in a single dll. 

When testing in Bitwig, it is also convenient to mark the plugins to be tested as favorites and then 
using the right browser panel and click on the star in the top-left (so it becomes yellow) to show 
only the favorites. That makes browsing to the plugin to be tested faster.

To debug with Bitwig and the Visual Studio debugger, we need to attach the VS debugger to the 
process:

  BitwigPluginHost-X64-SSE41.exe

That process only exists, when there is a plugin already plugged in. So to debug the instantiation 
process of an effect plugin, it makes sense to have some other, known-to-be-good CLAP plugin (e.g. 
Surge XT) being plugged in on the channel so we can attach ourselves to the process even before 
plugging in our own plugin, so we can also catch crashes on construction in the debugger.

...hmm - sometimes the copy step fails because the plugin file is in use by aonother process even 
though it is not plugged in. Apparently Bitwig does not always release the plugin file when plugging
out the plugin? Ah - we need to plug out Surge, too. That will release the file. Hmm...a bit 
inconvenient.

So to catch crashes on startup, we need to
-Plug in Surge to let Bitwig create the BitwigPluginHost-X64-SSE41.exe process
-Attach the debugger to this process
-Set breakpoints in our plugin code where we want to stop the execution
-Plug in our tested plugin.

To do an edit and rebuild, it is not enough to plug out the plugin. We need to plug out Surge too in 
order to release the plugin file

