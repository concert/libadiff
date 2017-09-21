libadiff - Audio diffing library
================================

[![Build Status](https://travis-ci.org/concert/libadiff.svg?branch=master)](
    https://travis-ci.org/concert/libadiff)

## Build:

To build you'll need:
 * [`ninja`](https://ninja-build.org/) (build system)
 * [`meson`](http://mesonbuild.com/Quick-guide.html) (build system)
 * [`glib2`](https://developer.gnome.org/glib/) with headers
 * [`libsndfile`](http://www.mega-nerd.com/libsndfile/) with headers

If you're on a Debian-based Linux system, you can install them with:
```
apt-get install ninja-build libglib2.0-dev libsndfile-dev
pip install meson  # Python3
```
Then you can build with:
```
mkdir build
meson build
ninja -C build
```
Run the tests with:
```
ninja -C build test
```
