#!/bin/sh
set -eu
if grep -q 'BALANCE STATUS' src/ui.c; then
  echo "ERROR: redundant BALANCE STATUS fetch heading remains" >&2
  exit 1
fi
[ -f CMakeLists.txt ]
[ -f VERSION ]
[ -f src/version.h ]
[ -f src/commands.c ]
[ -f src/commands.h ]
[ -f src/net.c ]
[ -f src/proto.c ]
[ -f src/crypto.c ]
[ -f src/store.c ]
[ -f src/ui.c ]
grep -F 'beta-0.3.1' VERSION >/dev/null
grep -F '#define BALANCE_VERSION "beta-0.3.1"' src/version.h >/dev/null
grep -F 'project(balance VERSION 0.3.1 LANGUAGES C)' CMakeLists.txt >/dev/null
grep -F 'src/commands.c' CMakeLists.txt >/dev/null
grep -F 'net_get_stats' src/net.h >/dev/null
grep -F 'net_close_tunnel' src/net.h >/dev/null
grep -F 'store_clear_history' src/store.h >/dev/null
grep -F 'FRAME_FRIEND_REQUEST' tests/test_all.c >/dev/null
grep -F 'payload_len==0' tests/test_all.c >/dev/null
grep -F 'beta-0.3.1' tests/test_all.c >/dev/null
grep -F 'transfers/%s--%s' src/ui.c >/dev/null
grep -F 'traffic  # RX  + TX' src/ui.c >/dev/null
grep -F 'target_compile_options(balance-tests PRIVATE -UNDEBUG)' CMakeLists.txt >/dev/null
echo 'balance source verification: OK'
