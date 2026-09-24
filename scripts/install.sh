#!/usr/bin/env bash
# Build the custom OpenFHE fork and AdvancedFHE, without sudo.
#
#   ./scripts/install.sh
#
# Env knobs:
#   PREFIX=<dir>          where the fork is installed   (default: .deps/openfhe)
#   OPENFHE_REF=<sha|br>  fork commit to build           (default: pinned SHA below)
#   NATIVE=1              build the fork with -march=native (WITH_NATIVEOPT)
#   PROFILE=1             apply scripts/openfhe-profile.patch (per-step [profile] timers)
#   JOBS=<n>              parallel build jobs            (default: nproc)
#
# Result: build/AdvancedFHE, checked with `--test`. Run it from build/, since
# the coefficient files are loaded from ../coeffs.
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$PWD"
PREFIX="${PREFIX:-$ROOT/.deps/openfhe}"
SRC="$ROOT/.deps/src/openfhe-development-chebyshevSIMD"
REPO_URL=https://github.com/lorenzorovida/openfhe-development-chebyshevSIMD
BRANCH=repeated_poly_and_stcboot
# Tip of $BRANCH when this script was verified (2026-09-24). Pinned so numbers are
# reproducible; set OPENFHE_REF=$BRANCH to track the branch.
OPENFHE_REF="${OPENFHE_REF:-d9da0cc0b94d3fbc02cb9eafb5ec8f6af0a5d6ea}"
JOBS="${JOBS:-$(nproc)}"

for t in git cmake make c++; do
    command -v "$t" >/dev/null || { echo "missing: $t (apt install build-essential cmake git)" >&2; exit 1; }
done

if [ ! -d "$SRC/.git" ]; then
    git clone -q --branch "$BRANCH" "$REPO_URL" "$SRC"
fi
git -C "$SRC" fetch -q origin "$BRANCH"
git -C "$SRC" checkout -q "$OPENFHE_REF"
echo "OpenFHE fork at $(git -C "$SRC" rev-parse --short HEAD)"

# The fork hardcodes -Wall -Werror after any user flags, and gcc 16 flags
# -Wsign-compare in ckksrns-advancedshe.cpp (the fork's own additions), so
# drop -Werror. Reset the checkout first so patches don't pile up across runs.
git -C "$SRC" checkout -q -- .
sed -i 's/ -Werror / /' "$SRC/CMakeLists.txt"
# PROFILE=1: time plaintext encoding, batched Chebyshev and StC-first bootstrap;
# AdvancedFHE prints the totals as [profile] lines at exit.
if [ "${PROFILE:-0}" = 1 ]; then git -C "$SRC" apply "$ROOT/scripts/openfhe-profile.patch"; fi

# Tests/examples/benchmarks are off: they add about 3x to the build and nothing here uses them.
cmake -S "$SRC" -B "$SRC/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DLIBINSTALL="$PREFIX/lib" \
    -DBUILD_UNITTESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF \
    -DWITH_NATIVEOPT="$([ "${NATIVE:-0}" = 1 ] && echo ON || echo OFF)"
cmake --build "$SRC/build" -j "$JOBS"
# Fresh prefix: install skips "up-to-date" files, which would keep a stale rpath.
rm -rf "$PREFIX"
cmake --install "$SRC/build" >/dev/null

# A non-system PREFIX is not on the loader path: the binary gets a build rpath,
# and the fork's libs get LIBINSTALL above (libOPENFHEpke must find binfhe). The
# fork sets CMAKE_INSTALL_RPATH from LIBINSTALL but never defines it.
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_BUILD_RPATH="$PREFIX/lib"
cmake --build "$ROOT/build" -j "$JOBS"

(cd "$ROOT/build" && ./AdvancedFHE --test --ring 12)
