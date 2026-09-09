#include <string>
#ifndef EMULATOR_HPP
#define EMULATOR_HPP

class Emulator
{
public:
  static void start(int argc, char *argv[]);
  static void parseArguments(int argc, char *argv[]);

private:
  static char *inputFile;
};

#endif