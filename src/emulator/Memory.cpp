#include "../../inc/emulator/Memory.hpp"
#include "../../inc/emulator/Terminal.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <iomanip>

std::map<uint32_t, uint8_t> Memory::memory;

uint8_t Memory::readByte(uint32_t addr)
{
  return memory[addr];
}

void Memory::writeByte(uint32_t addr, uint8_t byte)
{
  if (addr == 0xFFFFFF00) // terminal
  {
    std::cout << (char)byte;
    std::cout.flush();
  }
  memory[addr] = byte;
}

uint32_t Memory::readWord(uint32_t addr)
{

  uint32_t data = 0;
  uint8_t byte = 0;

  for (int i = 0; i < 4; i++)
  {
    byte = memory[addr + i];
    data = data | ((uint32_t)byte << (8 * i));
  }

  return data;
}

void Memory::writeWord(uint32_t addr, uint32_t value)
{

  if (addr == TERM_OUT_ADDR) // terminal
  {
    std::cout << (char)value;
    std::cout.flush();
  }

  for (int i = 0; i < 4; i++)
  {
    memory[addr + i] = (uint8_t)value;
    value >>= 8;
  }
}

int Memory::loadMemory(std::string file)
{
  memory.clear();

  std::ifstream inputFile(file, std::ios::binary);

  if (!inputFile.is_open())
  {
    return -1;
  }
  uint32_t address;
  uint8_t byte;
  uint32_t sz = 0;
  std::string line;
  while (std::getline(inputFile, line))
  {
    if (line.empty())
      continue;
    std::stringstream ss(line);

    std::string addressStr;
    ss >> addressStr;

    addressStr.pop_back(); // pops the ':'
    address = std::stoul(addressStr, nullptr, 16);
    std::string byteStr;

    while (ss >> byteStr)
    {
      byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
      memory[address++] = byte;
    }
  }
  inputFile.close();
  return 0;
}

void Memory::printMemory()
{
  for (auto &entry : Memory::memory)
  {
    std::cout << std::hex << std::setw(4) << std::setfill('0') << entry.first
              << ": " << std::setw(2) << (int)entry.second << " ";
  }
}