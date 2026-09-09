#include "../../inc/emulator/CPU.hpp"
#include "../../inc/emulator/Memory.hpp"
#include "../../inc/common/Instruction.hpp"
#include <iomanip>
#include <iostream>

uint32_t CPU::gprFile[16];
uint32_t CPU::csrFile[3];
uint8_t CPU::decoded_instruction[8];
uint32_t CPU::current_instruction;
bool CPU::interrupts[5];
bool CPU::running;

int32_t CPU::getDisp()
{
  int32_t disp = (int32_t)decoded_instruction[Disp3];
  disp |= ((int32_t)decoded_instruction[Disp2]) << 4;
  disp |= ((int32_t)decoded_instruction[Disp1]) << 8;

  disp &= 0xFFF;

  // if the 11th bit is 1 then the disp should be negative
  if (disp & 0x800)
    disp |= 0xFFFFF000;

  return disp;
}

bool CPU::isRunning()
{
  return running;
}

void CPU::reset()
{
  for (int i = 0; i < GPR_COUNT; i++)
    gprFile[i] = 0;
  for (int i = 0; i < CSR_COUNT; i++)
    csrFile[i] = 0;

  current_instruction = 0;
  for (int i = 0; i < 8; i++)
    decoded_instruction[i] = 0;

  gprFile[PC] = 0x40000000;
  running = true;
}

void CPU::fetch()
{
  // fetch WORD_SIZE from memory, update pc
  uint32_t currentPC = gprFile[PC];
  current_instruction = Memory::readWord(currentPC);
  gprFile[PC] = currentPC + 4;
}

void CPU::decode()
{
  for (int i = 0; i < 8; i++)
  {
    decoded_instruction[i] = (uint8_t)(current_instruction >> (28 - 4 * i) & 0xF);
  }
  current_instruction = 0;
}

void CPU::execute()
{
  switch (decoded_instruction[OC])
  {
  case Instruction::HALT_I:
    handleHalt();
    break;
  case Instruction::INT_I:
    handleINT();
    break;
  case Instruction::CALL_I:
    handleCall();
    break;
  case Instruction ::JUMP_I:
    handleJump();
    break;
  case Instruction::XCHG_I:
    handleXCHG();
    break;
  case Instruction::ARITHM_I:
    handleArithm();
    break;
  case Instruction::LOGIC_I:
    handleLogic();
    break;
  case Instruction::SHIFT_I:
    handleShift();
    break;
  case Instruction::STORE_I:
    handleStore();
    break;
  case Instruction::LOAD_I:
    handleLoad();
    break;
  case Instruction::RET_I:
    handleRET();
    break;
  }
}

void CPU::intr()
{
  if (csrFile[HANDLER] == 0)
  {
    // handler address is not valid
    return;
  }

  if (interrupts[EXCEPTION]) // non-maskable interrupt
  {
    interrupts[EXCEPTION] = false;
    push(csrFile[STATUS]);
    push(gprFile[PC]);

    csrFile[STATUS] |= 0x4; // mask external interrupts;
    writeGPR(PC, csrFile[HANDLER]);

    csrFile[CAUSE] = EXCEPTION;
    return;
  }

  if (csrFile[STATUS] & 0x4) // external interrupts are masked
    return;

  if (interrupts[TERMINAL] && !(csrFile[STATUS] & 0x2))
  {
    interrupts[TERMINAL] = false;
    push(csrFile[STATUS]);
    push(gprFile[PC]);

    csrFile[STATUS] |= 0x4;
    writeGPR(PC, csrFile[HANDLER]);

    csrFile[CAUSE] = TERMINAL;

    return;
  }
  if (interrupts[SOFTWARE])
  {
    interrupts[SOFTWARE] = false;
    push(csrFile[STATUS]);
    push(gprFile[PC]);

    csrFile[STATUS] |= 0x4;
    csrFile[CAUSE] = SOFTWARE;
    writeGPR(PC, csrFile[HANDLER]);
  }
}

void CPU::push(uint32_t value)
{
  gprFile[SP] -= 4;
  Memory::writeWord(gprFile[SP], value);
}

uint32_t CPU::pop()
{
  uint32_t value = Memory::readWord(gprFile[SP]);
  gprFile[SP] += 4;
  return value;
}

void CPU::handleHalt()
{
  running = false;
}

void CPU::handleINT()
{
  interrupts[SOFTWARE] = true;
}

void CPU::handleCall()
{

  uint32_t regA = readGPR((GPR)decoded_instruction[A]);
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  int32_t disp = getDisp();
  uint32_t value = regA + regB + disp;

  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::CALL_DIR:
    push(gprFile[PC]);
    gprFile[PC] = value;
    break;
  case Instruction::CALL_IND:
    push(gprFile[PC]);
    gprFile[PC] = Memory::readWord(value);
    break;
  default:
    interrupts[EXCEPTION] = true;
    break;
  }
}

