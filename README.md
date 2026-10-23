# balance

**current release:** `beta-0.3.3`
`balance` is a curses-based CLI messenger built around a local-first security model. the default chat mode is **volatile peer-to-peer**: messages are encrypted before transmission, no cloud service is contacted, and the volatile transcript is discarded when the tunnel closes. cloud persistence is an explicit opt-in.

visit or download the [balance vault](https://github.com/qoddit/balancevault) to see the full documented project structure.

<p align="center">
  <p align="center">
  <a href="https://github.com/qoddit/balance/actions/workflows/ci.yml"><img alt="CI status" src="https://img.shields.io/github/actions/workflow/status/qoddit/balance/ci.yml?branch=main&amp;style=for-the-badge&amp;label=CI&amp;logo=github"></a>
  <a
<a href="https://github.com/qoddit/balance/actions/workflows/release.yml"><img alt="Release workflow" src="https://img.shields.io/github/actions/workflow/status/qoddit/balance/release.yml?style=for-the-badge&amp;label=release&amp;logo=github"></a>
  <a href="https://github.com/qoddit/balance/releases"><img alt="GitHub releases" src="https://img.shields.io/github/v/release/qoddit/balance?include_prereleases&amp;style=for-the-badge&amp;label=release"></a>
  <a href="https://github.com/qoddit/balance/releases"><img alt="GitHub releases" src="https://img.shields.io/badge/releases-GitHub-181717?logo=github&amp;style=for-the-badge" height="28"></a>
  <a href="https://github.com/qoddit/balance/blob/main/LICENSE"><img alt="License: AGPL-3.0-or-later" src="https://img.shields.io/badge/license-AGPL--3.0--or--later-blue?style=for-the-badge" height="28"></a>
  <a href="https://github.com/qoddit/balance/commits/main"><img alt="Last commit" src="https://img.shields.io/github/last-commit/qoddit/balance.svg?style=for-the-badge" height="28"></a>
  <a href="https://github.com/qoddit/balance"><img alt="Repository" src="https://img.shields.io/badge/source-GitHub-181717?logo=github&amp;style=for-the-badge" height="28"></a>
</p>

<img width="1470" height="956" alt="balance tui" src="https://github.com/user-attachments/assets/6cfc632c-9255-4a0b-ac4c-155458bd542d" />

## highlights

- native C17 TUI built around curses/ncurses.
- fixed bottom composer with a scrollable chat viewport.
- pinned accept/reject actions above the composer.
- tab autocomplete for commands, friends, and pending request IDs.
- command history with Up/Down on home screens.
- authenticated Ed25519 peer identities.
- ephemeral X25519 session keys.
- ChaCha20-Poly1305 encrypted application frames.
- strict monotonic sequence checking for replay/out-of-order rejection.
- encrypted local vault and optional encrypted chat history.
- volatile mode that never invokes the cloud backend.
- recipient-approved encrypted peer-to-peer file transfer.
- transfer SHA-256 verification and safe non-overwriting finalization.
- live transfer percentage and throughput display.
- optional self-hosted encrypted blob persistence over HTTPS.
- `fetch` dashboard with the balance logo, clock, version, memory, network counters, and traffic graph.
- no telemetry, analytics, advertising, or application logging subsystem.

## quick start

For a full platform matrix, troubleshooting, strict builds, and sanitizer builds, see [`docs/BUILD.md`](docs/BUILD.md). This project uses C17, CMake, OpenSSL 3, libcurl, POSIX threads, and curses/ncurses.

### macOS — Apple Silicon and Intel

Install Xcode Command Line Tools and Homebrew, then install the dependencies:

```bash
xcode-select --install  # only if command-line tools are not already installed
brew install cmake openssl@3 curl ncurses
```

From the repository root:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DOPENSSL_INCLUDE_DIR="$(brew --prefix openssl@3)/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$(brew --prefix openssl@3)/lib/libcrypto.dylib" \
  -DCMAKE_PREFIX_PATH="$(brew --prefix curl);$(brew --prefix ncurses)"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

If Homebrew uses a different library filename or prefix on your machine, inspect `brew --prefix openssl@3` and pass the actual OpenSSL library path. Do not reuse a `build/` directory copied from Linux or another Mac architecture; delete it and configure again on the target machine.

### Linux

Install the compiler toolchain, CMake, OpenSSL 3 development headers, libcurl development headers, and ncurses development headers using your distribution's package manager. Examples for common distributions are in [`docs/BUILD.md`](docs/BUILD.md).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/balance
```

### Windows

Native Windows/MSVC is **not currently a supported build target** because Balance depends on POSIX sockets and curses. On Windows, use **WSL2 with a supported Linux distribution** (for example Ubuntu), install the Linux dependencies inside WSL, and build/run from the WSL filesystem. See [`docs/BUILD.md`](docs/BUILD.md). A Windows terminal emulator can display the WSL terminal, but that does not make the executable a native Windows binary.


## first run

`balance` registers the local account on first use. credentials are used to derive the key that unlocks the encrypted local vault; the plaintext password is not persisted.

after login, use `?` to see the command guide.

```text
balance  beta-0.3.3                             alice [volatile]
────────────────────────────────────────────────────────────────────────────

  type ? for help; Tab completes commands and friends

────────────────────────────────────────────────────────────────────────────
 ? help | type normally in chat to send | mouse: pinned actions
 balance ›
```

## usage

### home commands

| Command | Example | Purpose |
|---|---|---|
| `?` | `?` | Open the full help guide. |
| `? <command>` | `? tunnel` | Show focused help for a command. |
| `version` | `version` | Show the product/build version. |
| `about` | `about` | Explain the security and operating model. |
| `status` | `status` | Show mode, friends, peers, tunnels, and cloud state. |
| `fetch` | `fetch` | Open the live fastfetch-style dashboard. |
| `whoami` | `whoami` | Show the current username. |
| `endpoint` | `endpoint` | Show the local listening endpoint. |
| `key` | `key` | Show the local Ed25519 identity fingerprint. |
| `memory` | `memory` | Show process peak RSS. |
| `net` | `net` | Show live TX/RX rates, frames, peers, and tunnels. |
| `time` | `time` | Show current local time. |
| `friends` | `friends` | List configured friends and key state. |
| `friends online` | `friends online` | Probe all friends. |
| `online` | `online` | Short alias for `friends online`. |
| `friend <name>` | `friend alice` | Show one friend's endpoint and fingerprint. |
| `add <name> <host:port>` | `add alice 10.0.0.5:33777` | Add a friend and send a friend request. |
| `remove <name>` | `remove alice` | Remove a local friend record. |
| `tunnel <friend>` | `tunnel alice` | Start an authenticated volatile tunnel request. |
| `chat <friend>` | `chat alice` | Enter an active tunnel/chat. |
| `ping <friend>` | `ping alice` | Measure peer reachability latency. |
| `accept <id-or-name>` | `accept vlt-...` | Accept a tunnel, file, or friend request. |
| `reject <id> [reason]` | `reject 8f2a busy` | Reject a pending request. |
| `history` | `history` | Display encrypted local history entries. |
| `history <friend>` | `history alice` | Filter local history by chat name. |
| `history clear` | `history clear` | Remove the encrypted local history file. |
| `search <text>` | `search deploy` | Search the current in-memory view. |
| `transfers` | `transfers` | List active/recent file transfers. |
| `cloud` | `cloud` | Show cloud configuration without revealing the token. |
| `cloud on <url> [token]` | `cloud on https://vault.example token` | Enable encrypted cloud persistence. |
| `cloud off` | `cloud off` | Disable cloud persistence. |
| `sync` | `sync` | Upload the encrypted local history blob. |
| `pull` | `pull` | Download the encrypted cloud history blob. |
| `config` | `config` | Show local paths and non-secret configuration. |
| `doctor` | `doctor` | Check local storage permissions and cloud configuration. |
| `theme <name>` | `theme cyber` | Change `cyber`, `mono`, `amber`, or `cycle`. |
| `redraw` | `redraw` | Rebuild the entire TUI layout. |
| `clear` | `clear` | Clear the current volatile UI view. |
| `quit` | `quit` | Exit `balance`. |

there are intentionally more than 30 commands in the registry so the help system, autocomplete, aliases, and future command additions all share one source of truth.

### chat

inside a chat, normal text is sent as an encrypted message. commands remain available when useful:

```text
?                     help
file <path>           request a file transfer
accept <id>           accept a file/tunnel request
reject <id> [reason]  reject a request
transfers             inspect file-transfer state
history               inspect persisted history, if enabled
search <text>         search the current in-memory transcript
close                 close the volatile tunnel
fetch                  open the dashboard without leaving the chat
clear                 clear the visible volatile transcript
quit                  exit balance
```

### keyboard shortcuts

- `Tab` — autocomplete commands/friends/request IDs.
- `F1` — help.
- `Up` / `Down` — scroll chat; on home, browse command history.
- `PageUp` / `PageDown` — faster chat scrolling.
- `Ctrl-L` — redraw.
- `Ctrl-D` — quit when the composer is empty.
- `Esc` — return from the dashboard.
- Mouse click on a pinned action — fills the action command into the composer; it does not silently execute it.

the tunnel handshake authenticates the peer's long-term Ed25519 identity and derives ephemeral session encryption keys with X25519/HKDF.

when the tunnel is closed or the peer disconnects:

- the volatile transcript is cleared from the UI state;
- unfinished `.part` file transfers are deleted;
- the tunnel state is discarded;
- no cloud synchronization is performed in volatile mode.

completed received files remain in `transfers/<local-user>--<peer>/`.

application-layer encryption does **not** hide network metadata such as IP addresses, connection timing, traffic volume, or approximate packet sizes.

## file transfer

file transfer is request-based and peer-to-peer.

sender:

```text
file ~/Documents/report.pdf
```

recipient sees a pinned request and can select **accept** or **reject**. the action fills the exact command into the prompt.

accepted transfers display percentage and throughput:

```text
report.pdf  [###############-----] 76.3% · 18.4 MiB/s
```

the receiver stages data in a `0600` `.part` file, verifies the expected size and SHA-256 digest, then finalizes to a non-overwriting filename.

## cloud mode

cloud persistence is opt-in. the included backend is a small self-hosted blob service.

the backend receives an **already encrypted** history object. it does not receive the plaintext transcript or the vault key.

```text
plaintext chat
     ↓
local encrypted history
     ↓ HTTPS
self-hosted blob server
     ↓
opaque ciphertext
```

remote cloud URLs must use HTTPS. Plain HTTP is permitted only for `localhost`/`127.0.0.1` development testing.

Supabase and Google Drive are not implemented providers in this release. the cloud boundary is intentionally isolated so additional providers can be added without redesigning the cryptographic protocol.

## local storage

the user vault is stored under the platform's local application-data directory and is encrypted with a key derived from the account password. the code enforces restrictive filesystem permissions for the vault/history files and local transfer directories.

`doctor` checks these permissions without printing secret material.

## security model

the project separates these layers:

```text
ui.c       curses rendering, commands, autocomplete, actions
net.c      sockets, handshake, peers, encrypted tunnels
proto.c    authenticated frame encoding/decoding
crypto.c   Scrypt, HKDF, X25519, Ed25519, ChaCha20-Poly1305, SHA-256
store.c    encrypted local vault/history
cloud.c    opaque encrypted history upload/download
server.c   storage-only blob backend
```

### authentication

- local password unlocks the encrypted vault.
- Ed25519 identifies a peer.
- friend keys are pinned after explicit acceptance.
- incoming tunnel requests are rejected if the identity does not match the pinned friend key.

### session confidentiality

- ephemeral X25519 is used for session key agreement.
- HKDF derives directional session material.
- ChaCha20-Poly1305 provides authenticated encryption.
- frame sequence numbers must advance monotonically.

### privacy

there is no telemetry or analytics subsystem. the cloud is disabled by default. Volatile chats do not write their transcript to persistent history.

this project has not been independently audited. passing tests do not constitute a cryptographic audit or a guarantee against previously unknown implementation vulnerabilities.

## self-hosted server

development-only server:

```bash
./build/balance-server \
  --bind 127.0.0.1:8080 \
  --data ./balance-server-data \
  --token CHANGE_ME
```

for remote deployments, terminate TLS in a hardened reverse proxy, restrict network access, rotate the bearer token, and keep the storage directory private.

the server stores opaque encrypted blobs and does not decrypt chat content.

## build and test

strict local build:

```bash
cmake -S . -B build-werror \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-Werror"
cmake --build build-werror -j
ctest --test-dir build-werror --output-on-failure
```

sanitizer build:

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1"
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ctest --test-dir build-asan --output-on-failure
```

the release validation also includes a curses PTY smoke test for registration/login, help, dashboard rendering, and clean exit.

## version control and releases

`balance` uses Git with explicit release metadata:

- `VERSION` contains the product version, currently `beta-0.3.1`.
- `CMakeLists.txt` carries the matching SemVer core `0.3.1`.
- `CHANGELOG.md` records release changes.
- releases are tagged as `v0.3.1-beta`.
- `.github/workflows/ci.yml` builds/tests Linux and macOS.
- `.github/workflows/release.yml` packages tagged releases.
- `scripts/release.sh` validates the version/tag and creates a source archive plus SHA-256 checksum.

recommended workflow:

```bash
git checkout -b feature/my-change
# edit / test
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
git add .
git commit -m "feat: describe the change"
git push -u origin feature/my-change
```

a release is created only from a clean tagged commit:

```bash
./scripts/release.sh v0.3.1-beta
```

## building on Balance as a library

Balance is currently a standalone executable, not yet a supported reusable library. Its protocol, crypto, storage, and networking code are separated into C modules, which gives the project a useful starting point for a future library API. The intended direction is to expose a small, documented C API that other GUI projects can link against while keeping the terminal UI as one optional frontend.

The API design, ABI/versioning policy, ownership rules, event model, and build/install targets still need to be formalized before consumers should depend on these internals. See [`docs/LIBRARY_ROADMAP.md`](docs/LIBRARY_ROADMAP.md) for the proposed migration steps and architectural constraints. For now, external projects should not include internal headers or rely on private structs as a stable interface.

## repository layout

```text
balance/
├── .github/workflows/
├── compat/
├── docs/
├── scripts/
├── src/
│   ├── cloud.c/.h
│   ├── commands.c/.h
│   ├── common.c/.h
│   ├── crypto.c/.h
│   ├── main.c
│   ├── net.c/.h
│   ├── proto.c/.h
│   ├── server.c
│   ├── store.c/.h
│   ├── ui.c/.h
│   └── version.h
├── tests/
├── CHANGELOG.md
├── CMakeLists.txt
├── CONTRIBUTING.md
├── VERSION
└── README.md
```

## status

`beta-0.3.3` is a functional beta release. the local encrypted account/vault path, P2P authenticated tunnel, encrypted chat, request-based file transfer, curses command UI, encrypted self-hosted cloud blob path, and test suite are active.

the release deliberately does not claim anonymity, third-party cryptographic audit, or fully implemented Supabase/Google Drive adapters.

