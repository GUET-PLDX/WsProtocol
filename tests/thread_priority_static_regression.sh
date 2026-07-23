#!/usr/bin/env bash

set -euo pipefail

header=${1:-WsProtocol.hpp}

if [[ ! -f "$header" ]]; then
  printf 'missing header: %s\n' "$header" >&2
  exit 1
fi

fail() {
  printf 'priority regression: %s\n' "$1" >&2
  exit 1
}

constructor=$(sed -n '/WsProtocol(LibXR::HardwareContainer&/,/^  }/p' "$header")
[[ -n "$constructor" ]] || fail 'constructor not found'

if rg -q 'UNUSED\(thread_priority_uart\)' <<<"$constructor"; then
  fail 'thread_priority_uart is discarded'
fi

flat_constructor=$(tr '\n' ' ' <<<"$constructor")
rg -q 'thread_\.Create\(this, ThreadFunc, "WsProtocol",[[:space:]]*task_stack_depth_uart,[[:space:]]*thread_priority_uart\)' \
  <<<"$flat_constructor" || fail 'thread_priority_uart is not passed to thread_.Create'

if [[ "${PRIORITY_REGRESSION_MUTATION_CHILD:-0}" != "1" ]]; then
  mutant=$(mktemp)
  trap 'rm -f "$mutant"' EXIT
  perl -0pe \
    's/task_stack_depth_uart,\s*thread_priority_uart/task_stack_depth_uart, LibXR::Thread::Priority::MEDIUM/' \
    "$header" >"$mutant"
  if PRIORITY_REGRESSION_MUTATION_CHILD=1 bash "$0" "$mutant" >/dev/null 2>&1; then
    fail 'hardcoded-priority mutation survived'
  fi
fi

printf 'PASS: WsProtocol thread priority contract\n'
