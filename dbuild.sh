#!/bin/bash

g++ -c ./src/Util.cpp -o ./bin/Util.o -g -I./include
g++ -c ./src/DomainTransform.cpp -o ./bin/DomainTransform.o -g -I./include
g++ -c ./src/Main.cpp -o ./bin/Main.o -g -I./include
g++ ./bin/Util.o ./bin/DomainTransform.o ./bin/Main.o -o ./bin/domain-transform -g -lpthread