#include "../../inc/common/BinaryIO.hpp"
#include <iomanip>

void BinaryIO::writeInstructionBinary(Instruction instruction)
{
  // OCMOD AB CD DD
  // little endian DD CD AB OC MOD
  binaryOut.push_back((uint8_t)instruction.disp); // first byte; (DD)
  // move 8 bits to the right to get the first 4 bits of disp and add the rc bits
  binaryOut.push_back((uint8_t)((instruction.disp >> 8) & 0x0F) | (instruction.rc << 4)); // second byte (CD)
  binaryOut.push_back(((instruction.ra) << 4) | (instruction.rb));                        // third byte (AB)
  binaryOut.push_back(instruction.ocmod);                                                 // forth byte (OCMOD)
}

void BinaryIO::changeDisplacement(uint32_t addr, int32_t disp)
{                                                  // DD CD AB OCMOD
  uint8_t secondByte = binaryOut[addr + 1] & 0xF0; // get the second byte of the instruction, and clear the disp
  secondByte = secondByte | (disp >> 8) & 0x0F;    // add the first four bits of disp
  binaryOut[addr + 1] = secondByte;                // CD
  binaryOut[addr] = disp;                          // DD
}

void BinaryIO::writeByte(uint8_t byte)
{
  binaryOut.push_back(byte);
}

void BinaryIO::writeWord(uint32_t data)
{
  for (int i = 0; i < 4; i++)
  {
    binaryOut.push_back(data);
    data >>= 8;
  }
}

void BinaryIO::changeWord(uint32_t addr, uint32_t data)
{
  for (int i = 0; i < 4; i++)
  {
    binaryOut[addr + i] = (uint8_t)data;
    data >>= 8;
  }
}

void BinaryIO::printOutput(std::ostream &os)
{

  for (uint32_t i = 0; i < binaryOut.size(); i++)
  {

    if ((i) % 8 == 0)
      os << ((i != 0) ? "\n" : "") << std::right << std::uppercase << std::setw(4) << std::setfill('0') << std::hex << i << ":";

    os << " " << std::right << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(binaryOut[i]);
  }
  os << std::dec << std::endl;
}

void BinaryIO::printOutputBinary(std::ostream &os)
{
  size_t size = binaryOut.size();

  // print size so linker would know how many bytes to read
  os.write(reinterpret_cast<const char *>(&size), sizeof(size));

  for (uint32_t i = 0; i < binaryOut.size(); i++)
  {
    os.write(reinterpret_cast<const char *>(&binaryOut[i]), sizeof(binaryOut[i]));
  }
}

void BinaryIO::loadFromFile(std::istream &is)
{
  size_t size;

  // Read number of bytes
  is.read(reinterpret_cast<char *>(&size), sizeof(size));

  // Allocate space
  binaryOut.resize(size);

  // Read binary data
  is.read(reinterpret_cast<char *>(binaryOut.data()), size);
}

void BinaryIO::resize(uint32_t size)
{
  binaryOut.resize(size);
}

uint8_t BinaryIO::readByte(uint32_t addr)
{
  return binaryOut[addr];
}

void BinaryIO::changeByte(uint32_t addr, uint8_t byte)
{
  binaryOut[addr] = byte;
}

void BinaryIO::clear()
{
  binaryOut.clear();
}