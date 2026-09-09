#ifndef ASSEMBLEREXCEPTION_HPP
#define ASSEMBLEREXCEPTION_HPP
#include <exception>
#include <string>

class AssemblerException : public std::exception
{
  std::string message;

public:
  AssemblerException(const std::string &message) : message("assembler exception: " + message) {}

  const char *what() const noexcept override
  {
    return (message).c_str();
  }
};
#endif