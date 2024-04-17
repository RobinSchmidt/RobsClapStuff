Design Decisions
================

When using the raw CLAP API, a plugin implementor has to make bunch of decisions for how to 
handle various things. When making such decisions for a framework whose purpose is to make writing
plugins easy, we must anticipate the consequences of such decisions for the plugins that we intend 
to write on top of the framework. Decisions on the framework level will make writing plugins easier
but will also put limitations on the ways we can write our plugins. Making such decisions in an 
informed way that makes our lives as easy as possible without taking away too much of our 
flexibility consumes a considerable part of the development process. Here, I document the design 
decisions that I made for this "mini-framework" and explain their rationale.

----------------------------------------------------------------------------------------------------

Parameter Identifiers
---------------------

### Background:

The `clap_plugin_params` extension allows a plugin to announce its parameters to the host which the 
host can use to control the settings of the plugin. This can be done either live turning by knobs or 
sliders on a host generated GUI or by drawing in automation data. The API requires the plugin to 
assign an identifier (short: id) to each of its parameters. This identifier is of type `clap_id` 
which is just a typedef for `uint32_t`. The plugin is free to choose any id it wishes as long as it 
is unique, i.e. allows unique identification of the parameter. The host will use this id to identify 
the parameter whenever it wants to set or get its value, translate it to/from a string, etc. 

Parameters also have an index which is just a running number from 0 to N-1 where N is the number of 
parameters. When the host first wants to inquire what parameters a plugin has, it will request the 
plugin to fill out a clap_param_info struct. The id is just a field in this struct. Inquiring this 
info-struct is the only time where the host will refer to the parameter by its index and it's done 
once at instantiation time (Well, conceptually at least. Bitwig actually seems to call it twice at 
instantiation time and then again once at destruction). All subsequent accesses will be done via the 
id. For the plugin implementor, that means the plugin must be able to map quickly (i.e. in O(1) with 
small constant factor) from the paramter id to the storage location of its value. The value of a 
parameter is a `double`.


### Decision:

The list of ids should be a permutation of the list of indices.


### Consequences

- We can cause the host to re-order the knobs on the generated GUI when we publish an update for a
  plugin. That includes inserting knobs at arbitrary positions. When using the naive identity 
  mapping `index == id` like we implicitly did in VST2 where an id was not a thing, that would be 
  impossible.

- The back-and-forth mapping between index and id can simply be implemented by a pair of 
  `std::vector<clap_id>`, `std::vector<uint32_t>` of length N where N is the number of parameters. 
  Using the permutation map, accessing parameters by id or index is simple and fast - O(1) in both 
  directions. It's a simple array access in both directions. No need to pull in a hash table or 
  anything complicated like that.

- I guess, that obviates the "cookie" facility as well which, I suppose, has the purpose of 
  allowing plugins to speed up the lookup of a parameter object by id by avoiding the lookup
  altogether.

### Discussion

Mathematically, the situation that we are dealing with here is that of defining a bijective map 
between two sets. In our case, these sets are the set of indices which is given (namely the set of 
integer numbers 0..N-1) and the set of the N identifiers (which are under the hood also integers)
which we can freely choose as long as each element is unique (which is actually already implied 
anyway when we talk about sets). When we want to implement a map between the integers 0..N-1 and N 
other integers of our own choice, the most obvious and natural choice surely seems to be to use also
0...N-1. That choice allows us to implement the map by a simple pair of arrays of length N.

### Implementation

The class ClapPluginWithParams has a `std::vector<clap_param_info>` where it stores all the 
parameter infos. The storage index in that vector *is* the index of the paramter that the host uses 
when requesting the plugin to fill out the struct. The class has also a `std::vector<double>` in 
which it stores the values of the parameters. In this vector, the id (*not* the index) is used to 
address a particular parameter. When we want to index our parameter value array by the id and we 
want to use a length N array for the values as well, then the ids must also be numbers in 0...N-1. 
They can be different from the indices, though. 

In the plugin code, I usually assign the ids by letting the plugin class have an enum that 
enumerates all the parameters. The enum starts (implicitly) at zero and just counts up, i.e. I do 
not assign any enum entries to arbitrary numbers. Such parameter enums are already familiar from 
VST2. To facilitate correct state recall after version updates of the plugin, any published initial 
section of the enum must remain stable. That means we can add new entries only at the end (not 
somewhere in between existing ones), we cannot delete any entries and we can't reorder the entries. 
Ability to reorder and insertion at arbitrary positions would be desirable though - however, as far 
as the user is concerned, we actually *can* reorder the way in which the hosts presents the knobs on 
the generated GUI.

The way is reordering works is as follows: The order in which the hosts generates the knobs is 
determined by the parameter's index, not by its id. The index, in turn, is determined by the order 
in which the parameters are added in the constructor of the plugin class and is therefore 
independent from the order of the ids in our enum. Maybe the enum becomes an unordered jumbled mess
after a couple of updates but the user won't ever see any of that.

There is a class IndexIdentifierMap that does the back-and-forth mapping between indices and their
corresponding identifiers. It's not yet used though because for the currently implemented 
functionality, we don't really need a mapping from id to index. The fact that the values array is 
stored in the order of the ids (and not the order of the indices) let's us immediately acces the 
right value by directly using the id. We may later want to retrieve other info about the parameter 
by id (like min/max values) in which case we can pull in the IndexIdentifierMap.


### Alternatives

- The clap-saw-demo example uses "random" numbers for the ids with std::unordered_map:  
  https://github.com/surge-synthesizer/clap-saw-demo/blob/main/src/clap-saw-demo.h#L91  
  https://github.com/surge-synthesizer/clap-saw-demo/blob/main/src/clap-saw-demo.h#L355  
  That seems a bit like overkill to me. A std::unordered_map is already a (moderately) 
  complex data structure. I don't really want to pull in that kind of complexity just to handle the 
  humble parameter ids.

