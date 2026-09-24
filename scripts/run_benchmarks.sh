#!/usr/bin/env bash
# Benchmark AdvancedFHE over a range of ring sizes and emit results.csv.
#
#   ./scripts/run_benchmarks.sh [outdir] [runs]
#
# Defaults: outdir=benchmarks/<utc-date>, runs=3.
#
# Env knobs:
#   RINGS="12 13 14 15 16"   log2(N). The CLI accepts 12..16 (README "Custom parameters").
#   BITS="64"                word sizes for the `ops` workload (8 16 32 64 128 256)
#   WORKLOADS="ops decompose uniswapv3"
#
# Workloads:
#   ops        default mode: add, sub, compare, eq, mul, shift, div, sqrt ... on N/bits^2 words
#   decompose  --decompose: 8-bit to binary via batched Chebyshev + bootstrap
#   uniswapv3  --uniswapv3: 128-bit pipeline. Needs N >= 2^14: it packs N/128^2 words,
#              and at N=2^12 that is 0 and the zero-fill loop's unsigned "- 1" wraps.
#
# Every process builds a bootstrapping context and keys first, and that cost depends
# on (ring, bits). So `--test`, which stops after keygen, is timed for each
# (ring, bits), and its median is subtracted as median_net.
#
# The parameters are HEStd_NotSet, so none of these rings comes with a security
# guarantee. At N=2^16 expect minutes per run and tens of GB of RAM.
#
# Produces, in <outdir>:
#   results.csv      every measurement, one per row
#   hyperfine.json   raw hyperfine output
#   logs/*.log       program stdout, one file per command (all runs appended)
#   env.txt          machine and toolchain
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$PWD"
OUTDIR="$(realpath -m "${1:-benchmarks/$(date -u +%Y-%m-%d)}")"
RUNS="${2:-3}"
RINGS="${RINGS:-12 13 14 15 16}"
BITS="${BITS:-64}"
WORKLOADS="${WORKLOADS:-ops decompose uniswapv3}"
BIN="$ROOT/build/AdvancedFHE"

command -v hyperfine >/dev/null || { echo "hyperfine not found" >&2; exit 1; }
[ -x "$BIN" ] || { echo "$BIN missing: run scripts/install.sh first" >&2; exit 1; }
mkdir -p "$OUTDIR/logs"

{
    echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "git_sha=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "git_dirty=$(test -n "$(git status --porcelain --untracked-files=no 2>/dev/null)" && echo yes || echo no)"
    echo "openfhe_sha=$(git -C .deps/src/openfhe-development-chebyshevSIMD rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "cpu_model=$(awk -F: '/model name/{print $2; exit}' /proc/cpuinfo | sed 's/^ *//')"
    echo "cpu_avx512f=$(grep -q avx512f /proc/cpuinfo && echo yes || echo no)"
    echo "cores=$(nproc)"
    echo "mem_gb=$(awk '/MemTotal/{printf "%d", $2/1048576}' /proc/meminfo)"
    echo "cxx=$(c++ --version | head -1)"
    echo "runs=$RUNS"
    echo "rings=$RINGS"
    echo "bits=$BITS"
    echo "workloads=$WORKLOADS"
} > "$OUTDIR/env.txt"
echo "--- environment ---"; cat "$OUTDIR/env.txt"

# name|args. Names are ring<r>/<workload>/<bits>, and the merge step below relies on that format.
CMDS=()
for r in $RINGS; do
    keygen_bits=()
    for w in $WORKLOADS; do
        case "$w" in
            ops)       for b in $BITS; do CMDS+=("ring$r/ops/$b|--ring $r --bits $b"); keygen_bits+=("$b"); done ;;
            decompose) CMDS+=("ring$r/decompose/32|--ring $r --decompose"); keygen_bits+=(32) ;;
            uniswapv3) if [ "$r" -ge 14 ]; then CMDS+=("ring$r/uniswapv3/128|--ring $r --bits 128 --uniswapv3"); keygen_bits+=(128)
                       else echo "skip uniswapv3 at ring $r (needs >= 14)" >&2; fi ;;
            *) echo "unknown workload: $w" >&2; exit 1 ;;
        esac
    done
    for b in $(printf '%s\n' "${keygen_bits[@]}" | sort -un); do
        CMDS+=("ring$r/keygen/$b|--ring $r --bits $b --test")
    done
