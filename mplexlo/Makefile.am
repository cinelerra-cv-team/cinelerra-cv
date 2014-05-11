bin_PROGRAMS=mplexlo

AM_CPPFLAGS=-I$(top_srcdir)/libmpeg3
AM_CFLAGS = $(LARGEFILE_CFLAGS)
mplexlo_LDADD=../libmpeg3/libmpeg3hv.la
mplexlo_LDFLAGS=-lm -lpthread
mplexlo_SOURCES=mplex.c
