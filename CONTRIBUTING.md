# Contributing to balance

## Development rules

Keep security-sensitive behavior explicit and small. Do not introduce telemetry, plaintext credential storage, implicit cloud access, or network logging.

Every change should preserve:

- authenticated peer identity checking;
- AEAD frame integrity and sequence validation;
- restrictive local filesystem permissions;
- volatile-mode no-persistence behavior;
- explicit user approval for incoming tunnel/file requests.

## Before opening a pull request

```bash
cmake -S . -B build-werror -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-Werror"
cmake --build build-werror -j
ctest --test-dir build-werror --output-on-failure
```

For security-sensitive changes, also run the AddressSanitizer/UndefinedBehaviorSanitizer build described in `README.md`.

## Commit style

Use short, scoped commits when practical:

```text
feat: add command registry
fix: reject stale tunnel sequence
refactor: isolate dashboard renderer
test: add file digest regression
```

## Releases

Releases are created from a clean tagged commit. The canonical beta tag is `v0.3.1-beta`.
