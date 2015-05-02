plugin_LTLIBRARIES = greycstoration.la
greycstoration_la_LDFLAGS = -avoid-version -module -shared
greycstoration_la_LIBADD =
greycstoration_la_SOURCES = greycstorationwindow.C greycstorationplugin.C
AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = greycstorationplugin.h greycstorationwindow.h greycstoration.h
EXTRA_DIST = picon.png

BUILT_SOURCES = picon_png.h
picon_png.h: picon.png
	$(top_builddir)/guicast/pngtoh $^ || { rm -f $@; exit 1; }
clean-local:
	$(RM) picon_png.h