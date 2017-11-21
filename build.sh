#!/bin/bash

g++ -c ./src/Prologue.cpp -o ./bin/Prologue.o -O2 -I./include
g++ -c ./src/Epilogue.cpp -o ./bin/Epilogue.o -O2 -I./include
g++ -c ./src/PrologueT.cpp -o ./bin/PrologueT.o -O2 -I./include
g++ -c ./src/EpilogueT.cpp -o ./bin/EpilogueT.o -O2 -I./include
g++ -c ./src/Util.cpp -o ./bin/Util.o -O2 -I./include
g++ -c ./src/ParallelDomainTransform.cpp -o ./bin/ParallelDomainTransform.o -O2 -I./include
g++ -c ./src/Main.cpp -o ./bin/Main.o -O2 -I./include
g++ ./bin/Prologue.o ./bin/Epilogue.o ./bin/PrologueT.o ./bin/EpilogueT.o ./bin/Util.o ./bin/ParallelDomainTransform.o ./bin/Main.o -o ./bin/parallel-domain-transform -O2 -lpthread
