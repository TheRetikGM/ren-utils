all: doc ren_utils

doc: Doxyfile
	doxygen

lib: meson.build
	test -d build || meson setup build
	cd build && ninja -j 32

clean:
	rm -rf build doc
