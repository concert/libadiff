libadiff - Audio diffing library
================================

[![Build Status](https://travis-ci.org/concert/libadiff.svg?branch=master)](
    https://travis-ci.org/concert/libadiff)

## Build:
If you're on a Debian-based Linux system, prerequisites are:
```
apt-get install libglib2.0-dev libsndfile-dev
pip install meson ninja  # Python3
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
