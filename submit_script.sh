#!/bin/bash

mkdir -p "vsopcompiler"
mkdir -p "vsopcompiler/tests"

cp driver.cpp driver.hpp lexer.lex main.cpp Makefile parser.y ast.cpp ast.hpp symbol_table.hpp symbol_table.cpp type_checker.hpp type_checker.cpp ir.cpp ir.hpp object.ll object.c object.h vsopcompiler/

cp report.pdf vsopcompiler/

cp tests/4_ir/*.vsop vsopcompiler/tests/

tar -cJf vsopcompiler.tar.xz vsopcompiler

rm -rf vsopcompiler

echo "vsopcompiler.tar.xz has been created successfully."
