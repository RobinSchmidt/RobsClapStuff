


int toStringWithSuffix(double value, char* dest, int size, int numDigits, const char* suffix)
{
  //int pos = 0;

  char format[] = "%.2f";

  //int pos = sprintf_s(dest, size, "%.2f", value);

  int pos = sprintf_s(dest, size, format, value);


  /*
  if(dest == nullptr || size < 1)
    return pos;

  if(value < 0.0)
  {
    dest[0] = '-';
    pos++;
  }

  double tmp = abs(value);
  */




  return 0;  // preliminary
}



/*

ToDo:

Add:
-assertions that trigger debug breaks
-memory leak checks
-amp2db, db2amp, pitch2freq, freq2pitch, lin2lin, lin2exp, ...
-string conversion functions for the parameters


*/