#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <string>
#include <map>

#define MEMORY_SIZE 1 << 32

class Memory
{
public:
  static void writeByte(uint32_t addr, uint8_t byte);
  static uint8_t readByte(uint32_t addr);
  static void writeWord(uint32_t addr, uint32_t value);
  static uint32_t readWord(uint32_t addr);
  static int loadMemory(std::string file);
  static void printMemory();

private:
  static std::map<uint32_t, uint8_t> memory;
};

#endif