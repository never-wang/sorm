AC_INIT([sorm], [1.0], [never.wencan@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_HEADER([config.h])

AC_PROG_CC
LT_INIT
AM_PROG_LEX
AC_PROG_YACC
AM_PROG_CC_C_O

ZLOG_DIR="$(pwd)/third-part/zlog-1.2.1"
SQLITE_DIR="$(pwd)/third-part/sqlite3"
SRC_DIR="$(pwd)/src"
UTILS_DIR="$(pwd)/utils"

AC_SUBST(ZLOG_DIR)
AC_SUBST(SQLITE_DIR)
AC_SUBST(SRC_DIR)
AC_SUBST(UTILS_DIR)

AC_ARG_ENABLE(debug, 
    [AS_HELP_STRING([--enable-debug],[turns -g on and -O2 off[default = no]])], 
    [[CFLAGS+=" -g"] 
    [CFLAGS+=" -O2"]
    AC_DEFINE([ENABLE_ZLOG], [], [enable zlog])
    AC_DEFINE_UNQUOTED([ZLOG_CONF], ["$PWD/src/zlog.conf"], [zlog config file path])])


AC_CONFIG_FILES([Makefile src/Makefile third-part/Makefile
                 third-part/zlog-1.2.1/Makefile 
                 third-part/sqlite3/Makefile
                 utils/Makefile
                 test/Makefile])

AC_OUTPUT

