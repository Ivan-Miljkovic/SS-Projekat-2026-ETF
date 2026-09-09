#include "../../inc/linker/Linker.hpp"
#include "../../inc/linker/LinkerException.hpp"
#include <fstream>
#include <utility>
#include <iomanip>

std::vector<Linker::AgregateSection> Linker::sections;
SymbolTable Linker::globalSymTable;
RelocationTable Linker::globalRelTable;
BinaryIO Linker::globalBinaryOut;
std::map<uint32_t, uint8_t> Linker::hexMemory;
std::vector<Linker::PlaceFiles> Linker::placeFiles;
std::list<Linker::ObjectFile> Linker::files;
bool Linker::hex;
std::string Linker::outputFile;
std::vector<std::string> sectionOrder;

void Linker::Start(int argc, char *argv[])
{
  globalBinaryOut.clear();
  globalSymTable.clear();
  globalRelTable.clear();
  hex = false;
  ParseArguments(argc, argv);
  MapSections();
  PlaceSections();

  MergeSymTable();
  if (outputFile.empty())
  {
    outputFile = "out.hex";
  }
  std::ofstream outputBinary(std::string(outputFile), std::ios::binary);
  if (!outputBinary.is_open())
  {
    throw LinkerException("can't open output file");
  }
  std::cout << "===============LINKER=======================\n";
  if (hex)
  {
    MergeHexMemory();
    ResolveRelocations();

    PrintHexMemory(std::cout);
    PrintHexMemory(outputBinary);

    outputBinary.close();
    return;
  }
  MergeSections();
  MergeRelTable();

  globalBinaryOut.printOutput(std::cout);
  globalSymTable.printTable();
  globalRelTable.printTable(std::cout, &globalSymTable);
  globalBinaryOut.printOutputBinary(outputBinary);
  globalSymTable.printTableBinary(outputBinary);
  globalRelTable.printTableBinary(outputBinary, &globalSymTable);
  outputBinary.close();
}

void Linker::ParseArguments(int argc, char *argv[])
{
  std::vector<std::string> fileNames;
  bool optFound = false;

  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg.rfind("-place=", 0) == 0)
    {
      std::string place = arg.substr(7); // starts from the 7th char
      size_t pos = place.find('@');

      std::string sectionName = place.substr(0, pos);
      std::string addressStr = place.substr(pos + 1);

      uint32_t address = std::stoul(addressStr, nullptr, 0);

      placeFiles.push_back({sectionName, address});
      continue;
    }
    else if (arg.rfind("-relocatable") == 0)
    {
      if (optFound)
      {
        throw LinkerException("Multiple options defined");
        return;
      }
      hex = false;
      optFound = true;
      continue;
      ;
    }
    else if (arg.rfind("-hex") == 0)
    {
      if (optFound)
      {
        throw LinkerException("Multiple options defined");
      }
      hex = true;
      optFound = true;
      continue;
    }
    else if (arg.rfind("-o") == 0)
    {
      outputFile = argv[++i];
      continue;
    }
    fileNames.push_back(argv[i]);
  }
  if (!optFound)
  {
    throw LinkerException("No options defined");
  }
  LoadFiles(fileNames);
}
void Linker::LoadFiles(std::vector<std::string> fileNames)
{
  for (int i = 0; i < fileNames.size(); i++)
  {
    ObjectFile file;

    std::ifstream inputFile(fileNames[i], std::ios::binary);

    if (!inputFile.is_open())
    {
      throw LinkerException("Can't open input file");
      return;
    }

    file.binaryOut.loadFromFile(inputFile);
    file.symTable.loadFromFile(inputFile);
    file.relTable.loadFromFile(inputFile, &file.symTable);

    files.push_back(std::move(file));
  }
}
Linker::AgregateSection *Linker::FindAgregateSection(std::string &name)
{
  for (Linker::AgregateSection &s : sections)
  {
    if (s.name == name)
      return &s;
  }
  return nullptr;
}
void Linker::MapSections()
{
  sections.clear();
  for (ObjectFile &obj : files)
  {
    for (SymbolTable::Symbol *section : obj.symTable.getSections())
    {
      if (section->name == "UND")
        continue;
      AgregateSection *as = FindAgregateSection(section->name);

      if (!as)
      {
        sections.push_back({section->name, {}});
        as = &sections.back();
      }
      as->infos.push_back({&obj, section, UINT32_MAX});
    }
  }
}

