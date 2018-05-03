#!/bin/bash

lib_path="-L/usr/local/lib/"
lib_includes="-lopencv_calib3d -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_video -lopencv_videoio -lopencv_videostab"
additional_flags="-g -m64"
output_path="./bin/debug"

mkdir -p bin

g++ -c ./src/Main.cpp -o ./bin/main.o -I./include/ $additional_flags
g++ -c ./src/Prologue.cpp -o ./bin/Prologue.o -I./include $additional_flags
g++ -c ./src/Epilogue.cpp -o ./bin/Epilogue.o  -I./include $additional_flags
g++ -c ./src/PrologueT.cpp -o ./bin/PrologueT.o -I./include $additional_flags
g++ -c ./src/EpilogueT.cpp -o ./bin/EpilogueT.o -I./include $additional_flags
g++ -c ./src/Util.cpp -o ./bin/Util.o -I./include $additional_flags
g++ -c ./src/ParallelDomainTransform.cpp -o ./bin/ParallelDomainTransform.o -I./include $additional_flags
g++ $lib_path $lib_includes -g ./bin/Prologue.o ./bin/Epilogue.o ./bin/PrologueT.o ./bin/EpilogueT.o ./bin/main.o ./bin/ParallelDomainTransform.o ./bin/Util.o -o $output_path -I./include/ $additional_flags
