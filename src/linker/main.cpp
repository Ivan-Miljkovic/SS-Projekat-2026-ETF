#include "../../inc/linker/LinkerException.hpp"
#include "../../inc/linker/Linker.hpp"
#include <execinfo.h>
#include <cstdlib>
#include <iostream>

void printStackTrace()
{
  void *array[20];
  int size = backtrace(array, 20);
  char **strings = backtrace_symbols(array, size);

  for (int i = 0; i < size; i++)
    std::cerr << strings[i] << '\n';

  free(strings);
}
int main(int argc, char *argv[])
{
  try
  {
    Linker::Start(argc, argv);
  }
  catch (const LinkerException &e)
  {
    std::cout << e.what() << '\n';
    printStackTrace();
    return -1;
  }
  return 0;
}