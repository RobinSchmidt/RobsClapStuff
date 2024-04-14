#pragma once

/** A collection of templates, functions and classes that are useful in programming audio 
plugins. */


template<class T>
inline T clip(T val, T min, T max)
{
  if(val < min) return min;
  if(val > max) return max;
  return val;

  // ToDo: Maybe use std::min/max
}


/** Converts from decibels to raw amplitude. */
template<class T>
inline T dbToAmp(T db)
{
  return pow(T(10), T(1.0/20) * db);
}
// ToDo: 
// -Verify formulas - or:
// -Use optimized formula using exp (copy the code from RAPT)
// -Add ampToDb, pitchToFreq, freqToPitch, etc.


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


int copyString(const char* src, char* dst, int dstSize);
// ToDo: add documentation

bool copyString(const std::vector<std::string>& strings, int index, char* dst, int dstSize);
// ToDo: add documentation

int findString(const std::vector<std::string>& strings, const char* stringToFind);
// ToDo: add documentation


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


//=================================================================================================

/** A class that allows for mapping back and forth between indices and corresponding identifiers in
constant time. It's like a bidirectional key/value map where both, key and value, are some sort of 
integer. ...TBC...  */

class IndexIdentifierMap
{

public:

  void addIndexIdentifierPair(uint32_t index, clap_id id);
  // ToDo:
  // -Maybe return a bool that reports success/failure.
  // -Document usage. Explain how during building the map, it may temporarily be in an inconsistent
  //  state (depending on the order in which the entries are added) but at the very end, when 
  //  building it is finished, the state should be consistent - which should probably be checked by
  //  something like  assert(myMap.isConsistent()).
  // -Maybe rename to addEntry

  /** Returns the identifier that corresponds to the given index. */
  clap_id getIdentifier(uint32_t index) const
  {
    assert(isValidIndex(index));
    return identifiers[index];
  }

  /** Returns the index that corresponds to the given identifier. */
  uint32_t getIndex(clap_id identifier) const 
  {
    assert(isValidIdentifier(identifier));
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
  like  assert(myMap.isConsistent());  to make sure you didn't make any mistakes during filling the 
  map. */
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
