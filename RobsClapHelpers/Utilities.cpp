
int toStringWithSuffix(double value, char* dest, int size, int numDigits, const char* suffix)
{
  if(dest == nullptr || size < 1)
    return -1;   // -1 indicates error just like in sprintf_s

  // Crate a temporary std::string using ostringstream:
  std::ostringstream os;
  os.precision(numDigits);
  if(abs(value) <= 1.e15)         // Use fixed point notation for numbers with abs <= 10^15
    os << std::fixed;
  os << value;
  std::string str = os.str(); 
  if(suffix != nullptr)
    str += suffix;

  // Copy (an initial section of) the temporary string into the destination buffer:
  const char* cStr = str.c_str();
  int nullPos = std::min(size-1, (int)str.size());  // Position of terminating null
  for(int i = 0; i < nullPos; i++)
    dest[i] = cStr[i];
  dest[nullPos] = '\0';
  return nullPos;

  // Notes:
  //
  // -The use of std::ostringstream makes the implementation somewhat slow compared to a less safe
  //  solution using a function from the sprintf family. In the future with C++20, we may switch to
  //  std::format. For the time being, I think, this is OK. Conversion of doubles to strings is 
  //  typically not performance critical in the context of audio plugins.

  // See:
  // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/sprintf-s-sprintf-s-l-swprintf-s-swprintf-s-l?view=msvc-170
  // https://stackoverflow.com/questions/76029660/what-is-the-difference-between-sprintf-s-and-snprintf
  // Whoa!:
  // snprintf will truncate but sprintf_s will call a handler which, by default, will raise an 
  // exception and (most likely) terminate the program.
  //
  // https://stackoverflow.com/questions/1505986/sprintf-s-with-a-buffer-too-small
  // https://www.ryanjuckett.com/printing-floating-point-numbers/

  // This says something about stringstream using locale information - could that mean that on some
  // machines, we get commas instead of dots? That would break state recall!!
  // http://cplusplus.bordoon.com/speeding_up_string_conversions.html
  // https://mariusbancila.ro/blog/2023/09/12/formatting-text-in-c-the-old-and-the-new-ways/
  // https://iq.opengenus.org/convert-double-to-string-in-cpp/
}

int copyString(const char* src, char* dst, int dstSize)
{
  if(dst == nullptr || dstSize < 1)
    return -1;

  int i = 0;
  while(i < dstSize-1 && src[i] != '\0')
  {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';

  return i;
}

bool copyString(const std::vector<std::string>& strings, int index, char* dst, int dstSize)
{
  if(index >= 0 && index < (int) strings.size())
    return copyString(strings[index].c_str(), dst, dstSize) > 0;
  else
    return false;
}

int findString(const std::vector<std::string>& strings, const char* stringToFind)
{
  for(size_t i = 0; i < strings.size(); i++)
  {
    if(strcmp(strings[i].c_str(), stringToFind) == 0)
      return (int) i;
  }
  return -1;  // -1 Encodes "not found".
}


//=================================================================================================

void IndexIdentifierMap::addIndexIdentifierPair(uint32_t index, clap_id id)
{
  size_t newSize = std::max(std::max(index+1, id+1), getNumEntries());

  indices.resize(newSize);
  identifiers.resize(newSize);

  identifiers[index] = id;
  indices[id]        = index;

  // ToDo:
  //
  // -Maybe include an assertion that ensures that the given pair does not try to mess something 
  //  up by overwriting existing data.
}

bool IndexIdentifierMap::isConsistent() const
{
  // The indices and identifiers arrays must have the same size:
  if(indices.size() != identifiers.size())
    return false;

  // Check the consistency conditions for defining a bijective map:
  int N = getNumEntries();
  for(int i = 0; i < N; i++)
  {
    // In both of our arrays, each number from 0 to N-1 must occur exactly once:
    int count1 = countOccurrences(&indices[0],     N, (uint32_t) i);
    int count2 = countOccurrences(&identifiers[0], N, (clap_id)  i);
    if(count1 != 1 || count2 != 1)
      return false;

    // When mapping from index to id and back (or vice versa), we should get our original number
    // back for every i in 0..N-1:
    int j = identifiers[indices[i]];
    int k = indices[identifiers[i]];
    if(j != i || j != i)
      return false;
  }
  
  return true;  // All consistency checks passed
}

//=================================================================================================

bool ValueFormatter::valueToText(double value, char* text, uint32_t size)
{
  return toStringWithSuffix(value, text, size, 2) > 0;
}

bool ValueFormatter::textToValue(const char* text, double* value)
{
  *value = strtod(text, nullptr);
  return true;

  // ToDo:
  //
  // -Figure out what happens in case of a parse-error. It would be nice to have a parser that 
  //  returns a bool (true in case of success, false in case of failure).
}


bool ValueFormatterWithSuffix::valueToText(double value, char* text, uint32_t size)
{
  return toStringWithSuffix(value, text, size, precision, suffix.c_str()) > 0;
}


bool ValueFormatterForChoice::valueToText(double value, char* text, uint32_t size)
{
  return copyString(choices, (int) round(value), text, size);
}

bool ValueFormatterForChoice::textToValue(const char* text, double* value)
{
  int i = findString(choices, text);
  if(i == -1) { *value = 0.0;        return false; }
  else        { *value = (double) i; return true;  }
}




/*=================================================================================================

ToDo:

Add:
-assertions that trigger debug breaks
-memory leak checks
-amp2db, db2amp, pitch2freq, freq2pitch, lin2lin, lin2exp, ...
-string conversion functions for the parameters


*/