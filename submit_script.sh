#!/bin/bash

mkdir -p "vsopcompiler"

cp driver.cpp driver.hpp lexer.lex main.cpp Makefile parser.y ast.cpp ast.hpp symbol_table.hpp symbol_table.cpp type_checker.hpp type_checker.cpp vsopcompiler/

tar -cJf vsopcompiler.tar.xz vsopcompiler

rm -rf vsopcompiler

echo "vsopcompiler.tar.xz has been created successfully."
