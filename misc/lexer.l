%{
  #include "parser.hpp"
  #include <iostream>
  #include <string>
  #include <stdlib.h>
  #include <stdint.h>

  #define YYDEBUG 1
  int line_num = 1;
  extern int yyerror(const char*);
%}

%option outfile="misc/lexer.cpp" header-file="misc/lexer.hpp"
%option noyywrap

%%

\.global { return GLOBAL; }
\.extern { return EXTERN; }
\.section { return SECTION; }
\.word { return WORD; }
\.skip { return SKIP; }
\.ascii { return ASCII; }
\.equ { return EQU; }
\.end { return END; }

halt { return HALT; }
int { return INT; }
iret { return IRET; }
call { return CALL; }
ret { return RET; }
jmp { return JMP; }
beq { return BEQ; }
bne { return BNE; }
bgt { return BGT; }
push { return PUSH; }
pop { return POP; }
xchg { return XCHG; }
add { return ADD; }
sub { return SUB; }
mul { return MUL; }
div { return DIV; }
not { return NOT; }
and { return AND; }
or { return OR; }
xor { return XOR; }
shl { return SHL; }
shr { return SHR; }
ld { return LD; }
st { return ST; }
csrrd { return CSRRD; }
csrwr { return CSRWR; }


[ \t]+ { }
"#".* { }
\n {++line_num; return ENDL;}
([0-9]+|(0[Xx][0-9A-Fa-f]+)) { yylval.num = (uint32_t) strtoul(yytext, NULL, 0); return LITERAL; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str = new std::string(yytext); return SYMBOL; }
\[ { return LEFT_BRACKET; }
\] { return RIGHT_BRACKET; }
\, { return COMMA; }
\+ { return PLUS; }
\: { return COLON; }
\$ { return DOLLAR; }
\.[A-Za-z_][A-Za-z0-9]* { yylval.str = new std::string(yytext); return SECTION_NAME; }
\"([^"^#]*)\" { yylval.str =  new std::string(yytext + 1, yyleng - 2); return ASCII_STRING; }
%(sp|pc|(r[0-9]|r1[0-5])) {
  std::string reg = yytext;
  if(reg == "%sp") yylval.reg = 14;
  else if(reg == "%pc") yylval.reg = 15;
  else yylval.reg = (uint8_t)strtoul(yytext+2, NULL, 10);
  return GPR;
}
"%"(status|handler|cause) {
  std::string reg = yytext;
  if(reg == "%status") yylval.reg = 0;
  else if(reg == "%handler") yylval.reg = 1;
  else yylval.reg = 2;
  return CSR;
}
. {
  std::cout << "UKNOWN: '" << yytext << "'" << std::endl;
  yyerror("Uknown symbol");
}
%%


