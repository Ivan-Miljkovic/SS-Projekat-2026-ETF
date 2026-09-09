#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <thread>
#include <termios.h>
#include <unistd.h>
#include <iostream>

#define TERM_IN_ADDR 0xFFFFFF04
#define TERM_OUT_ADDR 0xFFFFFF00

class Terminal
{
public:
  static void start();
  static void stop();

private:
  static void run();
  static std::thread terminalThread;
  static bool running;
  static struct termios oldSettings;
};

#endif