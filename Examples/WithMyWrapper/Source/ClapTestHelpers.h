#pragma once

#include "DemoPlugins.h"

//=================================================================================================
// Streams
//
// Some helpers to mock the clap stream objects that will be provided by the host during state
// load/save. This stuff needs verification. I don't know if my mock-stream behaves the way it is 
// intended by the CLAP API. I'm just guessing.

struct ClapStreamData
{
  std::vector<uint8_t> data;
  size_t pos = 0;              // Current read or write position
};

int64_t clapStreamWrite(const struct clap_ostream* stream, const void* buffer, uint64_t size);

int64_t clapStreamRead(const struct clap_istream* stream, void* buffer, uint64_t size);