SRC = src
INC = inc
MISC = misc

COMMON_DIR = $(SRC)/common
COMMON_FILES = $(COMMON_DIR)/*.cpp

ASSEMBLER_DIR = $(SRC)/assembler
ASSEMBLER_FILES = $(ASSEMBLER_DIR)/*.cpp 

LINKER_DIR = $(SRC)/linker
LINKER_FILES = $(LINKER_DIR)/*.cpp

EMULATOR_DIR = $(SRC)/emulator
EMULATOR_FILES = $(EMULATOR_DIR)/*.cpp


LEXER_GENERATOR=  $(MISC)/lexer.l
PARSER_GENERATOR = $(MISC)/parser.y

LEXER_CPP= $(MISC)/lexer.cpp
PARSER_CPP = $(MISC)/parser.cpp

LEXER_HPP= $(MISC)/lexer.hpp
PARSER_HPP = $(MISC)/parser.hpp

all: parsing assembler linker emulator

parsing: 
	bison -d $(PARSER_GENERATOR)
	flex $(LEXER_GENERATOR)

assembler: parsing  
	g++ -std=c++11 -fdiagnostics-color=always -g -o ${@} \
	$(COMMON_FILES) $(ASSEMBLER_FILES) $(PARSER_CPP) $(LEXER_CPP)

linker: $(COMMON_FILES) $(LINKER_FILES)
	g++ -std=c++11 -fdiagnostics-color=always -g -o ${@} ${^}

emulator: $(COMMON_FILES) $(EMULATOR_FILES)
	g++ -std=c++11 -fdiagnostics-color=always -g -o ${@} ${^} -pthread
	
clean:
	rm -f assembler linker $(PARSER_CPP) $(LEXER_CPP) $(PARSER_HPP) $(LEXER_HPP)



	