plugin_LTLIBRARIES = findobject.la
findobject_la_LDFLAGS = -avoid-version -module -shared 
findobject_la_LIBADD = -lopencv_legacy -lopencv_objdetect -lopencv_core
findobject_la_SOURCES = findobject.C findobjectwindow.C surfscan.C

AM_CXXFLAGS = $(LARGEFILE_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/guicast -I$(top_srcdir)/cinelerra -I$(top_srcdir)/quicktime
LIBTOOL = $(SHELL) $(top_builddir)/libtool $(LTCXX_FLAGS)

noinst_HEADERS = findobject.h findobjectwindow.h surfscan.h findobject.inc findobjectwindow.inc
