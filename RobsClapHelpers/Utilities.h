#pragma once

/** A collection of macros templates, functions and classes that are useful in programming audio 
plugins. */


//=================================================================================================
// Debugging

#if defined(DEBUG) || defined(_DEBUG)
  #define CLAP_DEBUG
#endif

#if defined(CLAP_DEBUG)
  #ifdef _MSC_VER
    #pragma intrinsic (__debugbreak)
    #define CLAP_DEBUG_BREAK __debugbreak();
  #else 
    #define CLAP_DEBUG_BREAK { }
    //#define CLAP_DEBUG_BREAK __builtin_trap();  // Preliminary - GCC only?
  #endif
#endif
// ToDo: Reactivate the "#define CLAP_DEBUG_BREAK __builtin_trap();" and check, if that also works
// for clang. If not, put it into some sort of "#elif GCC" (look up what the macro actually is, may 
// be "__GNUC__" or something)

/** This function should be used to indicate a runtime error. */
inline void clapError(const char *message = nullptr)
{
#ifdef CLAP_DEBUG
  CLAP_DEBUG_BREAK;  // The "message" may (or may not) have more info about what happened
#endif
}

/** This function should be used for runtime assertions. */
inline void clapAssert(bool expression, const char *errorMessage = nullptr)
{
#ifdef CLAP_DEBUG
  if( expression == false )
    clapError(errorMessage);
#endif
}

/** Extracts the input events from teh given processing buffer into a std::vector for easy 
inspection in the debugger. In the process(...) function, you can just add a line like:

  auto inEvents = extractInEvents(p);

and then you can inspect the event list. */
inline std::vector<clap_event_header_t> extractInEvents(const clap_process* p)
{
  std::vector<clap_event_header_t> inEvents;
  if(p->in_events == nullptr)  // Should not occur in normal use, but may in the unit tests
    return inEvents;

  const uint32_t numEvents = p->in_events->size(p->in_events);
  inEvents.resize(numEvents);
  for(int i = 0; i < numEvents; i++)
    inEvents[i] = *(p->in_events->get(p->in_events, i));
  return inEvents;
}
// Maybe un-inline - move implementation to cpp file

//=================================================================================================
// Math

template<class T>
inline T clip(T val, T min, T max)
{
  if(val < min) return min;
  if(val > max) return max;
  return val;

  // ToDo: Maybe use std::min/max or std::clamp
}

/** Converts from decibels to raw amplitude. */
template<class T>
inline T dbToAmp(T dB)
{
  return exp(dB * T(0.11512925464970228420089957273422));
  //return pow(T(10), T(1.0/20) * dB);  // Naive, expensive formula
}
// ToDo: 
// -Verify formulas - or:
// -Use optimized formula using exp (copy the code from RAPT)
// -Add ampToDb, pitchToFreq, freqToPitch, etc.

/** Converts a MIDI-note value into a frequency in Hz assuming A4 = 440 Hz. Uses two 
multiplications and one call to exp. */
template<class T>
inline T pitchToFreq(T pitch)
{
  return T(8.1757989156437073336828122976033) 
           * exp(T(0.057762265046662109118102676788181) * pitch);
  //return 440.0*( pow(2.0, (pitch-69.0)/12.0) ); // naive, slower but numerically more precise
}

//=================================================================================================
// Arrays

/** Counts the number of times by which the given "element" occurrs in the "buffer" with given 
"length". */
template<class T>
inline int countOccurrences(const T* buffer, int length, const T& element)
{
  int count = 0;
  for(int i = 0; i < length; i++)
    if(buffer[i] == element)
      count++;
  return count;
}

/** Compares the two given buffers with given length for equality in the sense that all entries
must be equal. */
template<class T>
inline bool equals(const T* buf1, const T* buf2, int length)
{
  for(int i = 0; i < length; i++)
    if(buf1[i] != buf2[i])
      return false;
  return true;
}



//=================================================================================================
// Strings

/** Function to convert a float or double to a string in a roundtrip safe way. That means, when 
parsing the produced string with e.g. std::atof, we get the original value back exactly. Can be 
used as replacement for std::to_string when an exact roundtrip is required. This is used in the
implementation of the state save/load functionality. */
template<class T>
inline std::string toStringExact(T x)
{
  std::ostringstream os;
  os << std::setprecision(std::numeric_limits<T>::max_digits10) << x;
  return os.str();

  // Notes:
  //
  // -The implementation is taken from:
  //  https://possiblywrong.wordpress.com/2015/06/21/floating-point-round-trips/
  //
  // ToDo:
  //
  // -The function needs some unit tests with some more extreme values (close to 0, near the 
  //  range-max, denormals, etc.)
}

