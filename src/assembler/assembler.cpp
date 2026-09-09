#include <string>
#include <iostream>
#include "../../inc/assembler/assembler.hpp"
#include "../../inc/common/SymbolTable.hpp"
#include "../../inc/assembler/assemblerException.hpp"
#include <cstring>
#include <fstream>
extern int yyparse(void);
extern void yyerror(const char *);
extern FILE *yyin;
extern int yydebug;

SymbolTable Assembler::symTable;
SymbolTable::Symbol *Assembler::current_section;
uint32_t Assembler::current_section_size;
BinaryIO Assembler::binaryIO;
LiteralPool Assembler::literalPool;
RelocationTable Assembler::relTable;
std::list<Assembler::Backpatch> Assembler::backpatchList;
char *Assembler::inputFile;
char *Assembler::outputFile;

static int fileCount = 1;

bool isValue12b(int32_t value)
{
  return (value >= -2048 && value < 2048);
}

int Assembler::parseArguments(int argc, char *argv[])
{
  if (argc < 2)
    return -1;

  if (!strcmp("-o", argv[1]))
  {
    if (argc != 4)
      return -1;
    outputFile = argv[2];
    inputFile = argv[3];
    return 0;
  }

  else
  {
    inputFile = argv[1];
    outputFile = nullptr;
    return 0;
  }

  return -1;
}

int Assembler::start(int argc, char *argv[])
{
  binaryIO.clear();
  relTable.clear();
  literalPool.clear();
  yydebug = 0;
  if (parseArguments(argc, argv) < 0)
  {
    throw AssemblerException("Invalid Arguments");
  }
  if (!inputFile)
  {
    throw AssemblerException("No input file");
  }
  std::string out;
  if (!outputFile)
  {
    std::string in = inputFile;
    out = in.substr(0, in.size() - 1) + "o";
  }
  else
  {
    out = (std::string)outputFile;
  }

  FILE *f = fopen(inputFile, "r");
  if (!f)
  {
    throw AssemblerException("Cant open file: ");
  }
  // init table
  symTable.clear();
  current_section = symTable.getSymbol("UND");
  current_section_size = 0;
  // parse file
  yyin = f;
  yyparse();
  fclose(yyin);

  binaryIO.printOutput(std::cout);
  symTable.printTable();
  relTable.printTable(std::cout, &symTable);
  if (out.empty())
  {
    throw AssemblerException("Output file is null");
  }
  std::ofstream outputBinary(std::string(out), std::ios::binary);
  if (!outputBinary.is_open())
  {
    throw AssemblerException("can't open output file");
    return -1;
  }
  binaryIO.printOutputBinary(outputBinary);
  symTable.printTableBinary(outputBinary);
  relTable.printTableBinary(outputBinary, &symTable);
  outputBinary.close();
  fileCount++;
  return 0;
}

SymbolTable::Symbol *Assembler::getCurrentSection()
{
  return current_section;
}

void Assembler::handleSection(std::string *name)
{

  if (symTable.getSymbol(*name))
  {
    throw AssemblerException("Section already defined");
    return;
  }

  handleLiteralBackpatch();
  current_section->size += current_section_size; // update section size
  current_section = symTable.addEntry(name, 0, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::SECTION, 0);
  current_section_size = 0;
}

