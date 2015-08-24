BUILT_SOURCES = parser.hh startup.cc
AM_YFLAGS = -d
nabla_CPPFLAGS = $(GC_CFLAGS) $(LIBPCRE16_CFLAGS)
nabla_LDADD = $(GC_LIBS) $(LIBPCRE16_LIBS) $(LIBREADLINE)
bin_PROGRAMS = nabla
nabla_SOURCES = api.cc ast.cc evalast.cc context.cc data.cc builtin.cc \
	nabla.cc test.cc startup.cc\
	parser.yy token.ll

startup.cc: startup.js text2c.sh
	sh ./text2c.sh startup.js startup.cc
