%{
  #include <iostream>
  #include <string>
  #include <cstdint>
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
  void yyerror(const char*);
  extern int line_num;

%}

%defines "misc/parser.hpp"
%output "misc/parser.cpp"
%debug
%code requires {
  #include <cstdint>
  #include <string>
  #include "../inc/assembler/assembler.hpp"
}

%union {
  std::string *str;
  std::uint32_t num;
  int reg;
}

%token ENDL LEFT_BRACKET RIGHT_BRACKET COMMA PLUS DOLLAR COLON
%token <num> LITERAL
%token <str> SYMBOL
%token <str> ASCII_STRING

%token GLOBAL EXTERN SECTION WORD SKIP ASCII EQU END
%token <str> SECTION_NAME
%type <str> section_name_t

%token <reg> GPR
%token <reg> CSR

%token HALT
%token INT
%token IRET
%token CALL
%token RET
%token JMP
%token BEQ
%token BNE
%token BGT
%token PUSH
%token POP
%token XCHG
%token ADD
%token SUB
%token MUL
%token DIV
%token NOT
%token AND
%token OR
%token XOR
%token SHL
%token SHR
%token LD
%token ST
%token CSRRD
%token CSRWR
%%

program:
    lines;

lines:
    line |
    lines line;

line:
    notLabel ENDL |
    labels notLabel ENDL |
    labels ENDL |
    ENDL;

labels:
  label | labels label;

label:
    SYMBOL COLON
    {
      Assembler::handleLabel($1);
      delete $1;
    }

notLabel:
    directive | instruction

directive:
      GLOBAL sym_list_global { } |
      EXTERN sym_list_extern { } |
      SECTION section_name_t { Assembler::handleSection($2); delete $2; } |
      WORD sym_list_sym_lit { } |
      SKIP LITERAL { Assembler::handleSkip($2); } |
      END { Assembler::handleEnd(); YYACCEPT;} |
      ASCII ASCII_STRING { Assembler::handleAscii($2); delete $2; } |
      EQU SYMBOL COMMA LITERAL { delete $2; }
;

sym_list_global:
      SYMBOL { 
        Assembler::handleGlobal($1);
        delete $1;
      } |
      sym_list_global COMMA SYMBOL {
        Assembler::handleGlobal($3);
        delete $3;
      }
;

sym_list_extern:
      SYMBOL { 
        Assembler::handleExtern($1);
        delete $1;
      } |
      sym_list_extern COMMA SYMBOL {
        Assembler::handleExtern($3);
        delete $3;
      }
;

sym_list_sym_lit:
      sym_lit_sym_word |
      sym_list_sym_lit COMMA sym_lit_sym_word
;

sym_lit_sym_word:
    SYMBOL { 
      Assembler::handleWordSymbol($1);
      delete $1;
    } |
    LITERAL {
      Assembler::handleWordLiteral($1);
    }
;

section_name_t:
  SYMBOL { $$ = $1; } |
  SECTION_NAME { $$ = $1; }
;

instruction:
  HALT { Assembler::handleZeroArgInstruction(Instruction::OCMOD::HALT); } |
  INT  { Assembler::handleZeroArgInstruction(Instruction::OCMOD::INT); } |
  IRET { Assembler::handleIRet(); } |
  RET  { Assembler::handleRet(); } |
  gpr_instruction |
  jmp_instruction |
  mem_instruction 
;

gpr_instruction:
  ADD GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::ADD, $4, $2); } |
  SUB GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::SUB, $4, $2); } |
  MUL GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::MUL, $4, $2); } |
  DIV GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::DIV, $4, $2); } |
  NOT GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::NOT, $2, $2); } |
  AND GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::AND, $4, $2); } |
  OR GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::OR, $4, $2); } |
  XOR GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::XOR, $4, $2); } |
  SHL GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::SHL, $4, $2); } |
  SHR GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::SHR, $4, $2); } |
  XCHG GPR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::XCHG, $2, $4); } |
  CSRRD CSR COMMA GPR { Assembler::handleGPRInstruction(Instruction::OCMOD::CSRRD, $4, $2);} |
  CSRWR GPR COMMA CSR { Assembler::handleGPRInstruction(Instruction::OCMOD::CSRWR, $4, $2);}
;

