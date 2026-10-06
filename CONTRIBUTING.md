# contributing to balance

## development rules

keep security-sensitive behavior explicit and small. do not introduce telemetry, plaintext credential storage, implicit cloud access, or network logging.

every change should preserve:

- authenticated peer identity checking;
- AEAD frame integrity and sequence validation;
- restrictive local filesystem permissions;
- volatile-mode no-persistence behavior;
- explicit user approval for incoming tunnel/file requests.

## before opening a pull request

```bash
cmake -S . -B build-werror -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-Werror"
cmake --build build-werror -j
ctest --test-dir build-werror --output-on-failure
```

for security-sensitive changes, also run the AddressSanitizer/UndefinedBehaviorSanitizer build described in `README.md`.

## commit style

use short, scoped commits when practical:

```text
feat: add command registry
fix: reject stale tunnel sequence
refactor: isolate dashboard renderer
test: add file digest regression
```

## releases

releases are created from a clean tagged commit. The canonical beta tag is `v0.3.1-beta`.
