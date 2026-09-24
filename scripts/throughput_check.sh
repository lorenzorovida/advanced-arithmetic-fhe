#!/usr/bin/env bash
# Does a big multi-socket box pay off as a throughput machine? Ring-14 uniswapv3:
#   single    1 process, 16 threads pinned to 16 physical cores of socket 0
#   quad      4 processes at once, 16 threads each, 2 per socket, disjoint physical cores,
#             memory bound to the process's own socket
# If each quad process still takes about as long as `single`, throughput scales ~4x. If they
# slow down, memory bandwidth / LLC is the limit. Each process logs `/usr/bin/time -v`
# plus the [profile] lines (PROFILE=1 build).
#
#   ./scripts/throughput_check.sh [outdir] [procs]    (defaults: benchmarks/<utc-date>-throughput, 4)
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="$(realpath -m "${1:-benchmarks/$(date -u +%Y-%m-%d)-throughput}")"
PROCS="${2:-4}"
THREADS=16
mkdir -p "$OUT"
command -v numactl >/dev/null || sudo apt-get install -y -q numactl >/dev/null

# One logical CPU per physical core, grouped by socket: "socket cpu" lines.
lscpu -p=cpu,core,socket | grep -v '^#' | sort -t, -k3,3n -k2,2n -k1,1n | awk -F, '!seen[$2","$3]++ {print $3, $1}' > "$OUT/cores.txt"
numactl --hardware > "$OUT/numa.txt"
{ lscpu; echo; cat "$OUT/numa.txt"; } > "$OUT/env.txt"
git rev-parse --short HEAD >> "$OUT/env.txt"

# cpus for process k: socket k%2 (so processes alternate sockets), block k/2 of 16 cores on it
cpus_for() {
    local k=$1 s=$(( $1 % 2 )) b=$(( $1 / 2 ))
    awk -v s="$s" '$1 == s {print $2}' "$OUT/cores.txt" | sed -n "$(( b * THREADS + 1 )),$(( (b + 1) * THREADS ))p" | paste -sd,
}

run() {  # name k
    local cpus; cpus="$(cpus_for "$2")"
    [ "$(tr ',' '\n' <<<"$cpus" | wc -l)" -eq "$THREADS" ] || { echo "not enough cores for process $2" >&2; exit 1; }
    echo "$1: socket $(( $2 % 2 )) cpus $cpus" >> "$OUT/placement.txt"
    (cd build && OMP_NUM_THREADS=$THREADS OMP_PLACES=cores OMP_PROC_BIND=close \
        numactl --physcpubind="$cpus" --membind=$(( $2 % 2 )) \
        /usr/bin/time -v ./AdvancedFHE --ring 14 --bits 128 --uniswapv3 > "$OUT/$1.log" 2>&1)
}

run single 0
for k in $(seq 0 $(( PROCS - 1 ))); do run "quad$k" "$k" & done
wait

for f in "$OUT"/*.log; do
    printf '%s  wall=%s  rss_kb=%s  amount_ok=%s\n' "$(basename "$f" .log)" \
        "$(grep -a 'Elapsed (wall' "$f" | awk '{print $NF}')" \
        "$(grep -a 'Maximum resident' "$f" | awk '{print $NF}')" \
        "$(sed 's/\x1b\[[0-9;]*m//g' "$f" | grep -ac 'Amount_fx: 25965667213236754308256')"
done | tee "$OUT/summary.txt"
