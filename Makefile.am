SUBDIRS=src persclient_tool

if WANT_TESTS
SUBDIRS+=test
endif


sysconfdir = "/etc/"

# Write the config file for customer plugins to the config destination 
sysconf_DATA = config/pclCustomLibConfigFile.cfg

# Add config file to distribution 
EXTRA_DIST = $(sysconf_DATA) 

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=persistence_client_library.pc

MAINTAINERCLEANFILES = \
      Makefile.in aclocal.m4 compile config.guess config.h.in config.sub \
      configure depcomp install-sh ltmain.sh persistence_client_library.pc config.h.in~ Makefile missing
