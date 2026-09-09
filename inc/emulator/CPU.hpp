#include <cstdint>
#ifndef CPU_HPP
#define CPU_HPP

#define GPR_COUNT 16
#define CSR_COUNT 3
#define WORD_SIZE 4

enum GPR
{
  R0 = 0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  SP,
  PC
};
enum CSR
{
  STATUS = 0,
  HANDLER,
  CAUSE
};

enum InstructionParts
{
  OC = 0,
  MOD,
  A,
  B,
  C,
  Disp1,
  Disp2,
  Disp3
};

enum Interrupt
{
  EXCEPTION,
  TERMINAL = 0x3,
  SOFTWARE = 0x4
};

class CPU
{
  friend class Terminal;

public:
  static void fetch();
  static void decode();
  static void execute();
  static void intr();
  static bool isRunning();
  static void reset();
  static void printRegisters();

  static inline uint32_t readGPR(GPR gpr)
  {
    return gprFile[gpr];
  }
  static inline void writeGPR(GPR gpr, uint32_t value)
  {
    if (gpr == 0)
      return;
    gprFile[gpr] = value;
  }
  static inline uint32_t readCSR(CSR csr)
  {
    return csrFile[csr];
  }
  static inline void writeCSR(CSR csr, uint32_t value)
  {
    csrFile[csr] = value;
  }

  static inline void updatePC()
  {
    gprFile[PC] += WORD_SIZE;
  }

private:
  static void handleHalt();
  static void handleINT();
  static void handleCall();
  static void handleJump();
  static void handleXCHG();
  static void handleArithm();
  static void handleLogic();
  static void handleShift();
  static void handleStore();
  static void handleLoad();
  static void handleRET();

  static void push(uint32_t value);
  static uint32_t pop();
  static int32_t getDisp();

  static uint32_t gprFile[GPR_COUNT];
  static uint32_t csrFile[CSR_COUNT];
  static uint32_t current_instruction;
  static uint8_t decoded_instruction[8];
  static bool interrupts[5];

  static bool running;
};

#endif