void Assembler::handleGlobal(std::string *name)
{
  if (!symTable.getSymbol(*name))
  {
    symTable.addEntry(name, current_section_size, SymbolTable::SYM_BIND::SYMBOL_GLOBAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
    return;
  }
  else
  {
    symTable.getSymbol(*name)->bind = SymbolTable::SYM_BIND::SYMBOL_GLOBAL;
  }
}
void Assembler::handleExtern(std::string *name)
{
  if (!symTable.getSymbol(*name))
  {
    SymbolTable::Symbol *s = symTable.addEntry(name, current_section_size, SymbolTable::SYM_BIND::SYMBOL_GLOBAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
    return;
  }
  else
  {
    symTable.getSymbol(*name)->bind = SymbolTable::SYM_BIND::SYMBOL_GLOBAL;
  }
}

void Assembler::handleAscii(std::string *str)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  for (uint8_t c : *str)
  {
    // print to binary output
    binaryIO.writeByte(c);
    current_section_size++;
  }
}

void Assembler::handleLabel(std::string *name)
{
  // update symbol table
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  SymbolTable::Symbol *s = symTable.getSymbol(*name);
  if (s)
  {
    if (s->type != SymbolTable::SYM_TYPE::UNDEFINED) // already defined
    {
      throw AssemblerException("Symbol already defined, label");
    }
    s->offset = current_section_size;
    s->index = current_section->index;
    s->type = SymbolTable::SYM_TYPE::NOTYPE;
    return;
  }
  // not in the table, create a new symbol
  symTable.addEntry(name, current_section_size, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::NOTYPE, 0);
}
void Assembler::handleEnd()
{
  handleLiteralBackpatch();
  current_section->size += current_section_size; // update section size
  current_section = nullptr;
  current_section_size = 0;
  handleSymbolBackpatch();
}

void Assembler::handleWordLiteral(uint32_t value)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  binaryIO.writeWord(value);
  current_section_size += 4;
}

void Assembler::handleWordSymbol(std::string *name)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  SymbolTable::Symbol *sym = symTable.getSymbol(*name);
  if (!sym)
  {
    sym = symTable.addEntry(name, current_section_size, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
  }
  // backpatch it even if defined, too complicated to check it here
  binaryIO.writeWord(0);                                                 // resolve it in backpatch
  backpatchList.push_back({current_section_size, sym, current_section}); // put it into backpatch

  current_section_size += 4;
}

void Assembler::handleSkip(uint32_t size)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  // fill with zeroes;
  for (uint32_t i = 0; i < size; i++)
  {
    binaryIO.writeByte(0);
  }
  current_section_size += size;
}

void Assembler::handleZeroArgInstruction(Instruction::OCMOD ocmod)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {

  case Instruction::OCMOD::HALT:
  case Instruction::OCMOD::INT:
    binaryIO.writeInstructionBinary({ocmod});
    current_section_size += 4;
    break;
  default:
    throw AssemblerException("Invalid ZeroArg Instruction, check usage");
    break;
  }
}
void Assembler::handleRet()
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  binaryIO.writeInstructionBinary({Instruction::OCMOD::RET});
  current_section_size += 4;
}

void Assembler::handleIRet()
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  binaryIO.writeInstructionBinary({Instruction::OCMOD::IRET});
  current_section_size += 4;
}

void Assembler::handleGPRInstruction(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::XCHG:
    binaryIO.writeInstructionBinary({ocmod, gprD, gprS});
    break;
  case Instruction::OCMOD::ADD:
  case Instruction::OCMOD::SUB:
  case Instruction::OCMOD::MUL:
  case Instruction::OCMOD::DIV:
  case Instruction::OCMOD::AND:
  case Instruction::OCMOD::OR:
  case Instruction::OCMOD::XOR:
  case Instruction::OCMOD::SHL:
  case Instruction::OCMOD::SHR:
    binaryIO.writeInstructionBinary({ocmod, gprD, gprD, gprS});
    break;
  case Instruction::OCMOD::CSRRD:
  case Instruction::OCMOD::CSRWR:
  case Instruction::OCMOD::NOT:
    binaryIO.writeInstructionBinary({ocmod, gprD, gprS});
    break;

  default:
    throw AssemblerException("Invalid GPR Instruction, check usage");
  }

  current_section_size += 4;
}
void Assembler::handlePush(uint8_t gpr)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_PREINC, Instruction::SP, 0, gpr, -4});
  current_section_size += 4;
}

void Assembler::handlePop(uint8_t gpr)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_POSTINC, gpr, Instruction::SP, 0, 4});
  current_section_size += 4;
}

