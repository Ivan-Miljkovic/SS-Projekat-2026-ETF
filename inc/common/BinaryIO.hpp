#ifndef BINARYIO_H
#define BINARYIO_H

#include <iostream>
#include <cstdint>
#include <vector>
#include "Instruction.hpp"
class BinaryIO
{
public:
  void writeInstructionBinary(Instruction instruction);
  void changeDisplacement(uint32_t addr, int32_t disp);
  void writeByte(uint8_t byte);
  void writeWord(uint32_t data);
  void changeWord(uint32_t addr, uint32_t data);
  void printOutput(std::ostream &os);
  void printOutputBinary(std::ostream &os);
  void loadFromFile(std::istream &is);
  void resize(uint32_t size);
  uint8_t readByte(uint32_t addr);
  void changeByte(uint32_t addr, uint8_t byte);
  void clear();

private:
  std::vector<uint8_t> binaryOut;
};

#endif