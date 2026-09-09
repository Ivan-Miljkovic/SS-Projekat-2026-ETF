#ifndef SYMBOLTABLE
#define SYMBOLTABLE

#include <string>
#include <vector>
#include <cstdint>

class SymbolTable
{
public:
  typedef enum
  {
    NOTYPE,
    SECTION,
    UNDEFINED
  } SYM_TYPE;
  typedef enum
  {
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL
  } SYM_BIND;

  typedef struct
  {
    std::string name;
    uint32_t offset;
    uint32_t index;
    SYM_BIND bind;
    SYM_TYPE type;
    uint32_t size;
    uint8_t num;
  } Symbol;

  SymbolTable();
  Symbol *addEntry(std::string *name, uint32_t offset, SYM_BIND bind, SYM_TYPE type, uint32_t size);
  Symbol *addExistingEntry(Symbol sym, uint32_t index);
  void printTable();
  void printTableBinary(std::ostream &os);
  void loadFromFile(std::istream &is);
  int size();
  Symbol *getSymbol(std::string name);
  Symbol *getSymbol(uint8_t num);
  std::vector<Symbol *> getSymbols();
  uint32_t getSectionStart(Symbol *section);
  Symbol *getSection(int index);
  std::vector<Symbol *> getSections();
  bool isSymbolDefined(Symbol *sym);
  void clear();

private:
  std::vector<Symbol *> table;
  int ndx;
  int num;
};

#endif