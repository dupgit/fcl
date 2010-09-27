SUBDIRS =

DEFS = -I$(SRC_DIR)  						\
       -I$(top_builddir)/src/include		\
       $(CFLAGS) $(GLIB2_CFLAGS)	     	\
	   -DLOCALEDIR=\"${LOCALEDIR}\"

bin_PROGRAMS = testlibfcl
testlibfcl_LDFLAGS = $(LDFLAGS)
testlibfcl_LDADD = $(GLIB2_LIBS) -L$(top_builddir)/src/ -lfcl

testlibfcl_SOURCES =		\
	libfcltest.c			\
	libfcltest.h

AM_CPPFLAGS = 						\
	$(GLIB2_CFLAGS) 				\
	-DLOCALEDIR=\"${LOCALEDIR}\"
