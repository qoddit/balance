# Balance - Build Instructions

## Requirements

### System Requirements
- **Operating System:** Linux, Windows, or macOS
- **Compiler:** GCC 11+, Clang 13+, or MSVC 2022+
- **CMake:** Version 3.20 or higher
- **C++ Standard:** C++20

### Dependencies

#### Required Libraries
- **Qt 6:** Qt6 Core, Widgets, Qml, Quick (6.5 or higher)
- **OpenSSL:** Version 3.0 or higher
- **SQLite3:** Version 3.35 or higher

#### Optional Dependencies
- **Google Test:** Downloaded automatically via FetchContent
- **PKCS#11 Library:** For HSM support (Phase 2)

## Installation

### Ubuntu/Debian

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake git

# Install dependencies
sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev
sudo apt install libssl-dev libsqlite3-dev

# Verify Qt installation
qmake6 --version
```

### macOS

```bash
# Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake qt@6 openssl@3 sqlite3

# Add Qt to PATH
export PATH="/usr/local/opt/qt@6/bin:$PATH"
```

### Windows

1. Install Visual Studio 2022 with C++ development tools
2. Download and install [Qt 6](https://www.qt.io/download)
3. Download and install [OpenSSL for Windows](https://slproweb.com/products/Win32OpenSSL.html)
4. SQLite is usually included with Windows SDK

## Building

### Linux/macOS

```bash
# Clone repository
cd balance

# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run application
./balance
```

### Windows

```cmd
# Open Visual Studio Developer Command Prompt
cd balance

# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Run application
.\Release\balance.exe
```

## Building with Tests

```bash
# Configure with tests
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Or run test executable directly
./tests/balance_tests
```

## CMake Options

```bash
# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release

# Specify Qt location (if not in standard paths)
cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt/6.5.0/gcc_64"

# Enable FIPS mode (requires OpenSSL with FIPS module)
cmake .. -DENABLE_FIPS=ON
```

## Troubleshooting

### Qt Not Found

```bash
# Set Qt path manually
export CMAKE_PREFIX_PATH="/usr/local/Qt-6.5.0"
cmake ..
```

### OpenSSL Version Issues

```bash
# Check OpenSSL version
openssl version

# Use specific OpenSSL installation
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/ssl
```

### SQLite Not Found

```bash
# Ubuntu/Debian
sudo apt install libsqlite3-dev

# macOS
brew install sqlite3
```

## Installation

```bash
# Install to system (requires root/admin)
sudo make install

# Or install to custom location
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/balance
make install
```

## Cross-Platform Notes

### Linux
- Tested on Ubuntu 22.04, Debian 12, Fedora 38
- Use system package manager for dependencies
- Hardware acceleration (AES-NI) enabled by default

### macOS
- Tested on macOS 12 (Monterey) and later
- Use Homebrew for dependency management
- Code signing may be required for distribution

### Windows
- Tested on Windows 10/11
- Use Visual Studio 2022 or later
- OpenSSL DLLs must be in PATH or application directory

## Development Build

```bash
# Debug build with all warnings
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"

# With address sanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=address"

# Generate compile_commands.json for IDEs
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## Performance Optimization

```bash
# Maximum optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Link-time optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

## Verification

After building, verify the installation:

```bash
# Check version
./balance --version

# Run self-tests
./balance diagnostics

# Test encryption
echo "Hello, World!" > test.txt
./balance encrypt test.txt --algorithm balance --password testpass
./balance decrypt test.txt.balance --password testpass
```

## Getting Help

- **Documentation:** See docs/ directory
- **Issues:** https://github.com/balance/balance/issues
- **Build problems:** Check CMake output for specific errors
