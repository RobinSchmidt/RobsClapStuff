Overview
--------

This repo contains code resulting from my explorations of the new audio plugin format CLAP. There 
are some projects with example plugins with and without the official C++ wrapper as well as one 
project where I use my own wrapper - which is not really "my own" but actually a customized variant 
of the official one in which I scrapped some stuff which I don't need and added some stuff which I 
do need but was missing and where I also added some comments and documentation. The purpose of the 
repo is mostly educational. My goal is to make the process of building a first clap plugin so easy 
that even someone like me (i.e. someone with not so much experience with all the infrastructural 
stuff) can understand it. At the moment, there are only project files for Visual Studio. As starting
point, I recommend to load

> Examples/WithMyWrapper/Build/VisualStudio2019/ClapStuff.sln

into Visual Studio and have a look around. Try to Build-and-Run the "Tests" project. If all goes 
well, it should create a command-line application that runs some unit tests and reports success or 
failure. The things that are being tested there are my C++ convenience classes around the CLAP C-API 
and some demo plugins that have been created with those.


Making Plugins
--------------

When you try to build the "DemoPlugins" project, you will probably initially get an error message 
because there is a post-build step that tries to copy the resulting plugin library into the folder:
 
> C:/Temp/CLAP  

which will not work unless that folder exists. The reason for this post-build step is to make the 
Edit-Build-Test cycle with a DAW more convenient. I use the Temp/CLAP folder on my machine for 
plugins that are under construction. Aside from the failing post-build step, you should get some 
build products in subfolders of the project folder. Among them, you should find:

> Examples/WithMyWrapper/Build/VisualStudio2019/x64/Debug/DemoPlugins.clap

This is a library containing my first demo clap plugins. Yes - plugins in plural. The project also 
demonstrates how to put several plugins into a single library (i.e. .dll or .so or whatever). 
Currently, there's a gain and waveshaper plugin. If you are coming from VST and think "AGain", then 
you are exactly on the right track. I wanted to figure out the easiest way to create some first, 
GUI-less CLAP plugins similar in spirit to Steinberg's "AGain" example in the VST-SDK. Such simple 
gain plugins are the audio-plugin equivalent to "Hello World" examples for programming in general.
The main features from Steinberg's "AGain" example that I wanted to replicate are:

  - The plugin should be a subclass of some baseclass provided by the SDK/framework.
  - The host should be able to provide a resonable generic GUI where parameter names and nicely 
    formatted values are shown.
  - Automation and state-recall should "just work" without any further ado, i.e. without any code in
    the plugin class.
 
Additional desiderata:

  - Automation should be sample-accurate out of the box.
  - The amount of boilerplate code in the actual plugins should be minimal
  - The amount of framework code to make all of that happen should be smallish
  - The framework code should introduce only minimal computational overhead and size bloat

I have used the term "framework" here and it sounds like rather big word for the humble stuff that 
is going on here. I tend to use the term framework whenever I deal with some re-usable library code 
where the client code needs to implement functionality that the library code calls. In a "toolkit",
on the other hand, the flow of control is usually the other way around: client code calls functions 
or uses classes from the library. Frameworks invert this control flow. This is sometimes called the 
"Hollywood principle" ("Don't call us - we will call you") and is, in my opinion, the hallmark of 
frameworks. In object oriented frameworks, the way this works is usually that the client code 
derives a subclass from some library-provided baseclass and overrides one or more of its virtual 
functions. This is the case here - therefore, I guess one could call my set of wrapper classes a 
"mini-framework". In particular, in the file:

> RobsClapHelpers/ClapPluginClasses.h

there is a class called ClapPluginStereo32Bit. If all you want to do is to write a plugin that 
processes 32-bit floating point audio data in stereo, then you can just create a subclass of this
class and with a very small amount of boilerplate, the usually expected functionality of a 
GUI-less plugin (i.e. audio I/O, parameter-handling and state-recall) will just work. To see how 
this can be done, just take a look at:

> Examples/WithMyWrapper/Source/DemoPlugins.h  
> Examples/WithMyWrapper/Source/DemoPlugins.cpp  
> Examples/WithMyWrapper/Source/DemoPluginsEntryPoint.cpp

These 3 files are all you need to get a basic working "AGain"-style example for the CLAP format. 
It's actually already more than just an "AGain" because it has two plugins (inside a single 
library) and also demonstrates how choice parameters could be handled.


Future Plans
------------

I do have some ideas to further reduce the amount of boilerplate inside the plugin code. In 
particular, I have some ideas how to get rid of the current necessity to override the value/text
conversion functions by implementing a system for handling that in the baseclass. I'm not yet sure,
if should really do that, though. On the pro side, it would reduce the boilerplate in the plugin 
code. On the contra side, it would introduce a bit more of framework overhead and make the handling 
of the conversions a little bit less flexible by imposing certain ways doing them which may not 
always be most suitable. I tend to think that the reduction of boilerplate is more important, 
though - so we'll see...

Some of the other things on the ToDo list are to move the build system to CMake, to implement a 
baseclass for instrument plugins with convenient hooks for handling noteOn/Off events and to show 
how to implement GUIs and maybe to show how to use some of the more advanced features of CLAP such 
as polyphonic parameter modulation. I also want to make a suite of small, GUI-less plugins which 
should include Open303. But that will probably happen in my main repo where I have access to my full
DSP library.