


int toStringWithSuffix(double value, char* dest, int size, int numDigits, const char* suffix)
{
  // Create the number string via sprintf_s:
  char format[] = "%.2f";
  char d = '0' + (char) clip(numDigits, 0, 9); // We don't support more than 9 digits after the dot
  format[2] = d;
  int pos = sprintf_s(dest, size, format, value);

  // Append the suffix via strcpy_s:
  if(suffix != nullptr)
    pos += strcpy_s(&dest[pos], size-pos, suffix);


  return pos;
}



/*

ToDo:

Add:
-assertions that trigger debug breaks
-memory leak checks
-amp2db, db2amp, pitch2freq, freq2pitch, lin2lin, lin2exp, ...
-string conversion functions for the parameters


*/