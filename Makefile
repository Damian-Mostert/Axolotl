.PHONY: all build install uninstall clean test extensions update-extensions

all: build

build:
	@cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc)

install:
	@./install.sh

uninstall:
	@sudo ./uninstall.sh

clean:
	@rm -rf build

test: build
	@./build/compiler tests/all_tests.axo

extensions:
	@./build_extensions.sh

update-extensions:
	@./update_extensions.sh
