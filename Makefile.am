bin_PROGRAMS = bdiff
bdiff_CFLAGS = $(GLIB_CFLAGS)
bdiff_SOURCES = \
	src/block.c \
	src/chunk.c \
	src/bdiff.c \
	src/diff_hunk.c \
	src/rabin.c \
	src/hash_multiset.c
bdiff_LDADD=$(GLIB_LIBS)

check_PROGRAMS = unittest_multiset unittest_rabin

unittest_rabin_CFLAGS=$(GLIB_CFLAGS) -Iinclude
unittest_rabin_SOURCES = \
	tests/unittest_rabin.c \
	src/rabin.c
unittest_rabin_LDADD=$(GLIB_LIBS)

unittest_multiset_CFLAGS=$(GLIB_CFLAGS) -Isrc
unittest_multiset_SOURCES = \
	tests/unittest_multiset.c \
	src/hash_multiset.c
unittest_multiset_LDADD=$(GLIB_LIBS)

TESTS = $(check_PROGRAMS)
