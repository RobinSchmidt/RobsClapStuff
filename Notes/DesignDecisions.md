Design Decisions
================

Here, I document the design decisions that I made when writing this "mini-framework" for building
CLAP plugins


Parameter Identifiers
---------------------

The CLAP API allows (or rather requires) the plugin to assign a identifier (short: id) to each of 
its parameters. This identifier is of type clap_id which is just a typedef for uint32_t. The plugin
is free to choose any id it wishes as long as it is unique, i.e. allows unique identification of the
parameter. The host will use this id to identify the parameter whenever it wants to set or get its 
value, translate it to/from a string, etc.


















----------------------------------------------------------------------------------------------------