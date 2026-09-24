#!/usr/bin/env bash
# Does thread placement matter? Ring-14 uniswapv3, one run each, on the current build:
#   unpinned16    16 threads, OS scheduling (the benchmark default)
#   cores16       16 threads, one per physical core         (OMP_PLACES=cores   OMP_PROC_BIND=close)
#   smt16         16 threads packed two per core, on 8 cores (OMP_PLACES=threads OMP_PROC_BIND=close)
#   smt32         32 threads, every hardware thread         (OMP_PLACES=threads OMP_PROC_BIND=close)
# OMP_DISPLAY_AFFINITY prints each thread's binding into the logs, as proof of placement.
#
#   ./scripts/placement_checks.sh [outdir]    (default: benchmarks/<utc-date>-placement)
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-benchmarks/$(date -u +%Y-%m-%d)-placement}"
export OMP_DISPLAY_AFFINITY=true
bench() { RINGS=14 WORKLOADS=uniswapv3 ./scripts/run_benchmarks.sh "$OUT/$1" 1; }

OMP_NUM_THREADS=16 OMP_PROC_BIND=false bench unpinned16
OMP_NUM_THREADS=16 OMP_PLACES=cores OMP_PROC_BIND=close bench cores16
OMP_NUM_THREADS=16 OMP_PLACES=threads OMP_PROC_BIND=close bench smt16
OMP_NUM_THREADS=32 OMP_PLACES=threads OMP_PROC_BIND=close bench smt32