- Alternatively, one could just store the ids in a unordered array and use a linear search for each 
  parameter lookup by id. That would imply a O(N) lookup cost which is probably not acceptable 
  especially when a lot of automation is going on and/or there are a lot of parameters. Perhaps one 
  could sort them and then use binary search with O(log(N)) lookup cost - but that seems too 
  complicated as well. 


### Conclusion

In my opinion, the main advantage of the id system over just using the raw indices directly (as VST2 
did) is the ability to have freedom over the way in which the host orders the parameters on its 
generated GUI and the ability to change that apparent order in updated versions of plugins. The 
permutation mapping retains this ability and that's really all I care about. I don't really see how 
using totally "random" numbers would buy us any more flexibility for things that matter. So, the 
restriction to permutation maps seems to not give up anything of value while buying us a simple
implementation of the mapping. That's why I opted for it.

----------------------------------------------------------------------------------------------------

State
-----

### Background:

When a CLAP implements the `clap_plugin_state` extension, it can use it to allow the host to store 
the state of the plugin and recall it later. To make that work, the plugin must implement a pair of 
`save` and `load` functions in which the hosts asks the plugin to produce or consume a stream of 
bytes that represent the state. How that byte stream is formatted is entirely up the implementor of
the plugin.


### Decision

To store and recall the state of a CLAP whose state is given by the values of all of its parameters,
I use a simple, human readable textual format that I invented ad hoc for this specific purpose. An 
example state string could look as follows:
```
CLAP Plugin State

Identifier: RS-MET.WaveShaperDemo
Version: 2024.04.03
Vendor: RS-MET
Parameters: [0:Shape:1,1:Drive:7,2:DC:0.25,3:Gain:-5]
```
The values of the parameters are stored within square brackets in the format Identifier:Name:Value 
separated by commas. The state also stores the identifier of the plugin and information about the 
version of the plugin with which the state was produced as well as some additional info. When 
reading a state and it doesn't have a value stored for one of our parameters, then it means that the
state was stored with a previous version of the plugin which had less parameters. Such additional 
parameters for which the state has no values stored will be set to their respective deafault values 
in the state recall. The rationale is that default values should be neutral values, i.e. values at
which the respective parameter does not change the sound at all (like a gain of 0 dB, a detune of
0 semitones, a percentage of 100, etc.) .


### Consequences

This format is easy to write and read with a very small amount of code without needing to pull in 
any serialization library. Storing the plugin identifier allows to check, if this is really the 
right kind of state within the load function. Storing the version number allows to change the format 
in later versions of plugins. The format is reasonably compact such that the amount of data produced 
is not too extensive while also being readable enough such that humans can just inspect a state and 
understand it immediately. This may be helpful for trouble shooting and debugging purposes. I'm not 
claiming that this is a particularly efficient way of doing it - but this functionality is not so
performance critical in the context of audio plugins anyway.


### Implementation

`ClapPluginWithParams` has the functions `getStateAsString` and `setStateFromString` which produce 
or consume a std::string in the simple format explained above and these functions are used within 
the implementations of the state save and load functions. 

----------------------------------------------------------------------------------------------------

Event Handling
--------------

### Background

During its realtime operation, a plugin must deal with two streams of data: the audio inputs (and 
outputs) and control data. The control data is received in the form of "events" that are passed to 
the block processing function along with the current buffer of audio samples. The events have a time 
stamp measured in samples with respect to the start of the block. Plugins may use that time stamp
to implement their responses to the events with sample-accurate timing. This requires interleaving
of event processing and audio processing inside the plugin's block processing callback. Writing this 
interleaving code in each and every plugin again and again produces a lot of boilerplate code.


### Decision

The interleaving of audio and event processing is done once and for all in the baseclass. Subclasses 
need to override certain virtual functions to process one event at a time for the event processing. 
They also need to override an audio processing function that operates on event-free sub-blocks.


### Consequences

Plugins can be blissfully oblivious of the whole messy interleaving. For the event processing, they 
will just receive one event at a time in the form of a call to an overriden event-handler function 
to which they can respond immediately. For the audio processing, they will receive event-free (sub)
blocks, through which they can iterate in a simple loop of *pure* DSP code. We have separated the 
two concerns of event processing and signal processing on the framework level. Client code will 
never have to think about all this mess ever again.

The calling of virtual functions for one event at a time may incur a runtime overhead - especially
for buffers that have a lot of events. I struck a trade-off here: I may have left some performance
optimization on the table and bought convenience in return. If you really care about sqeezing out 
every possible bit of performance, you *can* still override the lower level `process` function and 
(re)write the interleaving code there yourself and thereby get rid of the overhead. But you don't 
have to do that just to get a correctly working plugin. If events are sparse, which they usually 
are, the cost/benefit calculation seems to justify the decision.







...TBC...

----------------------------------------------------------------------------------------------------

Parameter Formatting
--------------------

### Background

To display the parameter values of GUI-less plugins on the host generated GUI, we must somehow 
translate them into strings (and back). ...TBC...


### Decision

Currently, the class `ClapPluginWithParams` implements two member functions `paramsTextToValue` and
`paramsValueToText` which do some crude and very simple default formatting. For anything more 
sophisticated, client code needs to override them. That can actually mean to have to write a lot of
boilerplate code for repetitive things because we tend to need very similarv formatting strategies
over and over again. It could be argued that asking client code to "do it yourself" amounts to 
having *not* yet made any decision how to handle it once and for all....TBC...