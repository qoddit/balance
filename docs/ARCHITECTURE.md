# Architecture

`balance` is split into five major layers:

- `crypto.c`: OpenSSL 3 primitives, key derivation, AEAD, signatures, X25519, hashing.
- `store.c`: encrypted local vault/history and filesystem permissions.
- `proto.c`: deterministic frame encoding, AEAD framing, sequence enforcement support.
- `net.c`: TCP transport, authenticated peer establishment, tunnel requests, chat, friend requests, and file streaming.
- `ui.c`: curses-only rendering/input. Network threads enqueue events; only the curses/main thread touches curses windows.
- `commands.c`: single command registry used by parsing, help, aliases, completion, and tests; command behavior remains in the UI/application layer.
- `cloud.c`: optional encrypted-history blob backend.
- `server.c`: minimal self-hosted encrypted blob storage service.

The volatile path is entirely P2P after startup: there is no cloud rendezvous service. Friend endpoints are stored in the encrypted vault and are used for direct connections.
