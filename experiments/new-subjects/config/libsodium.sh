#!/bin/bash

BUILD_COMMAND="make distclean; ./configure --without-pthreads; make clean; make all -j 1"