done

# Run from build/, since the coefficient files are loaded from ../coeffs.
# --verbose 3 prints Expected/Obtained and per-op timings, which are parsed below.
# -i: one crashing command must not discard every other measurement; exit codes
# are kept in hyperfine.json and flagged in results.csv.
HF_ARGS=(--warmup 0 --runs "$RUNS" -i --export-json "$OUTDIR/hyperfine.json")
for c in "${CMDS[@]}"; do
    name="${c%%|*}"; args="${c#*|}"
    log="$OUTDIR/logs/${name//\//_}.log"
    HF_ARGS+=(--command-name "$name" "cd '$ROOT/build' && echo '=== run' >> '$log' && '$BIN' $args --verbose 3 >> '$log' 2>&1")
done
echo; echo "timing ${#CMDS[@]} commands, $RUNS run(s) each"
hyperfine "${HF_ARGS[@]}"

python3 - "$OUTDIR" <<'PYEOF'
import csv, json, pathlib, re, statistics, sys
out = pathlib.Path(sys.argv[1])
res = {r["command"]: r for r in json.loads((out / "hyperfine.json").read_text())["results"]}

rows = [("category", "ring", "workload", "bits", "item", "metric", "value", "unit")]
for name, r in res.items():
    ring, wl, bits = name.split("/")
    ring = ring.removeprefix("ring")
    rows.append(("timing", ring, wl, bits, "process", "median_wall", f"{r['median']:.3f}", "s"))
    rows.append(("timing", ring, wl, bits, "process", "stddev_wall", f"{r.get('stddev') or 0:.3f}", "s"))
    if bad := [c for c in r.get("exit_codes", []) if c != 0]:
        rows.append(("check", ring, wl, bits, "process", "failed_runs", f"{len(bad)}/{len(r['exit_codes'])} (exit {bad[0]})", ""))
    kg = res.get(f"ring{ring}/keygen/{bits}")
    if kg and wl != "keygen":
        rows.append(("timing", ring, wl, bits, "process", "median_net", f"{r['median'] - kg['median']:.3f}", "s"))

    # Per-op timings printed by print_duration (Utils.h:30): "S:MS sec" or "M.S:MS".
    # A section starts at its op title, and "Expected:"/"Obtained:" lines are compared as strings.
    log = out / "logs" / (name.replace("/", "_") + ".log")
    if not log.exists():
        continue
    ops, checks, expected, seen = {}, [], {}, {}
    ansi = re.compile(r"\x1b\[[0-9;]*m")  # Logger.h colors every line
    for line in ansi.sub("", log.read_text(errors="replace")).splitlines():
        if line.startswith("=== run"):
            seen = {}  # the same title can repeat in a run (two Quotients, two Multiplications)
        elif m := re.match(r"\s*Expected:\s*(.*)", line):
            expected["v"] = m[1].strip()
        elif (m := re.match(r"\s*Obtained:\s*(.*)", line)) and "v" in expected:
            got = m[1].strip().replace("-0", "0")
            checks.append(got == expected.pop("v").replace("-0", "0"))
        elif m := re.match(r"(.*?) took:\s*(?:(\d+)\.)?(\d+):(\d+)", line):
            secs = int(m[2] or 0) * 60 + int(m[3]) + int(m[4]) / 1000
            t = m[1].strip(); seen[t] = seen.get(t, 0) + 1
            ops.setdefault(t if seen[t] == 1 else f"{t} #{seen[t]}", []).append(secs)
    for op, ts in ops.items():
        rows.append(("op", ring, wl, bits, op, "median_internal", f"{statistics.median(ts):.3f}", "s"))
    if checks:
        rows.append(("check", ring, wl, bits, "expected_vs_obtained", "pass_fraction",
                     f"{sum(checks)}/{len(checks)}", ""))

for line in (out / "env.txt").read_text().splitlines():
    k, _, v = line.partition("=")
    rows.append(("meta", "", "", "", k, "value", v, ""))

with (out / "results.csv").open("w", newline="") as f:
    csv.writer(f).writerows(rows)
print(f"\nwrote {out/'results.csv'} ({len(rows)-1} rows)")
PYEOF

echo; echo "done. results in $OUTDIR/"
