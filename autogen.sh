#!/bin/bash
# autogen.sh
set -e

mkdir -p m4
glibtoolize -c -f
aclocal
autoconf -f
autoheader -f
automake -a -c -f
