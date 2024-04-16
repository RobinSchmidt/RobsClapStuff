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


Parameter Identifiers
---------------------

### Background:

The `clap_plugin_params` extension allows a plugin to report a bunch of parameters to the host which
the host can use to control the settings of the plugin. This can be done either live turning by 
knobs or sliders on a host generated GUI or by drawing in automation data. The API requires the 
plugin to assign an identifier (short: id) to each of its parameters. This identifier is of type 
`clap_id` which is just a typedef for `uint32_t`. The plugin is free to choose any id it wishes as 
long as it is unique, i.e. allows unique identification of the parameter. The host will use this id 
to identify the parameter whenever it wants to set or get its value, translate it to/from a string, 
etc. 

Parameters also have an index which is just a running number from 0 to N-1 where N is the number of 
parameters. When the host first wants to inquire what parameters a plugin has, it will request the 
plugin to fill out a clap_param_info struct. The id is just a field in this struct. Inquiring this 
info-struct is the only time where the host will refer to the parameter by its index and it's done 
once at instantiation time (Well, conceptually at least. Bitwig actually seems to call it twice at 
instantiation time and then again once at destruction). All subsequent accesses will be done via the 
id. For the plugin implementor, that means it must be able to quickly (preferably in O(1)) map from 
the paramter id to the storage location of its value. The value of a parameter is a `double`.


### Decision:

The list of ids must be a permutation of the list of indices.


### Consequences

- We can cause the host to re-order the knobs on the generated GUI when we publish an update for a
  plugin. That includes inserting knobs at arbitrary positions. When using the naive identity 
  mapping `index == id` like we implicitly did in VST2 where an id was not a thing, that would be 
  impossible.

- The back-and-forth mapping between index and id can simply be implemented by a pair of 
  `std::vector<clap_id>`, `std::vector<uint32_t>` of length N where N is the number of parameters. 
  Using the permutation map, accessing parameters by id or index is simple and fast - O(1) in both 
  directions. No need to pull in a hash table or anything complicated like that.

- I guess, that obviates the "cookie" facility as well which, I suppose, has the purpose of 
  allowing plugins to speed up the lookup of a parameter object by id by avoiding the lookup
  altogether.


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
as user is concerned, we actually *can* reorder the way in which the hosts presents the knobs on the 
generated GUI.

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
generated GUI and the ability to change that apparent order in updated vesriosn of plugins. The 
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
example state could look as follows:
```
CLAP Plugin State

Identifier: RS-MET.WaveShaperDemo
Version: 2024.04.03
Vendor: RS-MET
Parameters: [0:Shape:1,1:Drive:7,2:DC:0.25,3:Gain:-5]
```
The values of the parameters are stored within square brackets in the format Identifier:Name:Value 
separated by commas. The state also stores the identifier of the plugin and information about the 
version of the plugin with which the state was produced as well as some additional info.


### Consequences

This format is easy to write and read with a very small amount of code without needing to pull in 
any serialization library. A human readble format is generally nice for debugging. Storing the 
plugin identifier allows to check, if this is really the right kind of state. Storing the version 
number allows to change the format later. The format is reasonably compact
...TBC...


ToDo:
-Explain what happens in version updates with more parameters (the additional parameters will be set
 to default values)
-One could have left out the names and store only the id. That would make the size smaller but the 
 format would be less readable.