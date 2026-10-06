# balance

> Privacy-first, terminal-native encrypted messaging in C17.

**Current release:** `beta-0.3.1`

`balance` is a curses-based CLI messenger built around a local-first security model. The default chat mode is **volatile peer-to-peer**: messages are encrypted before transmission, no cloud service is contacted, and the volatile transcript is discarded when the tunnel closes. Cloud persistence is an explicit opt-in.

[![CI](https://github.com/your-org/balance/actions/workflows/ci.yml/badge.svg)](https://github.com/qoddit/balance/actions/workflows/ci.yml)

## Highlights

- Native C17 TUI built around curses/ncurses.
- Fixed bottom composer with a scrollable chat viewport.
- Pinned accept/reject actions above the composer.
- Tab autocomplete for commands, friends, and pending request IDs.
- Command history with Up/Down on home screens.
- Authenticated Ed25519 peer identities.
- Ephemeral X25519 session keys.
- ChaCha20-Poly1305 encrypted application frames.
- Strict monotonic sequence checking for replay/out-of-order rejection.
- Encrypted local vault and optional encrypted chat history.
- Volatile mode that never invokes the cloud backend.
- Recipient-approved encrypted peer-to-peer file transfer.
- Transfer SHA-256 verification and safe non-overwriting finalization.
- Live transfer percentage and throughput display.
- Optional self-hosted encrypted blob persistence over HTTPS.
- `fetch` dashboard with the balance logo, clock, version, memory, network counters, and traffic graph.
- No telemetry, analytics, advertising, or application logging subsystem.

## Quick start

### macOS — Apple Silicon / Intel

Install dependencies:

```bash
brew install cmake openssl@3 curl ncurses
```

Extract the release archive, then build:

```bash
cmake -S . -B build \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
  -DCMAKE_PREFIX_PATH="$(brew --prefix openssl@3);$(brew --prefix curl);$(brew --prefix ncurses)"

cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/balance
```

### Arch Linux

```bash
sudo pacman -S --needed base-devel cmake openssl curl ncurses
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/balance
```

## First run

`balance` registers the local account on first use. Credentials are used to derive the key that unlocks the encrypted local vault; the plaintext password is not persisted.

After login, use `?` to see the command guide.

```text
balance  beta-0.3.1                              alice [volatile]
────────────────────────────────────────────────────────────────────────────

  type ? for help; Tab completes commands and friends

────────────────────────────────────────────────────────────────────────────
 ? help | type normally in chat to send | mouse: pinned actions
 balance ›
```

## Usage

### Home commands

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

There are intentionally more than 30 commands in the registry so the help system, autocomplete, aliases, and future command additions all share one source of truth.

### Chat

Inside a chat, normal text is sent as an encrypted message. Commands remain available when useful:

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

### Keyboard shortcuts

- `Tab` — autocomplete commands/friends/request IDs.
- `F1` — help.
- `Up` / `Down` — scroll chat; on home, browse command history.
- `PageUp` / `PageDown` — faster chat scrolling.
- `Ctrl-L` — redraw.
- `Ctrl-D` — quit when the composer is empty.
- `Esc` — return from the dashboard.
- Mouse click on a pinned action — fills the action command into the composer; it does not silently execute it.

## Dashboard

`fetch` opens a live curses dashboard designed in the style of a terminal system fetcher, but it reports `balance`-specific information instead of generic host data.

```text
 .:     .    ..          time       22:41
.:--          --:.-         time       2026-10-06 22:41:12
:-==         .:-:...        version    beta-0.3.1
.---=:      .---:.          memory     8.4 MiB peak RSS
   ::.--::.::::.             tx rate    12.1 KiB/s
      ¨’::…::               rx rate    41.7 KiB/s
                             tx frames  83
                             rx frames  121
                             peers      2
                             tunnels    1

                             traffic  # RX  + TX
                             ······++###*****++··
`

The dashboard tracks at least these network statistics:

- transmitted bytes and current transmit rate;
- received bytes and current receive rate;
- transmitted frame count;
- received frame count;
- active peers;
- active tunnels.

The graph is a rolling RX/TX throughput graph sampled by the TUI.

## Volatile mode

Volatile mode is the default and is the safest operating mode provided by `balance`.

```text
Alice ── authenticated handshake ── Bob
  │                                  │
  └── ChaCha20-Poly1305 tunnel ─────┘

cloud: not contacted
chat history: memory only
```

The tunnel handshake authenticates the peer's long-term Ed25519 identity and derives ephemeral session encryption keys with X25519/HKDF.

When the tunnel is closed or the peer disconnects:

- the volatile transcript is cleared from the UI state;
- unfinished `.part` file transfers are deleted;
- the tunnel state is discarded;
- no cloud synchronization is performed in volatile mode.

Completed received files remain in `transfers/<local-user>--<peer>/`.

Application-layer encryption does **not** hide network metadata such as IP addresses, connection timing, traffic volume, or approximate packet sizes.

## File transfer

File transfer is request-based and peer-to-peer.

Sender:

```text
file ~/Documents/report.pdf
```

Recipient sees a pinned request and can select **accept** or **reject**. The action fills the exact command into the prompt.

Accepted transfers display percentage and throughput:

```text
report.pdf  [###############-----] 76.3% · 18.4 MiB/s
```

The receiver stages data in a `0600` `.part` file, verifies the expected size and SHA-256 digest, then finalizes to a non-overwriting filename.

## Cloud mode

Cloud persistence is opt-in. The included backend is a small self-hosted blob service.

The backend receives an **already encrypted** history object. It does not receive the plaintext transcript or the vault key.

```text
plaintext chat
     ↓
local encrypted history
     ↓ HTTPS
self-hosted blob server
     ↓
opaque ciphertext
```

Remote cloud URLs must use HTTPS. Plain HTTP is permitted only for `localhost`/`127.0.0.1` development testing.

Supabase and Google Drive are not implemented providers in this release. The cloud boundary is intentionally isolated so additional providers can be added without redesigning the cryptographic protocol.

## Local storage

The user vault is stored under the platform's local application-data directory and is encrypted with a key derived from the account password. The code enforces restrictive filesystem permissions for the vault/history files and local transfer directories.

`doctor` checks these permissions without printing secret material.

## Security model

The project separates these layers:

```text
ui.c       curses rendering, commands, autocomplete, actions
net.c      sockets, handshake, peers, encrypted tunnels
proto.c    authenticated frame encoding/decoding
crypto.c   Scrypt, HKDF, X25519, Ed25519, ChaCha20-Poly1305, SHA-256
store.c    encrypted local vault/history
cloud.c    opaque encrypted history upload/download
server.c   storage-only blob backend
```

### Authentication

- Local password unlocks the encrypted vault.
- Ed25519 identifies a peer.
- Friend keys are pinned after explicit acceptance.
- Incoming tunnel requests are rejected if the identity does not match the pinned friend key.

### Session confidentiality

- Ephemeral X25519 is used for session key agreement.
- HKDF derives directional session material.
- ChaCha20-Poly1305 provides authenticated encryption.
- Frame sequence numbers must advance monotonically.

### Privacy

There is no telemetry or analytics subsystem. The cloud is disabled by default. Volatile chats do not write their transcript to persistent history.

This project has not been independently audited. Passing tests do not constitute a cryptographic audit or a guarantee against previously unknown implementation vulnerabilities.

## Self-hosted server

Development-only server:

```bash
./build/balance-server \
  --bind 127.0.0.1:8080 \
  --data ./balance-server-data \
  --token CHANGE_ME
```

For remote deployments, terminate TLS in a hardened reverse proxy, restrict network access, rotate the bearer token, and keep the storage directory private.

The server stores opaque encrypted blobs and does not decrypt chat content.

## Build and test

Strict local build:

```bash
cmake -S . -B build-werror \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-Werror"
cmake --build build-werror -j
ctest --test-dir build-werror --output-on-failure
```

Sanitizer build:

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -O1"
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ctest --test-dir build-asan --output-on-failure
```

The release validation also includes a curses PTY smoke test for registration/login, help, dashboard rendering, and clean exit.

## Version control and releases

`balance` uses Git with explicit release metadata:

- `VERSION` contains the product version, currently `beta-0.3.1`.
- `CMakeLists.txt` carries the matching SemVer core `0.3.1`.
- `CHANGELOG.md` records release changes.
- Releases are tagged as `v0.3.1-beta`.
- `.github/workflows/ci.yml` builds/tests Linux and macOS.
- `.github/workflows/release.yml` packages tagged releases.
- `scripts/release.sh` validates the version/tag and creates a source archive plus SHA-256 checksum.

Recommended workflow:

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

A release is created only from a clean tagged commit:

```bash
./scripts/release.sh v0.3.1-beta
```

## Repository layout

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

## Status

`beta-0.3.1` is a functional beta release. The local encrypted account/vault path, P2P authenticated tunnel, encrypted chat, request-based file transfer, curses command UI, encrypted self-hosted cloud blob path, and test suite are active.

The release deliberately does not claim anonymity, third-party cryptographic audit, or fully implemented Supabase/Google Drive adapters.
