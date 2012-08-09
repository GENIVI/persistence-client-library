AUTOMAKE_OPTIONS = foreign

AM_CFLAGS =$(DEPS_CFLAGS) -Wall -I../include \
                          -I/usr/include/glib-2.0 \
                          -I/usr/include/dbus-1.0 \
                          -I/usr/include/dconf-dbus-1 \
                          -I/usr/lib/glib-2.0/include                          


include_HEADERS = ../include/persistence_client_library_key.h ../include/persistence_client_library_file.h


lib_LTLIBRARIES = libpersistence_client_library.la

libpersistence_client_library_la_LIBADD = $(DEPS_LIBS) -ldconf -ldl -ldconf-dbus-1 -lmygvdb

libpersistence_client_library_la_SOURCES = \
                                     persistence_client_library.c \
                                     persistence_client_library_key.c \
                                     persistence_client_library_file.c \
                                     persistence_client_library_data_access.c \
                                     persistence_client_library_handle.c \
                                     persistence_client_library_lc_interface.c \
                                     persistence_client_library_pas_interface.c \
                                     persistence_client_library_dbus_service.c \
                                     crc32.c

libpersistence_client_library_la_LDFLAGS = -module -export-dynamic $(LDFLAGS)


