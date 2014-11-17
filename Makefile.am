bin_PROGRAMS=diff
diff_CFLAGS=$(GLIB_CFLAGS)
diff_SOURCES= \
	src/block.c \
	src/chunk.c \
	src/diff.c \
	src/diff_hunk.c \
	src/hash_multiset.c
diff_LDADD=$(GLIB_LIBS)

check_PROGRAMS = unittest_multiset
unittest_multiset_CFLAGS=$(GLIB_CFLAGS) -Isrc
unittest_multiset_SOURCES = \
	tests/unittest_multiset.c \
	src/hash_multiset.c
unittest_multiset_LDADD=$(GLIB_LIBS)

TESTS = $(check_PROGRAMS)
