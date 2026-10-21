# Changelog

## Unreleased

- Expand cross-platform build instructions for macOS, major Linux distributions, Alpine, and Windows via WSL2.
- Add a Pinokio launcher with explicit build and launch actions; the launcher does not install system packages or claim native Windows support.
- Document a staged roadmap for turning the internal C modules into a reusable library for future GUI frontends.
- Clarify that the future public library API is not yet stable or implemented.
- `fetch`: remove the redundant `BALANCE STATUS` heading so the statistics panel starts directly beside the ASCII artwork.

## beta-0.3.1 fetch command polish

- Renamed the canonical fastfetch-style dashboard command from `logo` to `fetch`; `dashboard` remains as a compatibility alias.
- Updated dashboard layout to place the supplied Balance ASCII art on the left with compact spacing before live stats and graph.

## beta-0.3.1 — 2026-10-06

### Added

- C17 curses TUI command registry with 30+ practical commands.
- Focused `? <command>` help and categorized global help.
- Tab completion for commands, aliases, friends, and pinned request IDs.
- Command history on home/dashboard views.
- `logo` fastfetch-style dashboard with time, version, peak RSS, six network metrics, and RX/TX graph.
- `status`, `about`, `whoami`, `endpoint`, `key`, `memory`, `net`, `time`, `doctor`, `config`, `theme`, `sync`, `pull`, `history`, `search`, `transfers`, and `ping` commands.
- Explicit volatile tunnel close command.
- Live network TX/RX counters and active peer/tunnel counts.
- Version registry, GitHub CI/release workflows, changelog, and release script.

### Improved

- Cleaner curses layout with fixed composer, pinned request bar, status strip, and keyboard hints.
- Wrapped chat rendering with timestamps and sender labels.
- More useful friend/key status output.
- Explicit secure history cleanup command.
- Large history buffers moved off the stack after sanitizer validation.
- macOS/POSIX portability controls retained for `flock`/`mkdtemp` and curses.

### Security/quality

- Added command-registry regression coverage.
- Re-ran strict `-Werror` builds.
- Re-ran AddressSanitizer + UndefinedBehaviorSanitizer with leak detection.
- Added a curses PTY smoke test covering registration, help, dashboard, and clean exit.
