# Balance - Encryption Toolkit

A comprehensive C++ encryption toolkit featuring a proprietary hybrid encryption format, enterprise-grade security, and an intuitive CLI interface with chat capabilities.

## Features

### Core Encryption
- **Proprietary Balance Format:** Hybrid encryption combining XOR + AES-256 + RSA-4096
- **5 Encryption Algorithms:** Balance, AES-256, RSA-4096, Serpent, Blowfish
- **FIPS 140-2 Compliance:** Enterprise-grade cryptographic operations
- **Secure Memory Handling:** Protected key storage with automatic zeroing
- **Hardware Security Module (HSM) Support:** PKCS#11 interface for enterprise deployments

### User Interface
- **Glass-Like GUI:** Minimalistic, modern Qt-based interface with transparency effects
- **Powerful CLI:** 37+ commands with chaining, piping, and autocomplete
- **Embedded Terminal:** Integrated command-line interface in main window
- **Algorithm Selector:** Easy switching between encryption methods

### Chat System (Phase 1)
- **Local Chat:** Persistent message storage with SQLite
- **Separate Window:** Independent chat interface
- **Phase 2 Ready:** Supabase integration prepared with E2E encryption hooks

### Security Features
- **Tamper Detection:** File integrity verification with SHA-256
- **Secure Deletion:** Multi-pass file wiping (3 or 7 passes)
- **Key Management:** Encrypted keystore with master password protection
- **Audit Logging:** Comprehensive security event logging
- **Password Strength Validation:** Enforced complexity requirements

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev libssl-dev libsqlite3-dev

# macOS
brew install cmake qt@6 openssl@3 sqlite3

# Windows
# Install Visual Studio 2022, Qt 6, OpenSSL for Windows
```

### Building

```bash
# Linux/macOS
./scripts/build.sh

# Windows
scripts\build.bat

# Run
./build/balance
```

See [BUILD.md](docs/BUILD.md) for detailed build instructions.

### Basic Usage

```bash
# Encrypt a file with Balance format
balance encrypt document.pdf --password MySecurePassword123

# Decrypt a file
balance decrypt document.pdf.balance --password MySecurePassword123

# List available algorithms
balance list-algorithms

# Show help
balance help
```

## Documentation

- **[BUILD.md](docs/BUILD.md)** - Complete build instructions for all platforms
- **[CLI_REFERENCE.md](docs/CLI_REFERENCE.md)** - Full CLI command reference
- **[SECURITY.md](docs/SECURITY.md)** - Security specifications and best practices
- **[API.md](docs/API.md)** - C++ API documentation for developers

## Architecture

```
balance/
├── src/
│   ├── core/           # Encryption engine, security, storage
│   ├── cli/            # Command-line interface
│   ├── gui/            # Qt-based GUI
│   ├── chat/           # Chat system
│   └── utils/          # Logging, config, validators
├── include/            # Public API headers
├── tests/              # Comprehensive test suite
├── resources/          # QML, icons, config files
├── docs/               # Documentation
└── scripts/            # Build and utility scripts
```

## Encryption Algorithms

### Balance Format (Proprietary)
- **Hybrid encryption:** XOR + AES-256 + RSA-4096
- **Key derivation:** PBKDF2 with 100,000 iterations
- **Integrity:** SHA-256 hash verification
- **File extension:** .balance

### Standard Algorithms
- **AES-256-CBC:** FIPS-approved symmetric encryption
- **RSA-4096:** Asymmetric encryption with OAEP padding
- **Serpent-256:** AES finalist with high security margin
- **Blowfish-448:** Fast encryption for legacy compatibility

## CLI Commands

37+ commands organized into categories:

- **File Operations:** encrypt, decrypt, batch-encrypt, batch-decrypt, secure-delete
- **Key Management:** keygen, keystore, import-key, export-key
- **System:** config, status, version, diagnostics, help
- **Chat:** chat, chat-close
- **Advanced:** reencrypt, verify, inspect, benchmark

See [CLI_REFERENCE.md](docs/CLI_REFERENCE.md) for complete command documentation.

## Security

- **FIPS 140-2 Compliance:** Cryptographic operations using OpenSSL FIPS module
- **Secure Memory:** Memory locking and automatic zeroing
- **Tamper Detection:** File and application integrity verification
- **HSM Support:** PKCS#11 interface for hardware security modules
- **Audit Logging:** Comprehensive security event logging

See [SECURITY.md](docs/SECURITY.md) for detailed security information.

## Testing

```bash
# Run all tests
./scripts/run_tests.sh

# Or with CMake
cd build && ctest --output-on-failure

# Individual test executable
./build/tests/balance_tests
```

## Phase 1 vs Phase 2

### Phase 1 (Current)
- ✅ Complete local encryption toolkit
- ✅ All 5 encryption algorithms working
- ✅ Full CLI with 37+ commands
- ✅ Qt GUI with embedded terminal
- ✅ Local chat system with SQLite
- ✅ Comprehensive test suite
- ✅ Full documentation

### Phase 2 (Planned)
- 🔄 Supabase integration for chat system
- 🔄 Real-time message synchronization
- 🔄 End-to-end encrypted messaging
- 🔄 Cross-device chat sync
- 🔄 Enhanced QML interface
- 🔄 Additional 63 CLI commands (target: 100+)

## System Requirements

- **OS:** Linux (Ubuntu 22.04+), Windows 10/11, macOS 12+
- **RAM:** 4GB minimum, 8GB recommended
- **Disk:** 100MB for installation, varies for encrypted files
- **CPU:** Any modern x86_64 processor (AES-NI acceleration supported)

## Performance

- **AES-256:** ~50 MB/s on modern CPU
- **Balance Format:** ~40 MB/s (hybrid encryption overhead)
- **Streaming Support:** Files >100MB automatically streamed (4MB chunks)
- **Memory Limit:** 200MB default maximum memory usage
- **Hardware Acceleration:** AES-NI automatically utilized when available

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch
3. Follow existing code style (C++20, clang-format)
4. Add tests for new features
5. Update documentation
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Security Disclosure

For security vulnerabilities, please email security@balance-crypto.org. Do not file public issues for security problems.

## Acknowledgments

- **OpenSSL** - Cryptographic library
- **Qt** - Cross-platform GUI framework
- **SQLite** - Embedded database
- **Google Test** - Testing framework

## Contact

- **Issues:** https://github.com/balance/balance/issues
- **Documentation:** https://balance-crypto.org/docs
- **Security:** security@balance-crypto.org

---

**Built with modern C++20 | Powered by OpenSSL, Qt6, and SQLite**