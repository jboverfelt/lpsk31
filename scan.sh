#!/bin/sh
./autogen.sh
scan-build ./configure
scan-build make -j5
./distclean
#
