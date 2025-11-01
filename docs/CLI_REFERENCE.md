# Balance - CLI Command Reference

## Overview

Balance provides a comprehensive command-line interface with 37+ commands for encryption, key management, file operations, and system configuration.

## Command Syntax

```
command [arguments] [--flag value] [--flag]
```

### Special Features
- **Command chaining:** Use `&&` to chain commands
- **Output redirection:** Use `>` to save output to file
- **Piping:** Use `|` to pipe output between commands
- **Variables:** Use `$var` syntax for variable substitution
- **Tab completion:** Press Tab to autocomplete commands and paths
- **History:** Use Up/Down arrows to navigate command history

## File Encryption Commands

### encrypt
Encrypt a single file with specified algorithm.

**Syntax:**
```
encrypt <file> [--algorithm <algo>] [--output <path>] [--password <pwd>]
```

**Examples:**
```bash
encrypt document.pdf
encrypt secret.txt --algorithm aes256 --output encrypted.bin
encrypt data.json --password MySecurePass123
```

### decrypt
Decrypt an encrypted file.

**Syntax:**
```
decrypt <file> [--password <pwd>] [--output <path>]
```

**Examples:**
```bash
decrypt document.pdf.balance
decrypt encrypted.bin --password MySecurePass123 --output decrypted.txt
```

### batch-encrypt
Encrypt multiple files matching a pattern.

**Syntax:**
```
batch-encrypt <pattern> [--algorithm <algo>] [--recursive]
```

**Examples:**
```bash
batch-encrypt "*.txt"
batch-encrypt "documents/*.pdf" --algorithm serpent --recursive
```

### batch-decrypt
Decrypt multiple encrypted files.

**Syntax:**
```
batch-decrypt <pattern> [--recursive]
```

**Examples:**
```bash
batch-decrypt "*.balance"
batch-decrypt "encrypted/**/*.balance" --recursive
```

## Encryption Management Commands

### list-algorithms
Display all available encryption algorithms with details.

**Syntax:**
```
list-algorithms
```

**Output:**
```
Available encryption algorithms:

  Balance
    Description: Proprietary hybrid encryption (XOR + AES-256 + RSA-4096)
    Key size: 256 bits
    FIPS approved: Yes

  AES-256-CBC
    Description: Advanced Encryption Standard with 256-bit key
    ...
```

### set-algorithm
Set the default encryption algorithm for subsequent operations.

**Syntax:**
```
set-algorithm <algorithm>
```

**Examples:**
```bash
set-algorithm balance
set-algorithm aes256
```

### algorithm-info
Show detailed information about a specific algorithm.

**Syntax:**
```
algorithm-info <algorithm>
```

**Example:**
```bash
algorithm-info balance
```

### benchmark
Benchmark encryption performance.

**Syntax:**
```
benchmark [--algorithm <algo>] [--size <bytes>]
```

**Examples:**
```bash
benchmark
benchmark --algorithm aes256 --size 10485760
```

## Key Management Commands

### keygen
Generate a new encryption key.

**Syntax:**
```
keygen [--type <rsa|aes>] [--size <bits>] [--identifier <name>]
```

**Examples:**
```bash
keygen --type aes --identifier my-key
keygen --type rsa --size 4096 --identifier my-rsa-key
```

### keystore
Manage stored encryption keys.

**Syntax:**
```
keystore <command> [args]
```

**Subcommands:**
- `list` - List all stored keys
- `add <file>` - Add key from file
- `remove <id>` - Remove key by identifier
- `export <id> <file>` - Export key to file

**Examples:**
```bash
keystore list
keystore add my-key.pem
keystore remove old-key
keystore export my-key exported-key.pem
```

### change-password
Change the master password for encrypted keystore.

**Syntax:**
```
change-password
```

## File Operations Commands

### list
List files with encryption status indicator.

**Syntax:**
```
list [pattern] [--recursive]
```

**Examples:**
```bash
list
list "*.balance"
list --recursive
```

### info
Display detailed file information.

**Syntax:**
```
info <file>
```

**Example:**
```bash
info document.pdf.balance
```

### search
Search for files by name or extension.

**Syntax:**
```
search <pattern> [--path <directory>]
```

**Examples:**
```bash
search "*.txt"
search "document" --path /data
```

### wipe
Securely delete file with multiple overwrite passes.

**Syntax:**
```
wipe <file> [--passes <3|7>]
```

**Examples:**
```bash
wipe sensitive.txt
wipe secret.doc --passes 7
```

### secure-delete
Encrypt file then securely delete original.

**Syntax:**
```
secure-delete <file>
```

## System & Configuration Commands

### config
Get or set configuration values.

**Syntax:**
```
config <key> [value]
```

**Examples:**
```bash
config default_algorithm
config default_algorithm balance
config enable_fips true
```

### history
Show or clear command history.

**Syntax:**
```
history [--clear] [--limit <n>]
```

**Examples:**
```bash
history
history --limit 20
history --clear
```

### status
Show system status and statistics.

**Syntax:**
```
status
```

**Output:**
```
Balance Status:
  Encryption engines: 5
  CLI commands: 37
  Stored keys: 3
  FIPS mode: Disabled
  System: Ready
```

### version
Display version and build information.

**Syntax:**
```
version
```

### diagnostics
Run system diagnostics and show report.

**Syntax:**
```
diagnostics [--verbose]
```

## Chat Commands

### chat
Open the chat window.

**Syntax:**
```
chat [--new-window]
```

### chat-close
Close the chat window.

**Syntax:**
```
chat-close
```

## Help & Information Commands

### help
Show help for all commands or a specific command.

**Syntax:**
```
help [command]
```

**Examples:**
```bash
help
help encrypt
help keygen
```

## Advanced Usage

### Command Chaining
Execute multiple commands sequentially:

```bash
encrypt file1.txt && encrypt file2.txt && list "*.balance"
```

### Output Redirection
Save command output to file:

```bash
list > file-list.txt
status > system-status.txt
```

### Piping
Pass output from one command to another:

```bash
list | search ".balance"
```

### Variables
Use variables for repeated values:

```bash
set output_dir=/encrypted
encrypt file1.txt --output $output_dir/file1.balance
encrypt file2.txt --output $output_dir/file2.balance
```

## Error Codes

- **0:** Success
- **1:** General error
- **2:** Invalid usage/syntax
- **-1:** Command-specific error (check error message)

## Exit Codes

All commands return appropriate exit codes for script integration:

```bash
if balance encrypt file.txt; then
    echo "Encryption successful"
else
    echo "Encryption failed"
fi
```

## Best Practices

1. **Always use strong passwords** (minimum 12 characters)
2. **Verify encrypted files** using `verify` command
3. **Backup keys** before removing from keystore
4. **Use `secure-delete`** for sensitive data
5. **Enable FIPS mode** for compliance requirements
6. **Test decryption** after encryption to ensure data integrity

## See Also

- [API Documentation](API.md)
- [Security Documentation](SECURITY.md)
- [Build Instructions](BUILD.md)