void Assembler::handleJumpLiteralInstruction(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, int32_t value)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::JMP_DIR:
  case Instruction::OCMOD::BEQ_DIR:
  case Instruction::OCMOD::BNE_DIR:
  case Instruction::OCMOD::BGT_DIR:
    if (isValue12b(value))
    {
      // does not need literal pool, just write the instruction as is
      binaryIO.writeInstructionBinary({ocmod, gpr1, gpr2, 0, value});
    }
    else
    {
      literalPool.put(value, current_section_size);
      // make the instruction indirect since, for example, jmp_dir = 0011 0000 and jmp_ind = 0011 1000 and so on
      binaryIO.writeInstructionBinary({(uint8_t)(ocmod | 0x8), Instruction::PC, gpr1, gpr2}); // write instruction, the disp will change
    }
    break;
  case Instruction::OCMOD::CALL_DIR:
    if (isValue12b(value))
    {
      binaryIO.writeInstructionBinary({Instruction::OCMOD::CALL_DIR, Instruction::PC, 0, 0, value});
    }
    else
    {
      literalPool.put(value, current_section_size);
      binaryIO.writeInstructionBinary({Instruction::OCMOD::CALL_IND, Instruction::PC, 0, 0, 0});
    }
    break;
  default:
    throw AssemblerException("Invalid Jump Literal Instruction, check usage");
    break;
  }
  current_section_size += 4;
}
void Assembler::handleJumpSymbolInstruction(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, std::string *name)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  SymbolTable::Symbol *sym = symTable.getSymbol(*name);
  if (!sym) // symbol not in symbol table, add it
  {
    sym = symTable.addEntry(name, 0, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
  }
  int32_t disp = (int32_t)sym->offset - (int32_t)(current_section_size + 4);
  switch (ocmod)
  {
  case Instruction::JMP_DIR:
  case Instruction::BEQ_DIR:
  case Instruction::BNE_DIR:
  case Instruction::BGT_DIR:
    // symbol not defined or displacement is too big or not in the same section
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || !isValue12b(disp) || sym->index != current_section->index)
    {
      // make a literal pool entry
      literalPool.put(0, current_section_size, name); // this will change in literal backpatch
      // make the instruction indirect since, for example, jmp_dir = 0011 0000 and jmp_ind = 0011 1000 and so on
      binaryIO.writeInstructionBinary({(uint8_t)(ocmod | 0x8), Instruction::PC, gpr1, gpr2});
    }
    else // symbol is defined and displacement is in 12 bits and in the same section
    {
      binaryIO.writeInstructionBinary({ocmod, Instruction::PC, gpr1, gpr2, disp});
    }
    break;
  case Instruction::CALL_DIR:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || !isValue12b(disp) || sym->index != current_section->index) // symbol not defined or displacement is too big
    {
      // make a literal pool entry
      literalPool.put(0, current_section_size, name);
      binaryIO.writeInstructionBinary({Instruction::OCMOD::CALL_IND, Instruction::PC, 0, 0, 0}); // write instruction, the disp will change
    }
    else // symbol is defined and displacement is in 12 bits
    {
      binaryIO.writeInstructionBinary({Instruction::OCMOD::CALL_DIR, Instruction::PC, 0, 0, disp});
    }
    break;
  default:
    throw AssemblerException("Invalid Jump Symbol Instruction, check usage");
  }
  current_section_size += 4;
}
void Assembler::handleLoadStoreDirLiteral(Instruction::OCMOD ocmod, uint8_t gpr, int32_t value)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {
  case Instruction::LD_REG_DISP:
    if (isValue12b(value))
    {
      binaryIO.writeInstructionBinary({ocmod, gpr, 0, 0, value});
    }
    else
    {
      literalPool.put(value, current_section_size);
      binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gpr, Instruction::PC, 0, 0});
    }
    break;
  case Instruction::ST_DIR:
    if (isValue12b(value))
    {
      binaryIO.writeInstructionBinary({ocmod, 0, 0, gpr, value});
    }
    else
    {
      literalPool.put(value, current_section_size);
      binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_IND, Instruction::PC, 0, gpr, 0});
    }
    break;
  default:
    throw AssemblerException("Invalid Load/Store DIR Literal Instruction, check usage");
  }

  current_section_size += 4;
}

