#!/bin/bash

BUILD_COMMAND="make clean; make linux -j 1"
RUN_COMMAND='FILES=(bitwise.lua calls.lua closure.lua code.lua constructs.lua coroutine.lua db.lua errors.lua events.lua gc.lua goto.lua literals.lua locals.lua math.lua nextvar.lua pm.lua sort.lua strings.lua tpack.lua utf8.lua vararg.lua); cd tests; for file in ${FILES[@]}; do echo ${file}; ../src/lua ${file}; done'
