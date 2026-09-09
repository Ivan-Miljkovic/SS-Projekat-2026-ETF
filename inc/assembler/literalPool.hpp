#ifndef LITERAL_POOL_H
#define LITERAL_POOL_H

#include <vector>
#include <cstdint>
#include <string>
#include <list>
class LiteralPool
{
public:
  typedef struct LiteralPoolEntry
  {
    std::string symbol;
    uint32_t value;
    uint32_t offset;

    LiteralPoolEntry(uint32_t value, uint32_t offset, std::string symbol)
        : value(value), offset(offset), symbol(symbol) {}

  } LiteralPoolEntry;
  std::list<LiteralPoolEntry> literalPool;

  void put(uint32_t value, uint32_t offset, std::string *symbol = nullptr);
  LiteralPoolEntry popFirst();
  bool isEmpty();
  void clear();
  uint32_t size();
};

#endif