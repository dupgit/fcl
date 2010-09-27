SUBDIRS = src test

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = 		\
	libfcl.pc.in	\
	autogen.sh		\
	autoclean.sh	\
	RoadMap			\
	HISTORY

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfcl.pc

$(pkgconfig_DATA): config.status
