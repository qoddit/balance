# security model

## identity and authentication

each account has an Ed25519 identity key. the private key is stored only inside the encrypted local vault. the login password is not persisted as plaintext. A Scrypt-derived master key decrypts the vault.

incoming tunnel requests are accepted only when the sender's Ed25519 public key matches a previously pinned friend identity. A friend request can introduce an untrusted key; accepting that request pins the key locally.

## session establishment

peers exchange signed HELLO messages containing username, long-term Ed25519 public key, and an ephemeral X25519 public key. the signature authenticates the key-exchange transcript. the shared X25519 secret is expanded with HKDF-SHA256 into directional session material.

application frames are protected by ChaCha20-Poly1305. every frame carries its sequence number as authenticated data and the receiver requires the exact next sequence number. a separate random 96-bit AEAD nonce is carried in each encrypted frame. this prevents nonce reuse and rejects simple replay/reordering.

## local storage

the vault and persisted history use authenticated encryption. the files are created with restrictive permissions (`0600`) and their parent application directory is created with `0700` permissions. chat history is written only when persistence is explicitly enabled; volatile chat transcripts are held in memory and cleared on tunnel disconnect.

## file transfers

file transfers are separate protocol requests. the recipient must accept before file chunks are sent. the receiver writes to a `.part` file under `transfers/<chat>`, enforces exact chunk offsets, validates final size and SHA-256 digest, and finalizes with a non-overwriting hard-link operation.

## cloud

cloud persistence uploads the encrypted local history file, not plaintext messages. the server sees an object identifier, ciphertext, request metadata and network metadata, but not the chat plaintext or vault key. remote cloud endpoints are restricted to HTTPS.

## threats not eliminated

application-layer encryption cannot hide IP addresses, network timing, packet sizes, or the fact that two peers connected. The bundled self-hosted server is a minimal HTTP blob service and must be deployed behind TLS and normal network hardening for remote use.

no cryptographic implementation should be considered independently production-audited solely because the code compiles and tests pass. This repository should receive an external security review before high-risk deployment.
