
#include <iostream>

#include "ClapPluginTests.h"

int main()
{  
  std::cout << "Unit tests for Robin's CLAP wrapper classes.\n\n";

  // Run the tests:
  bool ok = true;
  ok = runAllClapTests(true);

  // Report test results:
  if(ok)
    std::cout << "Unit tests passed.";
  else
    std::cout << "!!! UNIT TESTS FAILED !!!";
  getchar();
}