jmp_instruction:
  CALL LITERAL { Assembler::handleJumpLiteralInstruction(Instruction::OCMOD::CALL_DIR, 0, 0, $2);} |
  JMP LITERAL { Assembler::handleJumpLiteralInstruction(Instruction::OCMOD::JMP_DIR, 0, 0, $2);} |
  BEQ GPR COMMA GPR COMMA LITERAL { Assembler::handleJumpLiteralInstruction(Instruction::OCMOD::BEQ_DIR, $2, $4, $6);} |
  BNE GPR COMMA GPR COMMA LITERAL { Assembler::handleJumpLiteralInstruction(Instruction::OCMOD::BNE_DIR, $2, $4, $6);} |
  BGT GPR COMMA GPR COMMA LITERAL { Assembler::handleJumpLiteralInstruction(Instruction::OCMOD::BGT_DIR, $2, $4, $6);} |

  CALL SYMBOL { Assembler::handleJumpSymbolInstruction(Instruction::OCMOD::CALL_DIR, 0, 0, $2); delete $2; } |
  JMP SYMBOL { Assembler::handleJumpSymbolInstruction(Instruction::OCMOD::JMP_DIR, 0, 0, $2); delete $2; } |
  BEQ GPR COMMA GPR COMMA SYMBOL { Assembler::handleJumpSymbolInstruction(Instruction::OCMOD::BEQ_DIR, $2, $4, $6); delete $6;} |
  BNE GPR COMMA GPR COMMA SYMBOL { Assembler::handleJumpSymbolInstruction(Instruction::OCMOD::BNE_DIR, $2, $4, $6); delete $6;} |
  BGT GPR COMMA GPR COMMA SYMBOL { Assembler::handleJumpSymbolInstruction(Instruction::OCMOD::BGT_DIR, $2, $4, $6); delete $6;}
;

mem_instruction:
  PUSH GPR { Assembler::handlePush($2); } |
  POP GPR { Assembler::handlePop($2); } |

  LD LITERAL COMMA GPR { Assembler::handleLoadStoreIndLiteral(Instruction::OCMOD::LD_MEM, $4, 0, 0, $2); } |
  ST GPR COMMA LITERAL { Assembler::handleLoadStoreIndLiteral(Instruction::OCMOD::ST_IND, 0, 0, $2, $4); } |

  LD SYMBOL COMMA GPR { Assembler::handleLoadStoreIndSymbol(Instruction::OCMOD::LD_MEM, $4, 0, 0, $2); delete $2; } |
  ST GPR COMMA SYMBOL { Assembler::handleLoadStoreIndSymbol(Instruction::OCMOD::ST_IND, 0, 0, $2, $4); delete $4; } |

  LD DOLLAR LITERAL COMMA GPR { Assembler::handleLoadStoreDirLiteral(Instruction::OCMOD::LD_REG_DISP, $5, $3); } |
  ST GPR COMMA DOLLAR LITERAL { Assembler::handleLoadStoreDirLiteral(Instruction::OCMOD::ST_DIR, $2, $5); } |

  LD DOLLAR SYMBOL COMMA GPR { Assembler::handleLoadStoreDirSymbol(Instruction::OCMOD::LD_REG_DISP, $5, $3); delete $3; } |
  ST GPR COMMA DOLLAR SYMBOL { Assembler::handleLoadStoreDirSymbol(Instruction::OCMOD::ST_DIR, $2, $5); delete $5; } |

  LD GPR COMMA GPR { Assembler::handleLoadStoreRegInstruction(Instruction::OCMOD:: LD_REG_DISP, $4, $2); } |
  ST GPR COMMA GPR { Assembler::handleLoadStoreRegInstruction(Instruction::OCMOD:: ST_DIR, $4, $2); } |

  LD LEFT_BRACKET GPR RIGHT_BRACKET COMMA GPR { Assembler::handleLoadStoreRegInstruction(Instruction::OCMOD::LD_MEM, $6, $3); } |
  ST GPR COMMA LEFT_BRACKET GPR RIGHT_BRACKET  { Assembler::handleLoadStoreRegInstruction(Instruction::OCMOD::ST_IND, $5, $2); } |

  LD LEFT_BRACKET GPR PLUS LITERAL RIGHT_BRACKET COMMA GPR { 
    Assembler::handleLoadStoreRegLiteral(Instruction::OCMOD::LD_MEM, $8, $3, $5);
    } |
  ST GPR COMMA LEFT_BRACKET GPR PLUS LITERAL RIGHT_BRACKET { 
    Assembler::handleLoadStoreRegLiteral(Instruction::OCMOD::ST_IND, $5, $2, $7);
    } |

  LD LEFT_BRACKET GPR PLUS SYMBOL RIGHT_BRACKET COMMA GPR { 
    Assembler::handleLoadStoreRegSymbol(Instruction::OCMOD::LD_MEM, $8, $3, $5);
    delete $5;
    } |
  ST GPR COMMA LEFT_BRACKET GPR PLUS SYMBOL RIGHT_BRACKET { 
    Assembler::handleLoadStoreRegSymbol(Instruction::OCMOD::ST_IND, $5, $2, $7);
    delete $7;
    } 
;

%%

void yyerror(const char *s) {
  std::cout << "Parse error,  Message: " << s << std::endl << "line num: " << line_num << std::endl;

  exit(-1);
}
