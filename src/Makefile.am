AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS =$(DEPS_CFLAGS) -g -I../include \
                          -Iglib-2.0 \
                          -Idbus-1.0 \
                          -Idconf-dbus-1 \
                          -Iglib-2.0/include
else
AM_CFLAGS =$(DEPS_CFLAGS) -I../include \
                          -Iglib-2.0 \
                          -Idbus-1.0 \
                          -Idconf-dbus-1 \
                          -Iglib-2.0/include
endif


include_HEADERS = ../include/persistence_client_library_key.h \
                  ../include/persistence_client_library_file.h \
                  ../include/persistence_client_library_error_def.h
                  

lib_LTLIBRARIES = libpersistence_client_library.la

libpersistence_client_library_la_LIBADD = $(DEPS_LIBS) -ldconf -ldl -ldconf-dbus-1 -lgvdb


libpersistence_client_library_la_SOURCES = \
                                     persistence_client_library.c \
                                     persistence_client_library_key.c \
                                     persistence_client_library_file.c \
                                     persistence_client_library_data_access.c \
                                     persistence_client_library_handle.c \
                                     persistence_client_library_lc_interface.c \
                                     persistence_client_library_pas_interface.c \
                                     persistence_client_library_dbus_service.c \
                                     persistence_client_library_custom_loader.c \
                                     persistence_client_library_access_helper.c \
                                     crc32.c

libpersistence_client_library_la_LDFLAGS = -export-dynamic $(LDFLAGS)


