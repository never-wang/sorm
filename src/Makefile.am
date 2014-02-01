lib_LTLIBRARIES = libsorm.la
libsorm_la_SOURCES = sorm.c sorm.h log.c log.h semaphore.c semaphore.h

libsorm_la_LIBADD = $(ZLOG_DIR)/libzlog.la $(SQLITE_DIR)/libsqlite.la
libsorm_la_CFLAGS = -I$(ZLOG_DIR) -I$(SQLITE_DIR)
libsorm_la_LDFLAGS = -ldl

AM_YFLAGS = -d
BUILT_SOURCES = parser_yacc.h
bin_PROGRAMS = sorm_generate
sorm_generate_SOURCES = generate.c generate.h header_generate.c header_generate.h \
						c_generate.c c_generate.h parser_yacc.y parser_lex.l \
						log.c log.h
sorm_generate_LDADD = libsorm.la 
sorm_generate_CFLAGS = -I$(ZLOG_DIR) -I$(SQLITE_DIR)

include_HEADERS = sorm.h
