#!/bin/bash

g++ -c ./src/Util.cpp -o ./bin/Util.o -O2 -I./include
g++ -c ./src/DomainTransform.cpp -o ./bin/DomainTransform.o -O2 -I./include
g++ -c ./src/Main.cpp -o ./bin/Main.o -O2 -I./include
g++ ./bin/Util.o ./bin/DomainTransform.o ./bin/Main.o -o ./bin/domain-transform -O2 -lpthread
