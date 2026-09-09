#ifndef assembler
#define assembler

#include "../common/SymbolTable.hpp"
#include "../common/BinaryIO.hpp"
#include "literalPool.hpp"
#include "../common/RelocationTable.hpp"
#include <iostream>
#include <string>

using namespace std;

class Assembler
{

public:
  static int start(int argc, char *argv[]);
  static SymbolTable symTable;

  static void handleSection(std::string *name);
  static SymbolTable::Symbol *getCurrentSection();
  static void updateSectionSize();

  static void handleGlobal(std::string *name);
  static void handleExtern(std::string *name);
  static void handleAscii(std::string *str);
  static void handleEnd();

  static void handleLabel(std::string *name);

  static void handleWordSymbol(std::string *name);
  static void handleWordLiteral(uint32_t value);

  static void handleSkip(uint32_t size);

  static void handleZeroArgInstruction(Instruction::OCMOD ocmod);

  static void handleRet();
  static void handleIRet();

  static void handleGPRInstruction(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS);

  static void handlePush(uint8_t gpr);
  static void handlePop(uint8_t gpr);

  static void handleJumpLiteralInstruction(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, int32_t value);
  static void handleJumpSymbolInstruction(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, std::string *name);

  static void handleLoadStoreDirLiteral(Instruction::OCMOD ocmod, uint8_t gpr, int32_t value);
  static void handleLoadStoreDirSymbol(Instruction::OCMOD ocmod, uint8_t gpr, std::string *name);

  static void handleLoadStoreIndLiteral(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, uint8_t gpr3, int32_t value);
  static void handleLoadStoreIndSymbol(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, uint8_t gpr3, std::string *name);

  static void handleLoadStoreRegInstruction(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS);

  static void handleLoadStoreRegLiteral(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS, int32_t value);
  static void handleLoadStoreRegSymbol(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS, std::string *name);

private:
  typedef struct Backpatch
  {
    uint32_t offset;
    SymbolTable::Symbol *symbol;
    SymbolTable::Symbol *section;

    Backpatch(uint32_t offset, SymbolTable::Symbol *symbol, SymbolTable::Symbol *section) : offset(offset), symbol(symbol), section(section) {}
  } Backpatch;

  static char *inputFile;
  static char *outputFile;
  static int parseArguments(int argc, char *argv[]);

  static void handleLiteralBackpatch();
  static void handleSymbolBackpatch();

  static SymbolTable::Symbol *current_section;
  static uint32_t current_section_size;

  static BinaryIO binaryIO;
  static LiteralPool literalPool;
  static std::list<Backpatch> backpatchList;
  static RelocationTable relTable;
};

#endif