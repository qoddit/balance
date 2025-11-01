# Balance - Security Documentation

## Overview

Balance implements enterprise-grade security with FIPS 140-2 compliance, secure memory handling, tamper detection, and hardware security module (HSM) support.

## Cryptographic Specifications

### Supported Algorithms

#### Balance Format (Proprietary)
- **Type:** Hybrid encryption
- **Components:** XOR + AES-256-CBC + RSA-4096
- **Key Size:** 256-bit (AES component)
- **Block Size:** 16 bytes (AES block size)
- **FIPS Status:** Compliant (AES and RSA components are FIPS-approved)
- **Use Case:** Primary encryption format for maximum security

**Security Features:**
- Double encryption layer (XOR + AES)
- RSA key encryption for secure key storage
- PBKDF2 key derivation (100,000 iterations)
- SHA-256 integrity verification
- Salt randomization (prevents rainbow tables)

#### AES-256-CBC
- **Standard:** FIPS 197
- **Key Size:** 256 bits
- **Block Size:** 128 bits
- **Mode:** CBC with random IV
- **FIPS Status:** Approved
- **Use Case:** Fast, secure symmetric encryption

#### RSA-4096
- **Standard:** FIPS 186-4
- **Key Size:** 4096 bits
- **Padding:** OAEP with SHA-256
- **FIPS Status:** Approved
- **Use Case:** Asymmetric encryption, key exchange

#### Serpent-256
- **Type:** AES finalist
- **Key Size:** 256 bits
- **Block Size:** 128 bits
- **FIPS Status:** Not approved (but highly secure)
- **Use Case:** High security margin, paranoid mode

#### Blowfish-448
- **Key Size:** Up to 448 bits
- **Block Size:** 64 bits
- **FIPS Status:** Not approved
- **Use Case:** Legacy compatibility, fast encryption

## FIPS 140-2 Compliance

### Enabling FIPS Mode

```bash
# In configuration file
enable_fips = true

# Or via command
config enable_fips true
```

### FIPS-Approved Operations
- AES-256 encryption/decryption
- RSA-4096 key generation and operations
- SHA-256 hashing
- PBKDF2 key derivation
- DRBG (Deterministic Random Bit Generator)

### Self-Tests
Balance performs cryptographic self-tests on startup when FIPS mode is enabled:
- Known Answer Tests (KAT) for all algorithms
- Continuous Random Number Generator Test
- Software integrity verification

### Audit Logging
All cryptographic operations are logged when audit mode is enabled:
- Operation type (encrypt/decrypt/sign/verify)
- Algorithm used
- Key identifier
- Timestamp
- Success/failure status

## Secure Memory Handling

### Implementation
- **OpenSSL Secure Memory:** Uses `OPENSSL_secure_malloc` for key storage
- **Memory Locking:** Prevents swapping sensitive data to disk
- **Automatic Zeroing:** All sensitive memory is zeroed before deallocation
- **RAII Pattern:** Automatic cleanup using C++ destructors

### Protected Data
- Encryption keys (all types)
- User passwords
- Decrypted plaintext (temporary)
- Key derivation salts
- Session tokens

### Usage Example
```cpp
// SecureString automatically zeros memory on destruction
SecureString password("MySecurePassword");
// password memory is locked and will be zeroed when out of scope

// SecureBuffer for binary data
SecureBuffer keyData(32);  // 256-bit key
// keyData memory is locked and zeroed on destruction
```

## Tamper Detection

### File Integrity
- **SHA-256 Hashing:** All encrypted files include integrity hash
- **Header Protection:** Magic numbers and version verification
- **Corruption Detection:** Automatic detection during decryption
- **Modification Alerts:** User notified of any tampering attempts

### Application Integrity
- **Binary Verification:** Self-check on startup
- **Debugger Detection:** Warns if debugger is attached
- **Configuration Checksums:** Config files protected against modification

### Response to Tampering
- Log security event with severity level
- Display warning to user
- Refuse operation if integrity compromised
- Optional: Lock application after multiple failures

## Hardware Security Module (HSM) Support

### PKCS#11 Interface
Balance supports HSM integration via PKCS#11 standard:

```cpp
// Example HSM configuration
HSMKeyStore hsm;
hsm.connect("/usr/lib/libpkcs11.so", "PIN");
```

### HSM Operations
- RSA key generation in HSM
- Private key never leaves HSM
- Hardware-backed random number generation
- Secure key backup and recovery

