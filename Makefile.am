bin_PROGRAMS = diff_frames patch_frames

diff_frames_CFLAGS = $(GLIB_CFLAGS)
diff_frames_SOURCES = \
	examples/diff_frames.c \
	src/adiff.c \
	src/chunk.c \
	src/bdiff.c \
	src/hunk.c \
	src/rabin.c \
	src/hash_counting_table.c
diff_frames_LDADD=$(GLIB_LIBS) $(SNDFILE_LIBS)

patch_frames_CFLAGS = $(GLIB_CFLAGS)
patch_frames_SOURCES = \
	examples/patch_frames.c \
	src/adiff.c \
	src/chunk.c \
	src/bdiff.c \
	src/hunk.c \
	src/rabin.c \
	src/hash_counting_table.c
patch_frames_LDADD=$(GLIB_LIBS) $(SNDFILE_LIBS)

check_PROGRAMS = unittest_rabin unittest_bdiff

unittest_rabin_CFLAGS=$(GLIB_CFLAGS) -Iinclude
unittest_rabin_SOURCES = \
	tests/unittest_rabin.c \
	src/rabin.c
unittest_rabin_LDADD=$(GLIB_LIBS)

unittest_bdiff_CFLAGS=$(GLIB_CFLAGS) -Isrc
unittest_bdiff_SOURCES = \
	tests/fake_fetcher.c \
	tests/unittest_hunk.c \
	src/hunk.c \
	tests/unittest_hash_counting_table.c \
	src/hash_counting_table.c \
	tests/unittest_chunk.c \
	src/chunk.c \
	src/rabin.c \
	src/bdiff.c \
	tests/unittest_bdiff.c
unittest_bdiff_LDADD=$(GLIB_LIBS)

TESTS = $(check_PROGRAMS)
