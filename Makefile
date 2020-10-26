# Copyright 2020 SiFive, Inc #
# SPDX-License-Identifier: Apache-2.0 #

PROGRAM ?= test-wfi-multicore

$(PROGRAM): $(wildcard *.c) $(wildcard *.h) $(wildcard *.S)

clean:
	rm -f $(PROGRAM) $(PROGRAM).hex

