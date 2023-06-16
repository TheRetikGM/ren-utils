all: doc lib

doc:
	doxygen

lib: meson.build
	test -d build || meson setup build
	cd build && ninja -j 32

clean:
	rm -rf build doc

test: lib
	cd build && ninja && ./bb_tests

.PHONY: doc clean

