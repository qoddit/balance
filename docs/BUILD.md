# Build and development guide

This guide covers supported native builds, dependency packages, reproducible CMake setup, testing, troubleshooting, and the Windows route. Run commands from the repository root, the directory containing `CMakeLists.txt`.

## Support matrix

| Environment | Status | Notes |
|---|---|---|
| macOS, Apple Silicon and Intel | Supported target | Apple Clang/Xcode Command Line Tools, Homebrew dependencies |
| Arch Linux and derivatives | Supported target | `base-devel`, CMake and development libraries |
| Debian / Ubuntu / Linux Mint | Supported target | `build-essential` and `-dev` packages |
| Fedora / RHEL / Rocky / AlmaLinux | Supported target | GCC toolchain and `-devel` packages |
| openSUSE | Supported target | GCC toolchain and `-devel` packages |
| Alpine Linux | Expected to work; verify locally | musl toolchain and `-dev` packages; ncurses/linker differences may require adjustment |
| Other POSIX Linux distributions | Expected to work if dependencies are available | Use distro equivalents and review CMake detection output |
| Windows native (MSVC/MinGW) | Not currently supported | POSIX sockets/curses assumptions; use WSL2 |
| Windows with WSL2 | Supported through Linux build | Build inside the WSL distribution, not as a native `.exe` |

“Supported target” means the project is intended to build there; CI coverage may be narrower than this table. Always run the tests on the machine and distribution you plan to use.

## Required dependencies

- C17-capable C compiler (`clang` or `gcc`)
- CMake 3.20 or newer
- OpenSSL 3.x development headers and `libcrypto`
- libcurl development headers and library
- curses/ncurses development headers and library
- POSIX threads (provided by the platform C runtime/toolchain)
- Git, if cloning the repository

The executable links to OpenSSL Crypto, libcurl, curses/ncurses, threads, and the platform math library. Runtime-only packages are not enough: install the development/header packages too.

## macOS — Apple Silicon and Intel

### 1. Install Apple's compiler tools

```bash
xcode-select --install
```

If they are already installed, macOS may report that no installation is required. Verify:

```bash
clang --version
cmake --version
```

Install Homebrew from its official site if it is not already installed, then:

```bash
brew update
brew install cmake openssl@3 curl ncurses
```

### 2. Configure CMake with explicit Homebrew paths

```bash
OPENSSL_PREFIX="$(brew --prefix openssl@3)"
CURL_PREFIX="$(brew --prefix curl)"
NCURSES_PREFIX="$(brew --prefix ncurses)"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR="$OPENSSL_PREFIX" \
  -DOPENSSL_INCLUDE_DIR="$OPENSSL_PREFIX/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_PREFIX/lib/libcrypto.dylib" \
  -DCMAKE_PREFIX_PATH="$CURL_PREFIX;$NCURSES_PREFIX"
```

If CMake cannot find OpenSSL, inspect the prefix and library files:

```bash
brew --prefix openssl@3
ls "$(brew --prefix openssl@3)/include/openssl/evp.h"
ls "$(brew --prefix openssl@3)/lib/"libcrypto.*
```

If the library is named differently, set `OPENSSL_CRYPTO_LIBRARY` to the actual file. Do not copy a Linux `build/` directory or executable to macOS: CMake caches compiler, SDK, architecture, and dependency paths.

### 3. Build, test, run

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

The output binary should be a Mach-O executable. Check with `file build/balance` if you see `exec format error`.

## Arch Linux and Arch-based distributions

```bash
sudo pacman -Syu --needed base-devel cmake openssl curl ncurses
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

For an Arch-derived distribution, use its documented equivalent if package names or repositories differ.

## Debian, Ubuntu, Linux Mint, Pop!_OS

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libssl-dev libcurl4-openssl-dev libncurses-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

Check that `libssl-dev` supplies OpenSSL 3.x. Older Debian/Ubuntu releases may ship an OpenSSL version below the project's CMake requirement; use a supported release or install OpenSSL 3 from a trusted distribution-supported source rather than disabling the version check.

## Fedora, RHEL, Rocky Linux, AlmaLinux

On Fedora and recent RHEL-compatible distributions:

```bash
sudo dnf install -y gcc gcc-c++ make cmake openssl-devel libcurl-devel ncurses-devel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

On older systems that use `yum`, substitute `yum` for `dnf`. Verify the OpenSSL package is version 3 or newer; older enterprise releases may need a newer supported platform or an isolated toolchain.

