SUBDIRS=src

if WANT_TOOLS
SUBDIRS+=tools
endif

if WANT_TESTS
SUBDIRS+=test
endif


# Write the config file for customer plugins to the config destination 
sysconf_DATA = config/pclCustomLibConfigFile.cfg \
               config/pclCustomLibConfigFileEmpty.cfg

# Add config file to distribution 
EXTRA_DIST = $(sysconf_DATA) 

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=persistence_client_library.pc

MAINTAINERCLEANFILES = \
      Makefile.in aclocal.m4 compile config.guess config.h.in config.sub \
      configure depcomp install-sh ltmain.sh persistence_client_library.pc config/pclCustomLibConfigFile.cfg config/pclCustomLibConfigFileEmpty.cfg config.h.in~ Makefile missing
