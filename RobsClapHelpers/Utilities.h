#pragma once

/** A collection of macros, functions and classes that are useful in programming audio 
plugins. */


/** Function to convert a float or double to a string in a roundtrip safe way. That means, when 
parsing the produced string with e.g. std::atof, we get the original value back exactly. Can be 
used as replacement for std::to_string when an exact roundtrip is required. */
template<typename T>
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
  // -Maybe move this into a file with helper functions.
}