void Assembler::handleLoadStoreDirSymbol(Instruction::OCMOD ocmod, uint8_t gpr, std::string *name)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }

  SymbolTable::Symbol *sym = symTable.getSymbol(*name);
  if (!sym) // symbol not in symbol table, add it
  {
    sym = symTable.addEntry(name, 0, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
  }
  int32_t disp = (int32_t)sym->offset - (int32_t)(current_section_size + 4);

  switch (ocmod)
  {
  case Instruction::OCMOD::LD_REG_DISP:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || !isValue12b(disp) || sym->index != current_section->index) // symbol not defined or displacement is too big
    {
      // make a literal pool entry
      literalPool.put(0, current_section_size, name); // this will change in literal backpatch
      binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gpr, Instruction::PC, 0, 0});
    }
    else // symbol is defined and displacement is in 12 bits
    {
      binaryIO.writeInstructionBinary({ocmod, gpr, Instruction::PC, 0, disp});
    }
    break;
  case Instruction::OCMOD::ST_DIR:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || !isValue12b(disp) || sym->index != current_section->index) // symbol not defined or displacement is too big
    {
      // make a literal pool entry
      literalPool.put(0, current_section_size, name); // this will change in literal backpatch
      binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_IND, Instruction::PC, 0, gpr, 0});
    }
    else // symbol is defined and displacement is in 12 bits
    {
      binaryIO.writeInstructionBinary({ocmod, Instruction::PC, 0, gpr, disp});
    }
    break;
  default:
    throw AssemblerException("Invalid Load/Store DIR Symbol Instruction, check usage");
  }
  current_section_size += 4;
}

void Assembler::handleLoadStoreIndLiteral(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, uint8_t gpr3, int32_t value)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::ST_IND:
    if (isValue12b(value))
    {
      binaryIO.writeInstructionBinary({Instruction::ST_DIR, gpr1, gpr2, gpr3, value});
      current_section_size += 4;
    }
    else
    {
      // first load the literal from the literal pool, then load the value
      binaryIO.writeInstructionBinary({Instruction::LD_MEM, gpr1, Instruction::PC, 0, 0});
      literalPool.put(value, current_section_size);
      binaryIO.writeInstructionBinary({ocmod, gpr1, gpr2, gpr3, value}); // this will get backpatched
      current_section_size += 8;
    }
    break;
  case Instruction::OCMOD::LD_MEM:
    if (isValue12b(value))
    {
      binaryIO.writeInstructionBinary({ocmod, gpr1, gpr2, gpr3, value});
      current_section_size += 4;
    }
    else
    {
      // first load the literal from the literal pool, then load the value
      binaryIO.writeInstructionBinary({Instruction::LD_MEM, gpr1, Instruction::PC, 0, 0});
      literalPool.put(value, current_section_size);
      binaryIO.writeInstructionBinary({ocmod, gpr1, gpr2, gpr3, value}); // this will get backpatched
      current_section_size += 8;
    }
    break;
  default:
    throw AssemblerException("Invalid Load/Store IND Literal Instruction, check usage");
  }
}

void Assembler::handleLoadStoreIndSymbol(Instruction::OCMOD ocmod, uint8_t gpr1, uint8_t gpr2, uint8_t gpr3, std::string *name)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }

  SymbolTable::Symbol *sym = symTable.getSymbol(*name);
  if (!sym) // symbol not in symbol table, add it
  {
    sym = symTable.addEntry(name, 0, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
  }
  int32_t disp = (int32_t)sym->offset - (int32_t)(current_section_size + 4);
  switch (ocmod)
  {
  case Instruction::OCMOD::ST_IND:
  case Instruction::OCMOD::LD_MEM:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || !isValue12b(disp) || sym->index != current_section->index) // symbol not defined or displacement is too big
    {
      // make a literal pool entry holding the symbol's resolved absolute address
      literalPool.put(0, current_section_size, name); // this will change in literal backpatch
      switch (ocmod)
      {
      case Instruction::OCMOD::LD_MEM:
        binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gpr1, Instruction::PC, 0, 0});
        binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gpr1, gpr1, 0, 0});
        current_section_size += 8;
        break;
      case Instruction::OCMOD::ST_IND:
        binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_IND, Instruction::PC, 0, gpr3, 0});
        current_section_size += 4;
        break;
      default:
        break;
      }
    }
    else // symbol is defined and displacement is in 12 bits: no pool needed, direct PC-relative access
    {
      switch (ocmod)
      {
      case Instruction::OCMOD::LD_MEM:
        binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gpr1, Instruction::PC, 0, disp});
        break;
      case Instruction::OCMOD::ST_IND:
        binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_DIR, Instruction::PC, 0, gpr3, disp});
        break;
      default:
        break;
      }
      current_section_size += 4;
    }
    break;
  default:
    throw AssemblerException("Invalid Load/Store IND Symbol Instruction, check usage");
  }
}