### Supported HSM Devices
- Any PKCS#11-compliant HSM
- YubiHSM
- SafeNet Luna
- Thales nShield
- AWS CloudHSM

### Configuration
```json
{
  "keystore": {
    "type": "hsm",
    "pkcs11_library": "/usr/lib/libpkcs11.so",
    "slot": 0,
    "pin_source": "env:HSM_PIN"
  }
}
```

## Password Security

### Requirements
- **Minimum Length:** 12 characters
- **Complexity:** At least 3 of:
  - Uppercase letters
  - Lowercase letters
  - Digits
  - Special characters

### Key Derivation
- **Algorithm:** PBKDF2-HMAC-SHA256
- **Iterations:** 100,000 (minimum)
- **Salt:** 128-bit random salt per password
- **Output:** 256-bit derived key

### Password Strength Meter
```bash
# Check password strength
balance validate-password "MyPassword123!"
# Returns: Strength: 75/100 (Good)
```

## Threat Model

### Protected Against
- **Brute Force:** High iteration count for key derivation
- **Rainbow Tables:** Unique salt per encryption
- **Known-Plaintext:** Strong encryption algorithms
- **Chosen-Plaintext:** Secure modes (CBC, OAEP)
- **Side-Channel:** Constant-time operations where possible
- **Memory Dumps:** Secure memory locking and zeroing
- **File Tampering:** Integrity verification with SHA-256
- **Replay Attacks:** Timestamps and unique IVs

### NOT Protected Against
- **Quantum Computers:** RSA-4096 vulnerable (use Serpent for quantum resistance)
- **Weak Passwords:** User must choose strong passwords
- **Malware:** Cannot protect against compromised system
- **Physical Access:** Full disk encryption recommended
- **Social Engineering:** User education required

## Security Best Practices

### For Users
1. **Use strong, unique passwords** for each encrypted file
2. **Store keys securely** in encrypted keystore
3. **Verify file integrity** after encryption
4. **Use `secure-delete`** for original sensitive files
5. **Keep software updated** for security patches
6. **Enable FIPS mode** for compliance requirements
7. **Use HSM** for high-value keys in enterprise

### For Developers
1. **Never log sensitive data** (keys, passwords, plaintext)
2. **Use SecureMemory classes** for all sensitive data
3. **Zero memory** after use
4. **Validate all inputs** before cryptographic operations
5. **Handle errors securely** (don't leak information)
6. **Use constant-time comparisons** for secrets
7. **Follow secure coding guidelines** (CERT, OWASP)

## Compliance

### Standards
- **FIPS 140-2:** Level 1 (software cryptographic module)
- **NIST SP 800-132:** Password-based key derivation
- **NIST SP 800-38A:** Block cipher modes
- **FIPS 186-4:** Digital signature standard (RSA)
- **FIPS 197:** Advanced encryption standard

### Certifications
- Balance is designed for FIPS 140-2 compliance
- OpenSSL FIPS module provides certified cryptography
- Full certification requires independent validation

## Vulnerability Reporting

### Responsible Disclosure
If you discover a security vulnerability:

1. **DO NOT** publicly disclose the vulnerability
2. Email security details to: security@balance-crypto.org
3. Include:
   - Detailed description
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)
4. Allow 90 days for patch before public disclosure

### Security Updates
- Critical security patches released immediately
- Security updates announced via security mailing list
- CVE numbers assigned for tracked vulnerabilities

## Audit Trail

### Logged Events
When audit logging is enabled:
- All encryption/decryption operations
- Key generation and deletion
- Password changes
- FIPS self-test results
- Tamper detection events
- Configuration changes

### Log Format
```
[2024-01-15 14:23:45.123] [SECURITY] [INFO] Encrypted file: document.pdf with algorithm: balance
[2024-01-15 14:23:50.456] [SECURITY] [WARNING] Tamper detected: invalid file header
[2024-01-15 14:24:00.789] [SECURITY] [CRITICAL] Multiple failed decryption attempts
```

### Log Protection
- Logs written to tamper-evident storage
- SHA-256 hash chain for integrity
- Rotation and archival for long-term storage

## Additional Resources

- [NIST Cryptographic Standards](https://csrc.nist.gov/)
- [OpenSSL FIPS Module](https://www.openssl.org/docs/fips.html)
- [OWASP Cryptographic Storage Cheat Sheet](https://cheatsheetseries.owasp.org/)

## Security Contact

For security-related inquiries:
- Email: security@balance-crypto.org
- PGP Key: Available at https://balance-crypto.org/security.asc
