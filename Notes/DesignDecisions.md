Design Decisions
================

Here, I document the design decisions that I made when writing this "mini-framework" for building
CLAP plugins


Parameter Identifiers
---------------------

Background:

The CLAP API allows (or rather requires) the plugin to assign an identifier (short: id) to each of 
its parameters. This identifier is of type clap_id which is just a typedef for uint32_t. The plugin
is free to choose any id it wishes as long as it is unique, i.e. allows unique identification of the
parameter. The host will use this id to identify the parameter whenever it wants to set or get its 
value, translate it to/from a string, etc. Parameters also have an index which is just a running 
number from 0 to N-1 where N is the number of parameters. When the host first wants to inquire what
parameters a plugin has, it will request the plugin to fill out a clap_param_info struct. The id is
just a field in this struct. Inquiring this info-struct is the only time where the host will refer 
to the parameter by its index and it's done once at instantiation time (well, conceptually at least 
- Bitwig actually seems to call it twice at instantiation time and then again once at destruction). 
All subsequent accesses will be done via the id. For the plugin implementor, that means it must be 
able to quickly (preferably in O(1)) map from the paramter id to the storage location of its value.
The value of a parameter is a double precision floating point number.

Decision:


If the parameter values are stored in an array of dou


Mapping from arbitrary numbers 


















----------------------------------------------------------------------------------------------------