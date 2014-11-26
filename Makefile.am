bin_PROGRAMS = bdiff
bdiff_CFLAGS = $(GLIB_CFLAGS)
bdiff_SOURCES = \
	src/block.c \
	src/chunk.c \
	src/bdiff.c \
	src/hunk.c \
	src/rabin.c \
	src/hash_counting_table.c
bdiff_LDADD=$(GLIB_LIBS)

check_PROGRAMS = unittest_hash_counting_table unittest_rabin

unittest_rabin_CFLAGS=$(GLIB_CFLAGS) -Iinclude
unittest_rabin_SOURCES = \
	tests/unittest_rabin.c \
	src/rabin.c
unittest_rabin_LDADD=$(GLIB_LIBS)

unittest_hash_counting_table_CFLAGS=$(GLIB_CFLAGS) -Isrc
unittest_hash_counting_table_SOURCES = \
	tests/unittest_hash_counting_table.c \
	src/hash_counting_table.c
unittest_hash_counting_table_LDADD=$(GLIB_LIBS)

TESTS = $(check_PROGRAMS)
