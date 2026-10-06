# Build guide

## macOS

```bash
brew install cmake openssl@3 curl ncurses
cmake -S . -B build \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix curl);$(brew --prefix ncurses)"
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/balance
```

No separate `tinfo` library is required on macOS.

## Arch Linux

```bash
sudo pacman -S --needed base-devel cmake openssl curl ncurses
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/balance
```

## Strict build

```bash
cmake -S . -B build-werror -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-Werror'
cmake --build build-werror -j
ctest --test-dir build-werror --output-on-failure
```

## Sanitizers

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1'
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ctest --test-dir build-asan --output-on-failure
```

## Version checks

`CMakeLists.txt` refuses to configure if `VERSION` does not match the CMake project version. Releases use the `v0.3.1-beta` Git tag for `VERSION=beta-0.3.1`.