void CPU::handleJump()
{
  uint32_t regA = readGPR((GPR)decoded_instruction[A]);
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  int32_t disp = getDisp();

  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::JMP_DIR:
    gprFile[PC] = regA + disp;
    break;
  case Instruction::BEQ_DIR:
    if (regB == regC)
      gprFile[PC] = regA + disp;
    break;
  case Instruction::BNE_DIR:
    if (regB != regC)
      gprFile[PC] = regA + disp;
    break;
  case Instruction::BGT_DIR:
    if ((int32_t)regB > (int32_t)regC)
      gprFile[PC] = regA + disp;
    break;
  case Instruction::JMP_IND:
    gprFile[PC] = Memory::readWord(regA + disp);
    break;
  case Instruction::BEQ_IND:
    if (regB == regC)
      gprFile[PC] = Memory::readWord(regA + disp);
    break;
  case Instruction::BNE_IND:
    if (regB != regC)
      gprFile[PC] = Memory::readWord(regA + disp);
    break;
  case Instruction::BGT_IND:
    if ((int32_t)regB > (int32_t)regC)
      gprFile[PC] = Memory::readWord(regA + disp);
    break;
  default:
    interrupts[EXCEPTION] = true;
    break;
  }
}

void CPU::handleXCHG()
{
  uint32_t temp = readGPR((GPR)decoded_instruction[B]);
  writeGPR((GPR)decoded_instruction[B], readGPR((GPR)decoded_instruction[C]));
  writeGPR((GPR)decoded_instruction[C], temp);
}

void CPU::handleArithm()
{
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::ADD:
    writeGPR((GPR)decoded_instruction[A], regB + regC);
    break;
  case Instruction::SUB:
    writeGPR((GPR)decoded_instruction[A], regB - regC);
    break;
  case Instruction::MUL:
    writeGPR((GPR)decoded_instruction[A], regB * regC);
    break;
  case Instruction::DIV:
    if (regC == 0)
      writeGPR((GPR)decoded_instruction[A], UINT32_MAX);
    else
      writeGPR((GPR)decoded_instruction[A], regB / regC);
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::handleLogic()
{
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::NOT:
    writeGPR((GPR)decoded_instruction[A], ~regB);
    break;
  case Instruction::AND:
    writeGPR((GPR)decoded_instruction[A], regB & regC);
    break;
  case Instruction::OR:
    writeGPR((GPR)decoded_instruction[A], regB | regC);
    break;
  case Instruction::XOR:
    writeGPR((GPR)decoded_instruction[A], regB ^ regC);
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::handleShift()
{
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::SHL:
    writeGPR((GPR)decoded_instruction[A], regB << regC);
    break;
  case Instruction::SHR:
    writeGPR((GPR)decoded_instruction[A], regB >> regC);
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::handleStore()
{
  uint32_t regA = readGPR((GPR)decoded_instruction[A]);
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  int32_t disp = getDisp();
  uint32_t addr;

  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::ST_DIR:
    addr = regA + regB + disp;
    Memory::writeWord(addr, regC);
    break;
  case Instruction::ST_IND:
    addr = Memory::readWord(regA + regB + disp);
    Memory::writeWord(addr, regC);
    break;
  case Instruction::ST_PREINC:
    regA += disp;
    writeGPR((GPR)decoded_instruction[A], regA);
    Memory::writeWord(regA, regC);
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::handleLoad()
{
  uint32_t regA = readGPR((GPR)decoded_instruction[A]);
  uint32_t regB = readGPR((GPR)decoded_instruction[B]);
  uint32_t regC = readGPR((GPR)decoded_instruction[C]);
  int32_t disp = getDisp();
  uint32_t addr;
  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::CSRRD:
    writeGPR((GPR)decoded_instruction[A], csrFile[(GPR)decoded_instruction[B]]);
    break;
  case Instruction::LD_REG_DISP:
    writeGPR((GPR)decoded_instruction[A], regB + disp);
    break;
  case Instruction::LD_MEM:
    addr = regB + regC + disp;
    writeGPR((GPR)decoded_instruction[A], Memory::readWord(addr));
    break;
  case Instruction::LD_POSTINC:
    writeGPR((GPR)decoded_instruction[A], Memory::readWord(regB));
    writeGPR((GPR)decoded_instruction[B], regB + disp);
    break;
  case Instruction::CSRWR:
    csrFile[(GPR)decoded_instruction[A]] = regB;
    break;
  case Instruction::CSR_OR:
    csrFile[(GPR)decoded_instruction[A]] = csrFile[(GPR)decoded_instruction[B]] | disp;
    break;
  case Instruction::CSR_MEM:
    addr = regB + regC + disp;
    csrFile[(CSR)decoded_instruction[A]] = Memory::readWord(addr);
    break;
  case Instruction::CSR_POSTINC:
    csrFile[(GPR)decoded_instruction[A]] = Memory::readWord(regB);
    writeGPR((GPR)decoded_instruction[B], regB + disp);
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::handleRET()
{
  switch ((decoded_instruction[OC] << 4) | decoded_instruction[MOD])
  {
  case Instruction::RET:
    gprFile[PC] = pop();
    break;
  case Instruction::IRET:
    gprFile[PC] = pop();
    csrFile[STATUS] = pop();
    break;
  default:
    interrupts[EXCEPTION] = true;
  }
}

void CPU::printRegisters()
{
  std::cout << "\n\n=================EMULATOR===================\n";
  std::cout << "-----------------------------------------------------------------" << std::endl;
  std::cout << "Emulated processor executed halt instruction" << std::endl;
  std::cout << "Emulated processor state:" << std::endl;
  for (int i = 0; i < 16; i++)
  {
    std::cout << "r" << std::dec << i << "=0x"
              << std::hex << std::setw(8) << std::setfill('0')
              << CPU::gprFile[i];
    if (i % 4 == 3)
      std::cout << "\n";
    else
      std::cout << " ";
  }
}