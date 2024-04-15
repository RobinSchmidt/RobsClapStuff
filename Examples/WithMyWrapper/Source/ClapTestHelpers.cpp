
#include "ClapTestHelpers.h"

//=================================================================================================
// Streams

int64_t clapStreamWrite(
  const struct clap_ostream* stream, const void* buffer, uint64_t size)
{
  // Determine the number of bytes to write:
  uint64_t writeLimit = 25;        // To simulate limited number of bytes written per call
  uint64_t numToWrite = std::min(writeLimit, size);

  // Adjust size of the stream's data array:
  ClapStreamData* (csd) = (ClapStreamData*) (stream->ctx);
  uint64_t newSize      = csd->data.size() + numToWrite;
  csd->data.resize(newSize);

  // Write data from the passed buffer into the stream's data object:
  uint8_t* buf = (uint8_t*) buffer;
  for(uint64_t i = 0; i < numToWrite; i++)
    csd->data[csd->pos+i] = buf[i];
  csd->pos += numToWrite;

  // Return the number of bytes written into the stream:
  return numToWrite;
}

int64_t clapStreamRead(const struct clap_istream* stream, void* buffer, uint64_t size)
{
  // Determine the number of bytes to read:
  uint64_t readLimit = 25;
  uint64_t numToRead = std::min(readLimit, size);
  ClapStreamData* (csd) = (ClapStreamData*) (stream->ctx);
  uint64_t remaining = csd->data.size() - csd->pos;
  numToRead = std::min(numToRead, remaining);

  // Read the bytes from the stream and write them into the buffer:
  uint8_t* buf = (uint8_t*) buffer;
  for(uint64_t i = 0; i < numToRead; i++)
    buf[i] = csd->data[csd->pos+i];
  csd->pos += numToRead;

  // Return the number of bytes consumed (i.e. written into the buffer):
  return numToRead;
}

