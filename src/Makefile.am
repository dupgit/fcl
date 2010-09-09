AUTOMAKE_OPTIONS = no-dependencies foreign

DEFS = 	-DPLUGINS_DIR=\"$(plugins_dir)\" 	\
		-DDATA_DIR=\"$(datadir)\" 			\
		-I$(top_srcdir)/include 			\
		-I$(srcdir)/  						\
		$(GLIB2_CFLAGS) $(CFLAGS)			\
		-I$(TOP_DIR) -I$(SRC_DIR)/include

libdir=$(exec_prefix)/lib
includedir=$(prefix)/include

headerfiles = 				\
	include/fcl.h

lib_LTLIBRARIES = libfcl.la
include_HEADERS = $(headerfiles)
libfcl_la_LDFLAGS = -version 0:0:1 -no-undefined -module -export-dynamic
libfcl_la_LIBADD = $(GLIB2_LIBS) $(LDFLAGS)

libfcl_la_SOURCES = 	\
	fcl.c				\
	$(headerfiles)
