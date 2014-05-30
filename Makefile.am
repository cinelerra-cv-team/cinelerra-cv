if HAVE_LIBMPEG3_INTERNAL
  MAYBE_LIBMPEG3 = libmpeg3
endif
if HAVE_MPEG3IO_EOF
  MAYBE_MPLEXLO = mplexlo
endif

SUBDIRS=$(MAYBE_LIBMPEG3) $(MAYBE_MPLEXLO) mpeg2enc toolame-02l \
	quicktime guicast cinelerra plugins po image m4
EXTRA_DIST = admin debian doc depcomp README.BUILD LICENSE \
	cinelerra-cvs-current.spec
AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4
