#include "../../inc/common/RelocationTable.hpp"
#include <iostream>
#include <iomanip>
void RelocationTable::AddRelocation(RelocationTable::Entry entry)
{
  table.push_back(entry);
}

void RelocationTable::printTable(std::ostream &os, SymbolTable *symtab)
{
  os << "| Offset |"
     << " Bind |"
     << " Symbol |"
     << " Section |"
     << " Addend |" << std::endl;
  for (size_t i = 0; i < table.size(); i++)
  {

    Entry *e = &table[i];
    os << " " << std::right << std::uppercase << std::setw(8) << std::setfill('0') << std::hex << e->offset << " "
       << std::dec << std::left << std::uppercase << std::setw(6) << std::setfill(' ');
    if (e->bind == REL_BIND::REL_GLOBAL)
      os << "GLOBAL";
    else
      os << "LOCAL";
    os << " " << std::left << std::uppercase << std::setw(8) << std::setfill(' ') << e->symbol->name << " "
       << std::uppercase << std::setw(9) << std::setfill(' ') << e->section->name << " " << e->addend << " \n";
  }
}

void RelocationTable::printTableBinary(std::ostream &os, SymbolTable *symtab)
{
  uint32_t count = table.size();
  os.write(reinterpret_cast<const char *>(&count), sizeof(count));

  for (const Entry &entry : table)
  {
    // offset
    os.write(reinterpret_cast<const char *>(&entry.offset), sizeof(entry.offset));

    // bind
    uint32_t bind = static_cast<uint32_t>(entry.bind);
    os.write(reinterpret_cast<const char *>(&bind), sizeof(bind));

    // symbol number
    uint8_t symbolNum;
    if (entry.bind == REL_BIND::REL_LOCAL)
    {
      symbolNum = symtab->getSection(entry.symbol->index)->num;
    }
    else
      symbolNum = entry.symbol->num;

    os.write(reinterpret_cast<const char *>(&symbolNum), sizeof(symbolNum));

    // section number
    uint8_t sectionNum = entry.section->num;
    os.write(reinterpret_cast<const char *>(&sectionNum), sizeof(sectionNum));

    // addend
    os.write(reinterpret_cast<const char *>(&entry.addend), sizeof(entry.addend));
  }
}

void RelocationTable::loadFromFile(std::istream &is, SymbolTable *symtab)
{
  uint32_t count;
  is.read(reinterpret_cast<char *>(&count), sizeof(count));

  for (uint32_t i = 0; i < count; i++)
  {
    Entry entry;

    // offset
    is.read(reinterpret_cast<char *>(&entry.offset), sizeof(entry.offset));

    // bind
    uint32_t bind;
    is.read(reinterpret_cast<char *>(&bind), sizeof(bind));
    entry.bind = static_cast<REL_BIND>(bind);

    // symbol number
    uint8_t symbolNum;
    is.read(reinterpret_cast<char *>(&symbolNum), sizeof(symbolNum));
    entry.symbol = symtab->getSymbol(symbolNum);

    // section number
    uint8_t sectionNum;
    is.read(reinterpret_cast<char *>(&sectionNum), sizeof(sectionNum));
    entry.section = symtab->getSymbol(sectionNum);
    // addend
    is.read(reinterpret_cast<char *>(&entry.addend), sizeof(entry.addend));

    AddRelocation(entry);
  }
}

std::vector<RelocationTable::Entry> RelocationTable::GetAllRelocations()
{
  return table;
}

void RelocationTable::clear()
{
  table.clear();
}