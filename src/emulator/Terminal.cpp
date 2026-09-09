#include "../../inc/emulator/Terminal.hpp"
#include "../../inc/emulator/Memory.hpp"
#include "../../inc/emulator/CPU.hpp"
#include <fcntl.h>

termios Terminal::oldSettings;
bool Terminal::running = false;
std::thread Terminal::terminalThread;

void Terminal::start()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &oldSettings);
  t = oldSettings;
  t.c_lflag &= ~(ICANON | ECHO);
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  running = true;
  terminalThread = std::thread(&run);
}

void Terminal::stop()
{
  running = false;
  terminalThread.join();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

void Terminal::run()
{
  char c;
  while (running)
  {
    if (read(STDIN_FILENO, &c, 1) == 1)
    {
      uint32_t val = (uint32_t)(c);
      Memory::writeWord(TERM_IN_ADDR, val);

      CPU::interrupts[TERMINAL] = true;
    }
  }
}