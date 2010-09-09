#!/bin/bash
#
# Author : Olivier Delhomme
# Licence : GNU GPL v2
#
if [ -f Makefile ];
 then
    make distclean
fi;
rm -f aclocal.m4
echo "rm -f aclocal.m4"
rm -fr autom4te.cache
echo "rm -fr autom4te.cache"
rm -f config.guess
echo "rm -f config.guess"
rm -f config.sub
echo "rm -f config.sub"
rm -f depcomp
echo "rm -f depcomp"
rm -f INSTALL
echo "rm -f INSTALL"
rm -f install-sh
echo "rm -f install-sh"
rm -f ltmain.sh
echo "rm -f ltmain.sh"
rm -f missing
echo "rm -f missing"
find . -name "Makefile" -exec rm -f {} \;
echo "find . -name Makefile -exec rm -f {} \;"
find . -name "Makefile.in" -exec rm -f {} \;
echo "find . -name Makefile.in -exec rm -f {} \;"
rm -f configure
echo "rm -f configure"
rm -f config.status config.log
echo "rm -f config.status config.log"
rm -f libtool
echo "rm -f libtool"
find . -name "*~" -exec rm -f {} \;
echo "find . -name *~ -exec rm -f {} \;"
find . -name "*#" -exec rm -f {} \;
echo "find . -name *# -exec rm -f {} \;"
find . -name *.gcd[ao] -exec rm -f {} \;
echo "find . -name *.gcd[ao] -exec rm -f {} \;"
find . -name *.gcov -exec rm -f {} \;
echo "find . -name *.gcov -exec rm -f {} \;"
find . -name *.gcno -exec rm -f {} \;
echo "find . -name *.gcno -exec rm -f {} \;"
find . -name gmon.out -exec rm -f {} \;
echo "find . -name gmon.out -exec rm -f {} \;"



