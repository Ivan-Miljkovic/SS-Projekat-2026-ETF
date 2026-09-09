#include "../../inc/assembler/literalPool.hpp"

void LiteralPool::put(uint32_t value, uint32_t offset, std::string *symbol)
{
  literalPool.push_back({value, offset, symbol ? *symbol : ""});
}
LiteralPool::LiteralPoolEntry LiteralPool::popFirst()
{
  LiteralPoolEntry entry = literalPool.front();
  literalPool.pop_front();
  return entry;
}

bool LiteralPool::isEmpty()
{
  return literalPool.empty();
}
uint32_t LiteralPool::size()
{
  return literalPool.size();
}

void LiteralPool::clear()
{
  literalPool.clear();
}