/** Function to convert a double to a string with a suffix (for a physical unit) for display on 
the GUI. It can be used without suffix as well. You may just pass a nullptr for the suffix in such 
a case. The "size" is the total size of the "destination" buffer, i.e. including the place for the 
terminating null. */
int toStringWithSuffix(double value, char* destination, int size, int numDigitsAfterDot,
  const char* suffix = nullptr);
// ToDo: Document the return value. It's the index of the null-terminator, so it's the length of 
// the string excluding the null-terminator or -1 in case of failure. This is consistent with the
// behavior of sprintf.

int copyString(const char* src, char* dst, int dstSize);
// ToDo: add documentation

bool copyString(const std::vector<std::string>& strings, int index, char* dst, int dstSize);
// ToDo: add documentation

int findString(const std::vector<std::string>& strings, const char* stringToFind);
// ToDo: add documentation


//=================================================================================================

/** A class that allows for mapping back and forth between indices and corresponding identifiers in
constant time. It's like a bidirectional key/value map where both, key and value, are some sort of 
integer. It's meant for mapping back and forth between parameter indices and corresponding 
identifiers (ids, for short). In CLAP, the id for a parameter is a unique number that the plugin 
itself can choose. If we choose the list of identifiers to be a permutation of the list of indices,
we get all the relevant benefits of the additional flexibility of such an id-system (mostly, the 
ability to re-order the knobs in the host-generated GUI in updated versions of the plugin) while at
the same time not introducing as much overhead as we would have to when using "random" numbers for
the ids. So, that's the route we choose here: the mapping between index and id is a bijective 
mapping (i.e. a permutation) of the set { 0, ..., N-1 } to itself where N is the number of 
parameters. This can easily be implemented by a pair of arrays (e.g. std::vector) of length N. No 
need for (moderately) complex data structures like a std::map or something similar.  */

class IndexIdentifierMap
{

public:

  void addIndexIdentifierPair(uint32_t index, clap_id id);
  // ToDo:
  // -Maybe return a bool that reports success/failure.
  // -Document usage. Explain how during building the map, it may temporarily be in an inconsistent
  //  state (depending on the order in which the entries are added) but at the very end, when 
  //  building it is finished, the state should be consistent - which should probably be checked by
  //  something like  clapAssert(myMap.isConsistent()).
  // -Maybe rename to addEntry

  /** Returns the identifier that corresponds to the given index. */
  clap_id getIdentifier(uint32_t index) const
  {
    clapAssert(isValidIndex(index));
    return identifiers[index];
  }

  /** Returns the index that corresponds to the given identifier. */
  uint32_t getIndex(clap_id identifier) const 
  {
    clapAssert(isValidIdentifier(identifier));
    return indices[identifier];
  }

  /** Returns the number of entries, i.e. the number of stored index/identifier pairs. */
  uint32_t getNumEntries() const
  {
    return (uint32_t) indices.size(); // == identifiers.size() as well
  }

  bool isValidIndex(uint32_t index) const 
  {
    return index >= 0 && index < getNumEntries();
  }

  bool isValidIdentifier(clap_id identifier) const 
  {
    return identifier >= 0 && identifier < (clap_id) getNumEntries();
  }


  /** Checks the map for internal consistency. This is useful for unit testing of the class and for
  catching bugs on the client code side via assertions. After filling a map, you can add a line 
  like  clapAssert(myMap.isConsistent());  to make sure you didn't make any mistakes during filling the 
  map. A mistake would be, for example, to use an id twice or not at all which can easily happen, I 
  guess. */
  bool isConsistent() const;

protected:

  std::vector<clap_id>  identifiers;
  std::vector<uint32_t> indices;

};

// ToDo:
// -Add a reserve function to pre-allocate the desired memory upfront
// -Add a clear function and maybe a removeIndexIdentifierPair function...although, we currently
//  don't need that
// -Maybe templatize on IndexType (uint32_t), IdentifierType (clap_id)
