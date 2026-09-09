#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

typedef struct Instruction
{
public:
  static const uint8_t SP = 14;
  static const uint8_t PC = 15;
  static const uint8_t STATUS = 1;
  static const uint8_t HANDLER = 2;
  static const uint8_t CAUSE = 3;

  enum OpCode : uint8_t
  {
    HALT_I = 0x0,
    INT_I = 0x1,
    CALL_I = 0x2,
    JUMP_I = 0x3,
    XCHG_I = 0x4,
    ARITHM_I = 0x5,
    LOGIC_I = 0x6,
    SHIFT_I = 0x7,
    STORE_I = 0x8,
    LOAD_I = 0x9,
    RET_I = 0xF
  };

  enum OCMOD : uint8_t
  {
    HALT = 0x00,
    INT = 0x10,

    CALL_DIR = 0x20,
    CALL_IND = 0x21,

    JMP_DIR = 0x30,
    BEQ_DIR = 0x31,
    BNE_DIR = 0x32,
    BGT_DIR = 0x33,

    JMP_IND = 0x38,
    BEQ_IND = 0x39,
    BNE_IND = 0x3A,
    BGT_IND = 0x3B,

    XCHG = 0x40,

    ADD = 0x50,
    SUB = 0x51,
    MUL = 0x52,
    DIV = 0x53,

    NOT = 0x60,
    AND = 0x61,
    OR = 0x62,
    XOR = 0x63,

    SHL = 0x70,
    SHR = 0x71,

    ST_DIR = 0x80,
    ST_IND = 0x82,
    ST_PREINC = 0x81, // PUSH

    CSRRD = 0x90,
    LD_REG_DISP = 0x91,
    LD_MEM = 0x92,
    LD_POSTINC = 0x93, // POP

    CSRWR = 0x94,
    CSR_OR = 0x95,
    CSR_MEM = 0x96,
    CSR_POSTINC = 0x97,

    RET = 0xF0,
    IRET = 0xF1
  };

  Instruction(uint8_t ocmod, uint8_t ra = 0, uint8_t rb = 0, uint8_t rc = 0, int32_t disp = 0)
      : ocmod(ocmod), ra(ra), rb(rb), rc(rc), disp(disp) {}

  uint8_t ocmod; // 8b
  uint8_t ra;    // 4b
  uint8_t rb;    // 4b
  uint8_t rc;    // 4b
  int32_t disp;  // 32b
} Instruction;

#endif