## openSUSE Leap / Tumbleweed

```bash
sudo zypper refresh
sudo zypper install -y gcc gcc-c++ make cmake libopenssl-devel libcurl-devel ncurses-devel
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

Package names can vary by openSUSE release; use `zypper search -s openssl-devel libcurl-devel ncurses-devel` if a package name is unavailable.

## Alpine Linux

Alpine uses musl rather than glibc, so treat this as a portability build that must be tested on the target Alpine release:

```sh
sudo apk add build-base cmake linux-headers openssl-dev curl-dev ncurses-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

If CMake cannot locate ncurses or a transitive terminal library, inspect the configure output and installed development packages. Do not hard-code a library path copied from another distro; prefer package-provided CMake/pkg-config metadata.

## Other Linux distributions

Use the package manager to install the required dependencies listed above. Typical package naming conventions are:

- Debian family: `*-dev`
- Fedora/RHEL/openSUSE: `*-devel`
- Arch: package names generally omit the suffix
- Alpine: `*-dev`

Then run the standard configure/build/test commands. If CMake fails, read the **first** error from configuration: a later `No rule to make target` usually means configuration did not complete, so no build system was generated.

## Windows through WSL2

Native Windows is not a supported target. Install WSL2 and a supported Linux distribution using Microsoft's current WSL instructions. Open a shell in that distribution, install its Linux build dependencies using the section above, and clone/build the repository from inside WSL. Prefer the WSL home directory (for example `~/src/balance`) rather than `/mnt/c/...` for better filesystem behavior.

Example in Ubuntu under WSL2:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libssl-dev libcurl4-openssl-dev libncurses-dev git
mkdir -p ~/src
cd ~/src
git clone https://github.com/qoddit/balance.git
cd balance
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

These commands run a Linux ELF executable inside WSL. They do not produce a native Windows executable. WSL terminal mouse/trackpad behavior depends on the terminal application and its mouse-event support.

## Reproducible clean build

If you switch operating systems, architectures, compilers, SDKs, or dependency locations, remove the CMake build directory before reconfiguring:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Never commit generated build directories or binaries to the source repository.

## Strict warnings build

```bash
cmake -S . -B build-werror \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-Werror"
cmake --build build-werror --parallel
ctest --test-dir build-werror --output-on-failure
```

A strict build can fail on compiler-version-specific warnings; fix real warnings rather than globally suppressing new diagnostics.

## Address and undefined-behavior sanitizers

Supported Clang/GCC environments:

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1"
cmake --build build-asan --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ctest --test-dir build-asan --output-on-failure
```

Sanitizer support and leak detection vary by operating system and compiler. If unsupported on a platform, run the regular tests and document the limitation.

## Troubleshooting

### `Could NOT find OpenSSL` or OpenSSL version is too old

Install the OpenSSL **development** package, verify it is version 3 or newer, and pass `OPENSSL_ROOT_DIR` / the actual `libcrypto` path where needed. On macOS, use `brew --prefix openssl@3` and the explicit flags above.

### `No rule to make target` after CMake errors

CMake configuration failed, so no build files were generated. Resolve the earliest CMake error first; then rerun configure and build.

### `exec format error`

The executable is for another OS or CPU architecture, or is otherwise not executable in the current environment. Remove `build/`, configure and compile on the target OS, and inspect the result with `file build/balance`.

### `curses.h` / `ncurses.h` missing

Install the curses development package (`libncurses-dev`, `ncurses-devel`, `ncurses`, or `ncurses-dev`, depending on the OS). Ensure headers and libraries are for the same target architecture.

### Terminal rendering or mouse-wheel differences

Use a modern terminal with UTF-8 and color support. Mouse-wheel/trackpad events depend on the terminal emulator and may not be exposed identically across platforms; keyboard scrolling is the fallback. Resize the terminal to at least a practical interactive width and use `Ctrl-L`/redraw if rendering becomes corrupted.

### `curl` found but CMake cannot link it

Install the development package and, on macOS, add Homebrew's curl prefix to `CMAKE_PREFIX_PATH`. Do not mix libraries from different architectures.

## Install targets

CMake defines `balance` and `balance-server` runtime targets. To install them to a custom prefix:

```bash
cmake --install build --prefix "$HOME/.local"
```

Ensure `$HOME/.local/bin` is on `PATH` if you want to run the installed programs by name. This does not install runtime configuration or migrate existing local account data.
