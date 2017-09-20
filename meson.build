project('libadiff', 'c')

add_global_arguments('-std=gnu11', '-fms-extensions', language: 'c')

glib = dependency('glib-2.0')
sndfile = dependency('sndfile')

adiff_inc = include_directories('include/')

# rabin

rabin_sources = ['src/rabin.c']

test_rabin = executable(
    'unittest_rabin',
    rabin_sources + ['tests/unittest_rabin.c'],
    include_directories: adiff_inc,
    dependencies: [glib])
test('rabin', test_rabin)

# bdiff

bdiff_sources = rabin_sources + [
    'src/hash_counting_table.c',
    'src/narrowing.c',
    'src/chunk.c',
    'src/hunk.c',
    'src/bdiff.c']

internal_headers = include_directories('src/')

test_bdiff = executable(
    'unittest_bdiff',
    bdiff_sources + [
	'tests/fake_fetcher.c',
	'tests/narrowable_test_tools.c',
	'tests/unittest_hunk.c',
	'tests/unittest_hash_counting_table.c',
	'tests/unittest_chunk.c',
	'tests/unittest_narrowing.c',
	'tests/unittest_bdiff.c'
    ],
    include_directories: [adiff_inc, internal_headers],
    dependencies: [glib])
test('bdiff', test_bdiff)

# adiff

adiff_sources = ['src/adiff.c'] + bdiff_sources

adiff = shared_library(
    'adiff', adiff_sources, include_directories: adiff_inc,
    dependencies: [glib, sndfile])

test_adiff = executable(
    'unittest_adiff',
    ['tests/fake_fetcher.c', 'tests/unittest_adiff.c'],
    link_with: adiff,
    dependencies: [glib, sndfile])
test('adiff', test_adiff)

# Examples

executable(
    'diff_frames',
    ['examples/diff_frames.c'],
    link_with: adiff)

executable(
    'patch_frames',
    ['examples/patch_frames.c'],
    link_with: adiff)
