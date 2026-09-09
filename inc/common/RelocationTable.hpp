#ifndef RELOCATIONTABLE_HPP
#define RELOCATIONTABLE_HP

#include "SymbolTable.hpp"
#include <cstdint>

class RelocationTable
{
public:
  typedef enum
  {
    REL_GLOBAL,
    REL_LOCAL
  } REL_BIND;

  typedef struct Entry
  {
    uint32_t offset;
    REL_BIND bind;
    SymbolTable::Symbol *symbol;
    SymbolTable::Symbol *section;
    uint32_t addend;

    Entry(uint32_t offset, SymbolTable::Symbol *symbol, SymbolTable::Symbol *section, REL_BIND bind = REL_GLOBAL, uint32_t addend = 0)
        : offset(offset), symbol(symbol), section(section), bind(bind), addend(addend) {}
    Entry() {}

  } Entry;

  void AddRelocation(Entry entry);
  std::vector<Entry> GetAllRelocations();
  void printTable(std::ostream &os, SymbolTable *symtab);
  void printTableBinary(std::ostream &os, SymbolTable *symtab);
  void loadFromFile(std::istream &is, SymbolTable *symtab);
  void clear();

private:
  std::vector<Entry> table;
};

#endif