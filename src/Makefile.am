lib_LTLIBRARIES = libsorm.la
libsorm_la_SOURCES = sorm.c sorm.h memory.c memory.h log.c log.h semaphore.c semaphore.h

libsorm_la_LIBADD = ../third-part/zlog-1.2.1/libzlog.la ../third-part/sqlite3/libsqlite.la
libsorm_la_CFLAGS = -I../third-part/zlog-1.2.1 -I../third-part/sqlite3

AM_YFLAGS = -d
BUILT_SOURCES = parser_yacc.h
bin_PROGRAMS = sorm_generate
sorm_generate_SOURCES = generate.c generate.h header_generate.c header_generate.h \
						c_generate.c c_generate.h parser_yacc.y parser_lex.l \
						log.c log.h
sorm_generate_LDADD = libsorm.la -ldl
sorm_generate_CFLAGS = -I../third-part/zlog-1.2.1

include_HEADERS = sorm.h
