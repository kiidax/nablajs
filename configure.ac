AC_INIT([Nabla JS], [0.1.1], [katsuya.iida@gmail.com], [nabla], [http://kiidax.wordpress.com/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CXX
AM_PROG_LEX
AC_PROG_YACC

AC_HEADER_ASSERT

AC_ARG_WITH([readline],
  [AS_HELP_STRING([--with-readline],
  [support fancy command line editing @<:@default=check@:>@])],
  [],
  [with_readline=check])
          
LIBREADLINE=
AS_IF([test "x$with_readline" != xno],
  [AC_CHECK_LIB([readline], [main],
  [AC_SUBST([LIBREADLINE], ["-lreadline -lncurses"])
   AC_DEFINE([HAVE_LIBREADLINE], [1],
             [Define if you have libreadline])
  ],
  [if test "x$with_readline" != xcheck; then
     AC_MSG_FAILURE(
       [--with-readline was given, but test for readline failed])
   fi
  ], -lncurses)])

PKG_CHECK_MODULES(LIBPCRE16, libpcre16)
PKG_CHECK_MODULES(GC, bdw-gc)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile nabla/Makefile tests/Makefile])
AC_OUTPUT
