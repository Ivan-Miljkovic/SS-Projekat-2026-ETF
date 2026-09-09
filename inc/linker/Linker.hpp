#ifndef LINKER_HPP
#define LINKER_HPP

#include <list>
#include <map>
#include <cstdint>

#include "../common/SymbolTable.hpp"
#include "../common/BinaryIO.hpp"
#include "../common/RelocationTable.hpp"

class Linker
{
public:
  typedef struct ObjectFile
  {
    SymbolTable symTable;
    RelocationTable relTable;
    BinaryIO binaryOut;
  } ObjectFile;

  typedef struct SectionInfo
  {
    ObjectFile *objFile;
    SymbolTable::Symbol *section;
    uint32_t start;

    uint32_t ndx;
  } SectionInfo;

  typedef struct AgregateSection
  {
    std::string name;
    std::vector<SectionInfo> infos;
  } AgregateSection;

  typedef struct PlaceFiles
  {
    std::string name;
    uint32_t start;
  } PlaceFiles;

  static std::vector<PlaceFiles> placeFiles;
  static std::list<ObjectFile> files;
  static std::vector<AgregateSection> sections;

  static void Start(int argc, char *argv[]);

private:
  static void ParseArguments(int argc, char *argv[]);
  static void LoadFiles(std::vector<std::string> fileNames);

  static void MapSections();
  static void PlaceSections();
  static void MergeSections();
  static void MergeHexMemory();
  static AgregateSection *FindAgregateSection(std::string &name);
  static void MergeSymTable();
  static SectionInfo *FindSection(ObjectFile *objFile, uint32_t index);

  static void MergeRelTable();

  static void ResolveRelocations();

  static void PrintHexMemory(std::ostream &os);

  static SymbolTable globalSymTable;
  static RelocationTable globalRelTable;
  static BinaryIO globalBinaryOut;
  static std::map<uint32_t, uint8_t> hexMemory;
  static bool hex;
  static std::string outputFile;
};
#endif