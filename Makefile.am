SUBDIRS=src

if WANT_TESTS
SUBDIRS+=test
endif

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=persistence_client_library.pc

MAINTAINERCLEANFILES = \
      Makefile.in aclocal.m4 compile config.guess config.h.in config.sub \
      configure depcomp install-sh ltmain.sh persistence_client_library.pc config.h.in~ Makefile missing
