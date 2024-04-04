


int toStringWithSuffix(double value, char* dest, int size, int numDigits, const char* suffix)
{
  // Crate a temporary std::string using ostringstream:
  std::ostringstream os;
  os.precision(numDigits);
  if(abs(value) <= 1.e15)       // Ad hoc: use exponential notation for numbers > 10^15
    os << std::fixed;
  os << value;
  std::string str = os.str(); 
  if(suffix != nullptr)
    str += suffix;

  // Copy (an initial section of) the temporary string into the destination buffer:
  const char* cStr = str.c_str();
  int terminator = std::min(size-1, (int)str.size());
  for(int i = 0; i < terminator; i++)
    dest[i] = cStr[i];
  dest[terminator] = '\0';

  return terminator;

  //return os.str();









  /*
  // For safety reasons, we first write the string into an internal buffer that is large enough for
  // the worst case

  static const int tmpSize = 32; // Ad hoc - verify if this is enough in the worst case!
  char tmp[tmpSize]; 

  // Create the number string via sprintf_s:
  char format[] = "%.2f";
  char d = '0' + (char) clip(numDigits, 0, 9); // We don't support more than 9 digits after the dot
  format[2] = d;                               // Use decimal notation by default
  if(abs(value) >= 1.e15)                      // Use exponential notation for very large values
    format[3] = 'e';

  int pos = sprintf_s(tmp, tmpSize, format, value);

  // Copy the result from the temporary buffer into the destination (with truncation, if needed):
  int copyLength = std::min(size-1, pos-1);
  for(int i = 0; i < copyLength; i++)
    dest[i] = tmp[i];

  if(suffix == nullptr)
  {

  }

  // Append suffix:
  int i = 0;
  while(pos+i < size-1 && 

  return pos;
  */


  //// Append the suffix via strcpy_s:
  //if(suffix != nullptr)
  //  pos += strcpy_s(&tmp[pos], tmpSize-pos, suffix);





  // 




  // ...this seems to be all unsafe! Maybe use C++ std::stringstream and then copy the result over
  // into the provided buffer

  // ...Maybe switch to exponential notation when the number is larger than 10^15, i.e 1.e15

  // See:
  // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/sprintf-s-sprintf-s-l-swprintf-s-swprintf-s-l?view=msvc-170
  // https://stackoverflow.com/questions/76029660/what-is-the-difference-between-sprintf-s-and-snprintf
  // Whoa!:
  // snprintf will truncate but sprintf_s will call a handler which, by default, will raise an 
  // exception and (most likely) terminate the program.
  //
  // https://stackoverflow.com/questions/1505986/sprintf-s-with-a-buffer-too-small
  // https://www.ryanjuckett.com/printing-floating-point-numbers/

  // This says something about straingstream using locale inromation - could that mean that on some
  // machines, we get commas instead of dots? That would break state recall!!
  // http://cplusplus.bordoon.com/speeding_up_string_conversions.html
  // https://mariusbancila.ro/blog/2023/09/12/formatting-text-in-c-the-old-and-the-new-ways/
  // https://iq.opengenus.org/convert-double-to-string-in-cpp/
}



/*

ToDo:

Add:
-assertions that trigger debug breaks
-memory leak checks
-amp2db, db2amp, pitch2freq, freq2pitch, lin2lin, lin2exp, ...
-string conversion functions for the parameters


*/