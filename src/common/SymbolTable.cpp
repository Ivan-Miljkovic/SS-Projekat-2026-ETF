#include "../../inc/common/SymbolTable.hpp"
#include <iostream>
#include <iomanip>
#include <cstdint>

SymbolTable::SymbolTable()
{
  ndx = 0;
  Symbol *sym = new Symbol();
  sym->name = "UND";
  sym->offset = 0;
  sym->index = 0;
  sym->bind = SYM_BIND::SYMBOL_LOCAL;
  sym->type = SYM_TYPE::SECTION;
  sym->size = 0;
  num = 0;
  table.push_back(sym);
}

void SymbolTable::printTable()
{
  std::cout << "|  Bind  "
            << "|  Type    "
            << "|  Offset  "
            << "|  Size  "
            << "|  Ndx  "
            << "|  Name  |" << std::endl;

  for (int i = 0; i < table.size(); i++)
  {
    Symbol *sym = table.at(i);

    std::cout << std::left << std::setw(8)
              << std::setfill(' ');
    ;
    if (sym->bind == SymbolTable::SYM_BIND::SYMBOL_LOCAL)
      std::cout << "LOCAL"
                << " ";
    else
      std::cout << "GLOBAL"
                << " ";
    std::cout << std::left << std::setw(12) << std::setfill(' ');
    if (sym->type == SymbolTable::SYM_TYPE::SECTION)
      std::cout << "SECTION"
                << " ";
    else if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED)
    {
      std::cout << "UNDEFINED"
                << " ";
    }
    else
      std::cout << "NOTYPE"
                << " ";

    std::cout << " 0x" << std::right << std::setw(8) << std::setfill('0') << std::hex << sym->offset
              << std::dec << " " << std::left << std::setw(8) << std::setfill(' ')
              << sym->size << " " << std::left << std::setw(7) << std::setfill(' ');

    if (sym->index == 0)
      std::cout << "UND";
    else
      std::cout << sym->index;

    std::cout << " " << std::left << std::setw(8) << std::setfill(' ') << sym->name << std::endl;
  }
}

SymbolTable::Symbol *SymbolTable::addEntry(std::string *name, uint32_t offset, SYM_BIND bind, SYM_TYPE type, uint32_t size)
{
  Symbol *sym = new Symbol();
  sym->name = *name;
  sym->offset = offset;
  if (type == SYM_TYPE::SECTION)
  {
    ndx++;
  }
  if (type == SYM_TYPE::UNDEFINED)
  {
    sym->index = 0;
  }
  else
  {
    sym->index = ndx;
  }

  sym->bind = bind;
  sym->type = type;
  sym->size = size;
  sym->num = num;
  num++;
  table.push_back(sym);
  return sym;
}
SymbolTable::Symbol *SymbolTable::addExistingEntry(Symbol symb, uint32_t index)
{
  Symbol *sym = new Symbol();
  sym->name = symb.name;
  sym->offset = symb.offset;
  sym->index = index;

  sym->bind = symb.bind;
  sym->type = symb.type;
  sym->size = symb.size;
  sym->num = num;
  num++;
  table.push_back(sym);
  return sym;
}

int SymbolTable::size()
{
  return table.size();
}

SymbolTable::Symbol *SymbolTable::getSymbol(std::string name)
{
  for (int i = 0; i < table.size(); i++)
  {
    if (table.at(i)->name == name)
      return table.at(i);
  }
  return nullptr;
}
SymbolTable::Symbol *SymbolTable::getSymbol(uint8_t num)
{
  for (int i = 0; i < table.size(); i++)
  {
    if (table.at(i)->num == num)
      return table.at(i);
  }
  return nullptr;
}
std::vector<SymbolTable::Symbol *> SymbolTable::getSymbols()
{
  return table;
}
uint32_t SymbolTable::getSectionStart(SymbolTable::Symbol *section)
{
  uint32_t addr = 0;
  for (int i = 0; i < table.size(); i++)
  {
    if (table[i]->type == SymbolTable::SYM_TYPE::SECTION)
    {
      if (table[i]->index == section->index)
        return addr;
      addr += table[i]->size;
    }
  }
  return addr;
}
bool SymbolTable::isSymbolDefined(Symbol *sym)
{
  if (sym->index == 0)
    return false;
  return true;
}

SymbolTable::Symbol *SymbolTable::getSection(int index)
{
  for (Symbol *s : table)
  {
    if (s->type == SymbolTable::SYM_TYPE::SECTION && s->index == index)
      return s;
  }
  return nullptr;
}

std::vector<SymbolTable::Symbol *> SymbolTable::getSections()
{
  std::vector<SymbolTable::Symbol *> sections;
  for (Symbol *s : table)
  {
    if (s->type == SymbolTable::SYM_TYPE::SECTION)
      sections.push_back(s);
  }
  return sections;
}

void SymbolTable::printTableBinary(std::ostream &os)
{
  // number of symbols
  uint32_t count = table.size();
  os.write(reinterpret_cast<const char *>(&count), sizeof(count));

  for (Symbol *sym : table)
  {
    // name
    uint32_t len = sym->name.size();
    os.write(reinterpret_cast<const char *>(&len), sizeof(len));
    os.write(sym->name.c_str(), len);

    // offset
    os.write(reinterpret_cast<const char *>(&sym->offset), sizeof(sym->offset));

    // index
    os.write(reinterpret_cast<const char *>(&sym->index), sizeof(sym->index));

    // bind
    uint32_t bind = static_cast<uint32_t>(sym->bind);
    os.write(reinterpret_cast<const char *>(&bind), sizeof(bind));

    // type
    uint32_t type = static_cast<uint32_t>(sym->type);
    os.write(reinterpret_cast<const char *>(&type), sizeof(type));

    // size
    os.write(reinterpret_cast<const char *>(&sym->size), sizeof(sym->size));

    // num
    os.write(reinterpret_cast<const char *>(&sym->num), sizeof(sym->num));
  }
}

void SymbolTable::loadFromFile(std::istream &is)
{
  uint32_t count;
  is.read(reinterpret_cast<char *>(&count), sizeof(count));

  for (uint32_t i = 0; i < count; i++)
  {
    Symbol *sym = new Symbol();

    uint32_t len;
    is.read(reinterpret_cast<char *>(&len), sizeof(len));

    sym->name.resize(len);
    is.read(&sym->name[0], len);

    is.read(reinterpret_cast<char *>(&sym->offset), sizeof(sym->offset));
    is.read(reinterpret_cast<char *>(&sym->index), sizeof(sym->index));

    uint32_t bind;
    is.read(reinterpret_cast<char *>(&bind), sizeof(bind));
    sym->bind = static_cast<SYM_BIND>(bind);

    uint32_t type;
    is.read(reinterpret_cast<char *>(&type), sizeof(type));
    sym->type = static_cast<SYM_TYPE>(type);

    is.read(reinterpret_cast<char *>(&sym->size), sizeof(sym->size));
    is.read(reinterpret_cast<char *>(&sym->num), sizeof(sym->num));

    table.push_back(sym);

    if (sym->type == SYM_TYPE::SECTION && sym->index > ndx)
      ndx = sym->index;

    if (sym->num >= num)
      num = sym->num + 1;
  }
}

void SymbolTable::clear()
{
  table.clear();
  ndx = 0;
  Symbol *sym = new Symbol();
  sym->name = "UND";
  sym->offset = 0;
  sym->index = 0;
  sym->bind = SYM_BIND::SYMBOL_LOCAL;
  sym->type = SYM_TYPE::SECTION;
  sym->size = 0;
  num = 1;
  table.push_back(sym);
}
