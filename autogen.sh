#!/bin/sh
# vi:set sts=2 sw=2 autoindent:
#
# autogen.sh - CVS intermediate file autogenerator for dds-tools.
#

aclocal || exit $?
autoconf || exit $?
automake --add-missing --include-deps || exit $?

