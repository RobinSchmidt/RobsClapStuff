This repo contains code resulting from my explorations of the new audio plugin format CLAP. There 
are some projects with example plugins with and without the official C++ wrapper as well as one 
project where I use my own wrapper - which is not really "my own" but actually a customized variant 
of the official one in which I scrapped some stuff which I don't need and added some stuff which I 
do need but was missing and where I also added some comments and documentation. The purpose of the 
repo is mostly educational. My goal is to make the process of building a first clap plugin so easy 
that even someone like me (i.e. someone with not so much experience with all the infrastructural 
stuff) can understand it. At the moment, there are only project files for Visual Studio. As starting
point, I recommend to load

  Examples/WithMyWrapper/Build/VisualStudio2019/ClapStuff.sln

into Visual Studio and have a look around. Try to Build-and-Run the "Tests" project. If all goes 
well, it should create a command-line application that runs some unit tests and reports success or 
failure. The things that are being tested there are my C++ convenience classes around the CLAP C-API 
and some demo plugins that have been created with those.

When you try to build the "DemoPlugins" project, you will probably initially get an error message 
because there is a post-build step that tries to copy the resulting plugin library into the folder:
 
  C:/Temp/CLAP  

which will not work unless that folder exists. The reason for this post-build step is to make the 
Edit-Build-Test cycle with a DAW more convenient. I use the Temp/CLAP folder on my machine for 
plugins that are under construction. Aside from the failing post-build step, you should get some 
build products in subfolders of the project folder. Among them, you should find:

  Examples/WithMyWrapper/Build/VisualStudio2019/x64/Debug/DemoPlugins.clap

This is a library containing my first demo clap plugins. Currently, there's a gain and waveshaper
plugin. If you are coming from VST and think "AGain", then you are exactly on the right track. I 
wanted to figure out the easiest way to create some first, GUI-less CLAP plugins similar in spirit 
to Steinberg's "Hello World" example to the VST-SDK.




* meaning: someone who is not so savvy with all the infrastructural stuff - APIs, ABIs, entry 
points, exported symbols, wrappers, yadda yadday yah. I've just been using JUCE for so many years 
which allowed me to remain blissfully ignorant of all these "details".