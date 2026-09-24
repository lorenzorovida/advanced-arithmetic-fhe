#!/usr/bin/env bash
# Where does the CPU-vs-GPU gap on --uniswapv3 come from? Four checks, one run each,
# all with the [profile] timers (PROFILE=1 build):
#   rings    ring 14 and 15, like-for-like with the GPU numbers (3.5 s / 6 s hot cache)
#   omp      ring 14 at OMP_NUM_THREADS 16 / 32 / 64 (128 is the default, from `rings`)
#   pinned   ring 14, 32 threads pinned to socket 0 (numactl + OMP_PROC_BIND=close)
#   native   ring 14 on a -march=native build (AVX-512), default threads
#
#   ./scripts/cpu_gap_checks.sh [outdir]      (default: benchmarks/<utc-date>-cpu-gap)
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-benchmarks/$(date -u +%Y-%m-%d)-cpu-gap}"
bench() { WORKLOADS=uniswapv3 ./scripts/run_benchmarks.sh "$OUT/$1" 1; }

PROFILE=1 ./scripts/install.sh

RINGS="14 15" bench rings
for t in 16 32 64; do OMP_NUM_THREADS=$t RINGS=14 bench "omp$t"; done

command -v numactl >/dev/null || sudo apt-get install -y -q numactl >/dev/null
OMP_NUM_THREADS=32 OMP_PROC_BIND=close OMP_PLACES=cores WRAPPER="numactl --cpunodebind=0 --membind=0" \
    RINGS=14 bench pinned32

PROFILE=1 NATIVE=1 ./scripts/install.sh
RINGS=14 bench native