void Linker::PlaceSections()
{
  uint32_t current = 0;
  uint32_t currentNdx = 1;
  if (hex)
  {
    for (auto &place : placeFiles)
    {
      AgregateSection *as = FindAgregateSection(place.name);
      if (!as)
        continue;

      uint32_t placeCurrent = place.start;

      for (SectionInfo &info : as->infos)
      {
        info.start = placeCurrent;
        info.ndx = currentNdx;
        placeCurrent += info.section->size;
      }
      currentNdx++;
      if (placeCurrent > current)
        current = placeCurrent;
    }
  }

  for (AgregateSection &as : sections)
  {
    if (as.infos.front().start != UINT32_MAX)
      continue; // section was already placed with -place

    for (SectionInfo &info : as.infos)
    {
      info.start = current;
      info.ndx = currentNdx;
      current += info.section->size;
    }
    currentNdx++;
  }
}

Linker::SectionInfo *Linker::FindSection(ObjectFile *objFile, uint32_t index)
{
  for (AgregateSection &as : sections)
  {
    for (SectionInfo &section : as.infos)
    {
      if (section.objFile == objFile && section.section->index == index)
        return &section;
    }
  }
  return nullptr;
}

void Linker::MergeSections()
{
  uint32_t totalSize = 0;

  for (AgregateSection &as : sections)
  {

    for (SectionInfo &info : as.infos)
    {

      uint32_t end = info.start + info.section->size;

      if (end > totalSize)
        totalSize = end;
    }
  }
  globalBinaryOut.resize(totalSize);
  for (AgregateSection &as : sections)
  {
    for (SectionInfo &info : as.infos)
    {
      uint32_t src = info.objFile->symTable.getSectionStart(info.section);
      uint32_t dst = info.start;
      // copies the bytes to the right destination
      for (uint32_t i = 0; i < info.section->size; i++)
      {
        globalBinaryOut.changeByte(dst + i, info.objFile->binaryOut.readByte(src + i));
      }
    }
  }
}
void Linker::MergeHexMemory()
{
  hexMemory.clear();
  uint32_t src;
  uint32_t dst;
  uint32_t address;
  for (AgregateSection &as : sections)
  {
    for (SectionInfo &info : as.infos)
    {
      src = info.objFile->symTable.getSectionStart(info.section);
      dst = info.start;

      for (uint32_t i = 0; i < info.section->size; i++)
      {
        address = dst + i;
        hexMemory[address] = info.objFile->binaryOut.readByte(src + i);
      }
    }
  }
}
void Linker::MergeSymTable()
{
  globalSymTable.clear();
  // add all the sections
  for (AgregateSection &as : sections)
  {
    if (as.name == "UND")
      continue;
    uint32_t size = 0;
    uint32_t secStart = as.infos.front().start; // start of the section
    for (SectionInfo &info : as.infos)          // gets the final size of the section
    {
      size += info.section->size;
    }
    // adds the section into the global symbol table
    std::string name = as.name;
    SymbolTable::Symbol *sec = globalSymTable.addEntry(&name, secStart, SymbolTable::SYM_BIND::SYMBOL_LOCAL, SymbolTable::SYM_TYPE::SECTION, size);

    // updates the index in the original table
    for (SectionInfo &info : as.infos)
      info.ndx = sec->index;
  }
  // add the global symbols
  for (ObjectFile &obj : files)
  {
    for (SymbolTable::Symbol *sym : obj.symTable.getSymbols())
    {
      if (sym->type == SymbolTable::SYM_TYPE::SECTION || sym->bind == SymbolTable::SYM_BIND::SYMBOL_LOCAL)
        continue;

      if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED)
        continue; // its extern, skip it

      if (globalSymTable.getSymbol(sym->name))
      {
        throw LinkerException("Multiple definitions of a symbol");
      }
      // find the section of the global symbol in the sections list
      SectionInfo *sec = FindSection(&obj, sym->index);
      // calculate the address
      uint32_t address = sec->start + sym->offset;
      // add the symbol into the global symbol table and update the index
      SymbolTable::Symbol *out = globalSymTable.addEntry(&sym->name, address, sym->bind, sym->type, sym->size);
      out->index = sec->ndx;
    }
  }
  if (!hex) // add the local and extern symbols if -relocatable
    for (ObjectFile &obj : files)
    {
      for (SymbolTable::Symbol *sym : obj.symTable.getSymbols())
      {
        if (globalSymTable.getSymbol(sym->name))
          continue;
        if (sym->type == SymbolTable::SYM_TYPE::UNDEFINED)
        {
          SymbolTable::Symbol *out = globalSymTable.addEntry(&sym->name, 0, sym->bind, sym->type, sym->size);
          out->index = 0; // UND
          continue;
        }

        // if the symbol is extern and still not defined, it needs to be added for relocations
        SectionInfo *sec = FindSection(&obj, sym->index);
        // calculate the address
        uint32_t address = sec->start + sym->offset;
        // add the symbol into the global symbol table and update the index
        SymbolTable::Symbol *out = globalSymTable.addEntry(&sym->name, address, sym->bind, sym->type, sym->size);
        out->index = sec->ndx;
      }
    }
}

