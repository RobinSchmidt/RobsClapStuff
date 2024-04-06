#pragma once

/** A collection of macros, functions and classes that are useful in programming audio 
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


//=================================================================================================

/** A class that allows for mapping back and forth between indices and corresponding identifiers in
constant time. It's like a bidirectional key/value map where both, key and value, are some sort of 
integer. ...TBC...  */

class IndexIdentifierMap
{

public:

  void addIndexIdentifierPair(uint32_t index, clap_id id);
  // Maybe return a bool that reports success/failure

  /** Returns the identifier that corresponds to the given index. */
  clap_id getIdentifier(uint32_t index) const
  {
    assert(isValidIndex(index));
    return identifiers[index];
  }

  /** Returns the index that corresponds to the given identifier. */
  uint32_t getIndex(clap_id identifier) const 
  {
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

  //bool isConsistent() const;
  // Check for internal consistency

protected:

  std::vector<clap_id>  identifiers;
  std::vector<uint32_t> indices;

};

// ToDo:
// -Maybe templatize on IndexType (uint32_t), IdentifierType (clap_id)
// -Include functions for checking internal consistency. This is useful for unit tests and finding
//  bugs via assertions. The invariants to be maintained are:
//  -indices and identifiers arrays must have the same length
//  -indices and identifiers contain a permutation of 0...N-1  where N is the number of entries
//  -for all i in 0...N-1: identifiers[indices[i]] == i  and  indices[identifiers[i]] == i