#!/bin/bash

BUILD_COMMAND="rm config.cache -f; ./configure; make clean; make all -j 1"
RUN_COMMAND="cd gas; make check"
