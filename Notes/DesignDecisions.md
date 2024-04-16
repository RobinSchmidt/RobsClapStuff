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

The CLAP API allows (or rather requires) the plugin to assign an identifier (short: id) to each of 
its parameters. This identifier is of type `clap_id` which is just a typedef for `uint32_t`. The 
plugin is free to choose any id it wishes as long as it is unique, i.e. allows unique identification 
of the parameter. The host will use this id to identify the parameter whenever it wants to set or 
get its value, translate it to/from a string, etc. Parameters also have an index which is just a 
running number from 0 to N-1 where N is the number of parameters. When the host first wants to 
inquire what parameters a plugin has, it will request the plugin to fill out a clap_param_info 
struct. The id is just a field in this struct. Inquiring this info-struct is the only time where the 
host will refer to the parameter by its index and it's done once at instantiation time (Well, 
conceptually at least. Bitwig actually seems to call it twice at instantiation time and then again 
once at destruction). All subsequent accesses will be done via the id. For the plugin implementor, 
that means it must be able to quickly (preferably in O(1)) map from the paramter id to the storage 
location of its value. The value of a parameter is a `double`.


### Decision:

The list of ids must be a permutation of the list of indices.


### Consequences

- We can cause the host to re-order the knobs on the generated GUI when we publish an update for a
  plugin. That includes inserting knobs at arbitrary positions. When using the naive identity 
  mapping `index == id` (like we implicitly did in VST2 where an id was not a thing), that would be 
  impossible.

- The back-and-forth mapping between index and id can simply be implemented by a pair of 
  `std::vector<clap_id>`, `std::vector<uint32_t>` of length N where N is the number of parameters. 
  Using the permutation map, accessing parameters by id or index is simple and fast - O(1) in both 
  directions. No need to pull in a hash table or anything complicated like that.

- I guess, that obviates the "cookie" facility as well which, I suppose, has the purpose of 
  allowing plugins to speed up the lookup of a parameter (object) by id (by avoiding the lookup
  altogether).





### Implementation

The class ClapPluginWithParams has a `std::vector<clap_param_info>` where it stores all the 
parameter infos. The storage index in that vector *is* the index of the paramter that the host uses 
when requesting the plugin to fill out thet struct. The class has also a `std::vector<double>` in 
which it stores the values of the parameters. In this vector, the id (*not* the index) is used to 
address a particular parameter. When we want to index our parameter value array by the id and we 
want to use a length N array for the values as well, then the ids must also be numbers in 0...N-1. 
They can be different from the indices, though.



### Conclusion

In my opinion, the main advantage of the id system over just using indices directly as VST2 did is 
the ability to change the apparent order of the parameters presented to the user. The permutation
mapping retains this ability and that's really all I care about. I don't really see how using 
totally "random" numbers would buy us any more flexibility for things that matter. So, the 
restriction to permutation maps seems to not give up anything of value while buying us a simple
implementation of the mapping. That's why I opted for it.














----------------------------------------------------------------------------------------------------