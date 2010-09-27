AC_PREREQ(2.59)

AC_INIT([LIBFCL], [0.0.1],
        [Olivier Delhomme],
        [libfcl])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([$PACKAGE_TARNAME], [$PACKAGE_VERSION])

dnl **************************************************
dnl * Directory requirements                         *
dnl **************************************************
TOP_DIR=`pwd`
SRC_DIR="${TOP_DIR}/src"
AC_SUBST(TOP_DIR)
AC_SUBST(SRC_DIR)

dnl **************************************************
dnl * Libraries requirements                         *
dnl **************************************************
GLIB2_VERSION=2.10.0
GIO_VERSION=2.10.0
AC_SUBST(GLIB2_VERSION)
AC_SUBST(GIO_VERSION)

AM_MAINTAINER_MODE

dnl **************************************************
dnl * Checks for headers                             *
dnl **************************************************
AC_HEADER_STDC

dnl **************************************************
dnl * Checks for typedefs, structures, and           *
dnl * compiler characteristics                       *
dnl **************************************************
# AC_C_CONST
# AC_TYPE_PID_T
# AC_TYPE_SIZE_T
# AC_TYPE_SIGNAL
# AC_STRUCT_TM

dnl **************************************************
dnl * Checks dynamic libraries capabilities          *
dnl **************************************************
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl **************************************************
dnl * Checks for paths                               *
dnl **************************************************
if test "x$prefix" = "xNONE"; then
    prefix="/usr/local"
fi

if test "x$localstatedir" = "x\${prefix}/var"; then
    localstatedir="$prefix/var"
fi

if test "x$sysconfdir" = "x\${prefix}/etc"; then
    sysconfdir="$prefix/etc"
fi

AC_SUBST(localstatedir)
AC_SUBST(sysconfdir)
AC_SUBST(prefix)

dnl **************************************************
dnl * languages                                      *
dnl **************************************************
ALL_LINGUAS=""

dnl **************************************************
dnl * triggers gettext                               *
dnl **************************************************
GETTEXT_PACKAGE=libfcl
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package used])
AM_GLIB_GNU_GETTEXT
LOCALEDIR=[${datadir}/locale]
AC_SUBST(LOCALEDIR)

dnl **************************************************
dnl * checking for glib                              *
dnl **************************************************
PKG_CHECK_MODULES(GLIB2,[glib-2.0 >= $GLIB2_VERSION])

dnl **************************************************
dnl * checking for gio                              *
dnl **************************************************
PKG_CHECK_MODULES(GIO,[gio-2.0 >= $GIO_VERSION])

AC_PROG_INSTALL

CFLAGS="$CFLAGS -Wall -Wstrict-prototypes -Wmissing-declarations \
-Wbad-function-cast -Wcast-align -Wnested-externs -Wunused \
-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
-DGSEAL_ENABLE -DG_DISABLE_DEPRECATED=1"

case $host in
    *linux*)
        AC_DEFINE_UNQUOTED(SYS_LINUX, 1, [This is a Linux system])
        LDFLAGS="$LDFLAGS -export-dynamic -no-undefined -module"
    ;;
    *mingw*)
        AC_DEFINE_UNQUOTED(SYS_MINGW, 1, [This is a Windows system using MinGW])
        LDFLAGS="$LDFLAGS -export-dynamic -no-undefined -module -Wl,--export-all-symbols"
        CFLAGS="$CFLAGS -mms-bitfields -mwindows -mno-cygwin"
    ;;
esac

AC_SUBST(LDFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile libfcl.pc])
AC_OUTPUT


echo
echo "*** Dumping configuration ***"
echo
echo "    - Host                   : $host"
echo "    - Compiler               : ${CC}"
echo "    - Prefix                 : ${prefix}"
echo "    - Exec prefix            : $exec_prefix"
echo "    - Data dir               : $datadir"
echo "    - Include dir            : $includedir"
echo
echo "    You can now run 'make' to compile libfcl"
echo
