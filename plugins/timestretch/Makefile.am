plugin_LTLIBRARIES = timestretch.la
timestretch_la_LDFLAGS = -avoid-version -module -shared 
timestretch_la_LIBADD = ../libfourier/libfourier.la
timestretch_la_SOURCES = timestretch.C timestretchengine.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime -I$(top_srcdir)/plugins/libfourier
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = timestretch.h timestretchengine.h timestretchengine.inc
EXTRA_DIST = picon.png

BUILT_SOURCES = picon_png.h
picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }
clean-local:
	$(RM) picon_png.h
