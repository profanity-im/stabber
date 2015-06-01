lib_LTLIBRARIES = libstabber.la

sources = \
	src/server/server.c src/server/server.h \
	src/server/httpapi.c src/server/httpapi.h \
	src/server/xmppclient.c src/server/xmppclient.h \
	src/server/stream_parser.c src/server/stream_parser.h \
	src/server/parser.c src/server/parser.h \
    src/server/stanza.c src/server/stanza.h \
    src/server/log.c src/server/log.h \
    src/server/prime.c src/server/prime.h \
    src/server/verify.c src/server/verify.h \
    src/client/stabber.c src/client/stabber.h

libstabber_la_LDFLAGS = -export-symbols-regex '^stbbr_'
libstabber_la_SOURCES = $(sources)

include_HEADERS = stabber.h

noinst_PROGRAMS = stabbertest
stabbertest_SOURCES = stabbertest.c
stabbertest_CFLAGS = -I$(top_srcdir)
stabbertest_LDADD = libstabber.la -lpthread

bin_PROGRAMS = stabber
stabber_SOURCES = stabber.c
stabber_CFLAGS = -I$(top_srcdir)
stabber_LDADD = libstabber.la -lpthread
