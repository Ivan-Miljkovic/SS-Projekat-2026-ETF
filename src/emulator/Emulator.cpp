#include "../../inc/emulator/Emulator.hpp"
#include "../../inc/emulator/CPU.hpp"
#include "../../inc/emulator/Memory.hpp"
#include "../../inc/emulator/Terminal.hpp"
#include <fstream>

char *Emulator::inputFile;

void Emulator::parseArguments(int argc, char *argv[])
{
  if (argc != 2)
  {
    // error
    return;
  }

  inputFile = argv[1];
}

void Emulator::start(int argc, char *argv[])
{
  parseArguments(argc, argv);

  Memory::loadMemory(std::string(inputFile));

  CPU::reset();

  Terminal::start();

  while (CPU::isRunning())
  {
    CPU::fetch();
    CPU::decode();
    CPU::execute();
    CPU::intr();
  }

  Terminal::stop();
  CPU::printRegisters();
}
