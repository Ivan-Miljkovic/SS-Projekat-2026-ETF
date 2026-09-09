/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_MISC_PARSER_HPP_INCLUDED
# define YY_YY_MISC_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 16 "misc/parser.y"

  #include <cstdint>
  #include <string>
  #include "../inc/assembler/assembler.hpp"

#line 54 "misc/parser.hpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ENDL = 258,
    LEFT_BRACKET = 259,
    RIGHT_BRACKET = 260,
    COMMA = 261,
    PLUS = 262,
    DOLLAR = 263,
    COLON = 264,
    LITERAL = 265,
    SYMBOL = 266,
    ASCII_STRING = 267,
    GLOBAL = 268,
    EXTERN = 269,
    SECTION = 270,
    WORD = 271,
    SKIP = 272,
    ASCII = 273,
    EQU = 274,
    END = 275,
    SECTION_NAME = 276,
    GPR = 277,
    CSR = 278,
    HALT = 279,
    INT = 280,
    IRET = 281,
    CALL = 282,
    RET = 283,
    JMP = 284,
    BEQ = 285,
    BNE = 286,
    BGT = 287,
    PUSH = 288,
    POP = 289,
    XCHG = 290,
    ADD = 291,
    SUB = 292,
    MUL = 293,
    DIV = 294,
    NOT = 295,
    AND = 296,
    OR = 297,
    XOR = 298,
    SHL = 299,
    SHR = 300,
    LD = 301,
    ST = 302,
    CSRRD = 303,
    CSRWR = 304
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 22 "misc/parser.y"

  std::string *str;
  std::uint32_t num;
  int reg;

#line 121 "misc/parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MISC_PARSER_HPP_INCLUDED  */
