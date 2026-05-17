CXX 			= clang++

CXXFLAGS 		= -Wall -Wextra

BISONFLAGS 		= -d -Wcounterexamples

EXEC			= vsopc

SRC				= main.cpp \
				  driver.cpp \
				  parser.cpp \
				  lexer.cpp \
				  ast.cpp \
				  type_checker.cpp \
      			  symbol_table.cpp \
				  ir.cpp

OBJ	  			= $(SRC:.cpp=.o)


all: $(EXEC)

main.o: driver.hpp parser.hpp

driver.o: driver.hpp parser.hpp type_checker.hpp

parser.o: driver.hpp parser.hpp

lexer.o: driver.hpp parser.hpp

ast.o: ast.hpp

type_checker.o: type_checker.hpp symbol_table.hpp

symbol_table.o: symbol_table.hpp

ir.o: ir.hpp ast.hpp symbol_table.hpp


$(EXEC): $(OBJ)
	$(CXX) -o $@ $(LDFLAGS) $(OBJ)

parser.cpp: parser.y
	bison $(BISONFLAGS) -o parser.cpp $^

parser.hpp: parser.y
	bison $(BISONFLAGS) -o parser.cpp $^

lexer.cpp: lexer.lex
	flex $(LEXFLAGS) -o lexer.cpp $^

clean:
	@rm -f $(EXEC)
	@rm -f $(OBJ)
	@rm -f lexer.cpp
	@rm -f parser.cpp parser.hpp location.hh

.PHONY: clean install-tools

install-tools:
	mkdir -p $(HOME)/.vsop
	cp object.ll $(HOME)/.vsop/object.ll
