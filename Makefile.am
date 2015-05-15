lib_LTLIBRARIES = libstabber.la

sources = \
	src/server/server.c src/server/server.h \
	src/server/xmppclient.c src/server/xmppclient.h \
	src/server/parser.c src/server/parser.h \
    src/server/stanza.c src/server/stanza.h \
    src/client/stabber.c src/client/stabber.h

libstabber_la_LDFLAGS = -export-symbols-regex '^stabber_'
libstabber_la_SOURCES = $(sources)

include_HEADERS = stabber.h

noinst_PROGRAMS = runstabber
runstabber_SOURCES = runstabber.c
runstabber_CFLAGS = -I$(top_srcdir)
runstabber_LDADD = libstabber.la