void Assembler::handleLoadStoreRegInstruction(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::LD_MEM:
  case Instruction::OCMOD::LD_REG_DISP:
    binaryIO.writeInstructionBinary({ocmod, gprD, gprS});
    break;
  case Instruction::OCMOD::ST_IND:
    binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_DIR, gprD, 0, gprS});
    break;
  case Instruction::OCMOD::ST_DIR: // stores the value of one register to another, basically a load
    binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_REG_DISP, gprS, 0, gprD});
    break;
  default:
    throw AssemblerException("Invalid Load Store Reg Instruction, check usage");
  }
  current_section_size += 4;
}

void Assembler::handleLoadStoreRegLiteral(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS, int32_t value)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  if (!isValue12b(value))
  {
    throw AssemblerException("Displacement is too big");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::LD_MEM:
    binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gprD, gprS, 0, value});
    break;
  case Instruction::OCMOD::ST_IND:
    binaryIO.writeInstructionBinary({Instruction::OCMOD::ST_DIR, gprD, 0, gprS, value});
    break;
  default:
    throw AssemblerException("Invalid Load/Store reg Literal Instruction, check usage");
  }
  current_section_size += 4;
}

void Assembler::handleLoadStoreRegSymbol(Instruction::OCMOD ocmod, uint8_t gprD, uint8_t gprS, std::string *name)
{
  if (current_section->num == 0)
  {
    throw AssemblerException("Section not opened");
  }
  SymbolTable::Symbol *sym = symTable.getSymbol(*name);
  if (!sym) // symbol not in symbol table, add it
  {
    sym = symTable.addEntry(name, 0, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::UNDEFINED, 0);
  }
  int32_t disp = sym->offset - (current_section_size + 4);
  if (!isValue12b(disp)) // displacement is too big
  {
    throw AssemblerException("Displacement is too big");
  }
  switch (ocmod)
  {
  case Instruction::OCMOD::LD_REG_DISP:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || sym->index != current_section->index)
    {
      binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gprD, Instruction::PC, 0, 0});
      // make a literal pool entry
      literalPool.put(0, current_section_size, name);             // this will change in literal backpatch
      binaryIO.writeInstructionBinary({ocmod, gprD, 0, gprS, 0}); // this will get backpatched

      current_section_size += 8;
    }
    else
    {
      binaryIO.writeInstructionBinary({ocmod, gprD, gprS, 0, disp});
      current_section_size += 4;
    }

    break;
  case Instruction::OCMOD::ST_DIR:
    if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED || sym->index != current_section->index)
    {
      binaryIO.writeInstructionBinary({Instruction::OCMOD::LD_MEM, gprS, Instruction::PC, 0, 0});
      // make a literal pool entry
      literalPool.put(0, current_section_size, name);             // this will change in literal backpatch
      binaryIO.writeInstructionBinary({ocmod, gprD, 0, gprS, 0}); // this will get backpatched

      current_section_size += 8;
    }
    else
    {
      binaryIO.writeInstructionBinary({ocmod, gprD, 0, gprS, disp});
      current_section_size += 4;
    }

    break;
  default:
    throw AssemblerException("Invalid Load Store Reg Symbol Instruction, check usage");
  }
}
void Assembler::handleLiteralBackpatch()
{

  if (literalPool.isEmpty())
    return;

  std::vector<std::pair<uint32_t, uint32_t>> literals_in_pool; // keeps track of literals already in pool to prevent duplcate entries
  uint32_t sectionStart = symTable.getSectionStart(current_section);
  // check how big literal pool is, put instructions accordingly
  bool canUseJMP_DIR = (literalPool.size() * 4 <= 0x7ff);
  if (!canUseJMP_DIR) // literal pool is so big, jmp_dir wont do, use jmp_ind instead
  {
    handleJumpLiteralInstruction(Instruction::JMP_IND, Instruction::PC, 0, 0);
    handleWordLiteral(0); // this will be changed to the end address of the literal pool
  }
  else
    handleJumpLiteralInstruction(Instruction::JMP_DIR, Instruction::PC, 0, 0); // displacement will change

  bool symbolDefinedInCurSection;
  bool literalInPool;
  uint32_t poolStart = current_section_size; // starting address of the literal pool

  while (!literalPool.isEmpty())
  {

    symbolDefinedInCurSection = false;
    literalInPool = false;
    LiteralPool::LiteralPoolEntry entry = literalPool.popFirst();

    int32_t disp = (int32_t)(current_section_size) - (int32_t)(entry.offset + 4);
    if (entry.symbol != "") // check if its a literal or a symbol
    {                       // its a symbol
      SymbolTable::Symbol *sym = symTable.getSymbol(entry.symbol);
      handleWordLiteral(0);
      backpatchList.push_back({current_section_size - 4, sym, current_section});
    }
    else // case for literals
    {
      for (auto literal_in_pool : literals_in_pool)
      {
        if (literal_in_pool.second == entry.value) // prevents duplicate entries
        {
          disp = literal_in_pool.first - (entry.offset + 4);
          literalInPool = true;
          break;
        }
      }
      literals_in_pool.push_back({current_section_size, entry.value});
      if (!literalInPool)
      {
        handleWordLiteral(entry.value);
      }
    }

    // write the literal in the pool
    if (!symbolDefinedInCurSection && !literalInPool)
    {
      handleWordLiteral(entry.value);
    }
    // change the displacement of the instruction
    binaryIO.changeDisplacement(entry.offset + sectionStart, disp);
  }
  // fix the jump that skips over literal pool
  if (!canUseJMP_DIR)
  {
    // this is the offset to the end of the literal pool of the section,
    // since its relative to the start of the file
    // relocation is needed
    binaryIO.changeWord(poolStart + sectionStart - 4, current_section_size + sectionStart);
    // add the start of the section with its size to get to the end of the literal pool
    relTable.AddRelocation({poolStart - 4, current_section, current_section, RelocationTable::REL_BIND::REL_LOCAL, current_section_size});
  }
  else
  {
    binaryIO.changeDisplacement(poolStart + sectionStart - 4, current_section_size - poolStart);
  }
}

