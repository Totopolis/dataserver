#!/bin/bash 

if [ -d "bin" ]; then
  rm -r bin
fi

if [ -d "build" ]; then
  rm -r build
fi

mkdir build
cd build
#cmake -G Xcode ..
cmake -G "Sublime Text 2 - Unix Makefiles" ..
#cmake ..
