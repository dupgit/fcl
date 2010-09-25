SUBDIRS =

DEFS = -I$(SRCDIR)  -I$(SRCDIR)	 			\
       -I$(top_builddir)/src/include		\
       $(CFLAGS) $(GLIB2_CFLAGS)	     	\
	   -DLOCALEDIR=\"${LOCALEDIR}\"

bin_PROGRAMS = libfcltest
heraia_LDFLAGS = $(LDFLAGS)
heraia_LDADD = $(GLIB2_LIBS) -L$(top_builddir)/src/ -lfcl


heraia_SOURCES = 					\
	libfcltest.c					\
	libfcltest.h

AM_CPPFLAGS = 			\
	$(GLIB2_CFLAGS) 	\
	-DLOCALEDIR=\"${LOCALEDIR}\"