void Linker::MergeRelTable()
{
  for (ObjectFile &obj : files)
  {
    for (RelocationTable::Entry &rel : obj.relTable.GetAllRelocations())
    {
      SectionInfo *sec = FindSection(&obj, rel.section->index);

      RelocationTable::Entry newEntry;
      newEntry.offset = sec->start + rel.offset;
      newEntry.bind = rel.bind;
      newEntry.addend = rel.addend;
      newEntry.section = globalSymTable.getSection(sec->ndx);
      if (rel.bind == RelocationTable::REL_GLOBAL)
      {
        newEntry.symbol = globalSymTable.getSymbol(rel.symbol->name);
      }
      else
      {
        SectionInfo *symSec = FindSection(&obj, rel.symbol->index);
        newEntry.symbol = globalSymTable.getSection(symSec->ndx);
      }

      globalRelTable.AddRelocation(newEntry);
    }
  }
}

void Linker::ResolveRelocations()
{
  uint32_t value;
  for (ObjectFile &obj : files)
  {
    for (RelocationTable::Entry &rel : obj.relTable.GetAllRelocations())
    {
      SectionInfo *sec = FindSection(&obj, rel.section->index);
      if (!sec)
      {
        std::cout << "Can't find section " << rel.section->name << std::endl;
        abort();
      }
      uint32_t address = sec->start + rel.offset;

      if (rel.bind == RelocationTable::REL_BIND::REL_GLOBAL) // global relocation
      {
        SymbolTable::Symbol *sym = globalSymTable.getSymbol(rel.symbol->name);
        if (!sym)
        {
          throw LinkerException("Undefined symbol");
        }
        value = sym->offset + rel.addend;
      }
      else // local relocation
      {
        SectionInfo *symSec = FindSection(&obj, rel.symbol->index);
        if (!symSec)
        {
          std::cout << "Can't find section " << rel.section->name << std::endl;
          abort();
        }
        value = symSec->start + rel.addend;
      }
      if (!hex)
      {
        globalBinaryOut.changeWord(address, value);
      }
      else
        for (int i = 0; i < 4; i++)
        {
          hexMemory[address + i] = (uint8_t)value;
          value >>= 8;
        }
    }
  }
}

void Linker::PrintHexMemory(std::ostream &os)
{
  if (hexMemory.empty())
    return;

  auto it = hexMemory.begin();

  while (it != hexMemory.end())
  {
    uint32_t startAddress = it->first;

    os << std::uppercase
       << std::hex
       << std::setw(8)
       << std::setfill('0')
       << startAddress
       << ": ";

    uint32_t expectedAddress = startAddress;
    int count = 0;

    while (it != hexMemory.end() &&
           it->first == expectedAddress &&
           count < 8)
    {
      os << std::setw(2)
         << std::setfill('0')
         << static_cast<unsigned int>(it->second);

      ++it;
      ++expectedAddress;
      ++count;

      if (it != hexMemory.end() &&
          it->first == expectedAddress &&
          count < 8)
      {
        os << ' ';
      }
    }

    os << '\n';
  }
}