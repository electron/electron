#!/bin/bash
# One clang-tidy build step (see script/gen-clang-tidy-ninja.py).
#   run-clang-tidy-action.sh <clang> <compile flags...> -c <source> -o <output>
# Fails if clang-tidy reports anything, so only clean results get cached.
# Writes <output> and a gcc depfile <output>.d for ninja/siso.

set -euo pipefail

bin_dir="$(dirname "$1")"
clang="$1"
shift

flags=()
src=
out=
while (($#)); do
  case "$1" in
    -c) src="$2"; shift 2 ;;
    -o) out="$2"; shift 2 ;;
    *) flags+=("$1"); shift ;;
  esac
done

if [[ -z "$src" || -z "$out" ]]; then
  echo "usage: $0 <clang> <flags...> -c <source> -o <output>" >&2
  exit 2
fi

mkdir -p "$(dirname "$out")"
"$bin_dir/clang-tidy" --quiet -header-filter= "--warnings-as-errors=*" "$src" -- "${flags[@]}"
"$clang" "${flags[@]}" -w -M -MF "$out.d" -MT "$out" "$src"
: > "$out"
