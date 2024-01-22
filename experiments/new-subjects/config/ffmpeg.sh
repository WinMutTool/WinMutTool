#!/bin/bash

BUILD_COMMAND="make distclean; ./configure --disable-pthreads --disable-debug --cc='$CC' --cxx='$CXX'; make clean; make all -j 1"
