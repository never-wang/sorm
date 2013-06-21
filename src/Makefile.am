lib_LTLIBRARIES = libsorm.la
libsorm_la_SOURCES = sorm.c sorm.h memory.c memory.h

libsorm_la_LIBADD = ../third-part/zlog-1.2.1/libzlog.la ../../NoahArk/third-part/sqlite3/libsqlite.la
libsorm_la_CFLAGS = -I../third-part/zlog-1.2.1 -I../../NoahArk/third-part/sqlite3

AM_YFLOAGS = -d

bin_PROGRAMS = sorm_generate
sorm_generate_SOURCES = generate.c generate.h header_generate.c header_generate.h \
						c_generate.c c_generate.h parser_yacc.y parser_lex.l 
sorm_generate_LDADD = libsorm.la -ldl
sorm_generate_CFLAGS = -I../third-part/zlog-1.2.1