void Assembler::handleSymbolBackpatch()
{
  while (!backpatchList.empty())
  {
    Backpatch bp = backpatchList.front();
    backpatchList.pop_front();

    // check if its extern
    if (bp.symbol->bind == SymbolTable::SYM_BIND::SYMBOL_GLOBAL && bp.symbol->type == SymbolTable::SYM_TYPE::UNDEFINED)
    {
      // its extern, relocation is needed
      relTable.AddRelocation({bp.offset, bp.symbol, bp.section, RelocationTable::REL_BIND::REL_GLOBAL, 0});
      continue;
    }
    if (bp.symbol->type == SymbolTable::SYM_TYPE::UNDEFINED)
    {
      throw AssemblerException("Local symbol left undefined");
    }
    // the symbol is defined
    // relocation is needed
    relTable.AddRelocation({bp.offset, (bp.symbol->bind == SymbolTable::SYM_BIND::SYMBOL_LOCAL) ? symTable.getSection(bp.symbol->index) : bp.symbol, bp.section,
                            (bp.symbol->bind == SymbolTable::SYM_BIND::SYMBOL_GLOBAL) ? RelocationTable::REL_GLOBAL : RelocationTable::REL_LOCAL,
                            (bp.symbol->bind == SymbolTable::SYM_BIND::SYMBOL_GLOBAL) ? 0 : bp.symbol->offset});

    // backpatch
    uint32_t patch_location = bp.offset + symTable.getSectionStart(bp.section);
    uint32_t value = bp.symbol->offset;
    binaryIO.changeWord(patch_location, value);
  }
}