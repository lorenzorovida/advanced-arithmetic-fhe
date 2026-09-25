# Code map: advanced-arithmetic-fhe

CKKS (OpenFHE) integer and boolean arithmetic for 8 to 256-bit words. Paper: ia.cr/2026/450.
A single C++17 binary, `AdvancedFHE`. The rest is data, notebooks and results.

## Build
- `./scripts/install.sh` (verified on Debian with gcc 16.2, about 5.5 min on 4 cores). It builds the **OpenFHE fork**
  (github.com/lorenzorovida/openfhe-development-chebyshevSIMD, branch `repeated_poly_and_stcboot`, pinned SHA) into `.deps/openfhe`
  without sudo, then builds `build/AdvancedFHE` and runs `--test`. Stock OpenFHE lacks `EvalChebyshevSeriesPSBatchRepeated`
  and `EvalBootstrapStCFirstBits`.
- Fork traps the script works around: (1) `-Wall -Werror` is hardcoded, and gcc 16 trips `-Wsign-compare`, so it is stripped with `sed`.
  (2) `CMAKE_INSTALL_RPATH` is set from an undefined `LIBINSTALL`, so it passes `-DLIBINSTALL`; otherwise libOPENFHEpke cannot find binfhe.
  (3) `cmake --install` skips "up-to-date" files, so the prefix is wiped first.
- **Run from `build/`**: the coefficient paths are hardcoded as `../coeffs/...`. `read_vector_file` only prints an error
  on a missing file and then carries on with empty coefficients.
- CI (`.github/workflows/c-cpp.yml`) only triggers on the `build` branch and runs a stale binary name (`FlexibleIntsCKKS`).

## Benchmarks
`./scripts/run_benchmarks.sh [outdir] [runs]`, with env `RINGS` (12..16), `BITS`, `WORKLOADS` (ops, decompose, uniswapv3).
It runs hyperfine per (ring, workload, bits), subtracts `--test` keygen, and parses the per-op "took" lines and the Expected/Obtained
pairs from `--verbose 3` output. Output is `results.csv`.
Compiler: `install.sh` defaults to clang with libomp. With gcc, libgomp rebuilds its thread team on every change in width
(openfhe #1300): ring-14 uniswapv3 at 128 threads took 954 s with gcc and 283 s with clang.
Best machine measured: c4d-highcpu-32 (1-socket Zen5), with 16 threads bound one per core. Ring 14 uniswapv3 took 150 s there,
ring 16 took 709 s, and SMT made it 26% slower. On n2-standard-128, 4 concurrent 16-thread processes, each bound to its own socket, ran only about 5% slower each.
Threads: it defaults to `OMP_NUM_THREADS=16`. Ring-14 uniswapv3 on n2-standard-128 took 954 s at 128 threads versus 262 s at 16,
and 32 threads, pinned or not, gave 263 s. `-march=native` gave no gain. `PROFILE=1 ./scripts/install.sh` adds `[profile]` per-step timers.
At 16 threads the time splits into bootstrap 54%, other homomorphic ops about 30%, Chebyshev 10%, encoding 4%, and setup plus LUT reads 2%. At ring 12 with 4 cores: ops/16 about 185 s, decompose about 9 s.

## Known correctness issues (upstream, as of fa364f7)
- fa364f7 left a debug `return x;` in ct/ct `div_integer`, which returned the Newton hint. Removed on branch `jpp_improvements`.
- At ring 12 / 16 bits: plaintext division (`a / 42`) is correct only in slot 0 (multi-slot is not implemented, and not needed: uniswap uses zslots=1),
  and `square_root_integer` gives wrong values (smoke run, 8/10 checks passed).
- Fixed on `jpp_improvements`, all Linux or driver bugs on the uniswap path:
  - 7b213dc: the 128-bit division LUTs were named `-BITS-`, but the code loads `-bits-`. That only works on case-insensitive macOS.
  - da324e5: `Reciprocal.py` needed an undocumented numpy, so plaintext division silently used an empty reciprocal.
  - 82945bd: the uniswapv3 driver never zero-padded `m_fx`, so it was encrypted with 8192 slots and `binboot(m_fx - X_post)`
    aborted in `GetBootPrecom()`. term2, u and X_post were already correct before the abort. Also `g_den_Y_prec` wrapped in uint64 (unused).
- GPU port: github.com/lorenzorovida/FIDESlib-chebyshevSIMD, branch `uniswapv3`, with host program lorenzorovida/advanced-arithmetic-fhe-cuda.
  Fixes, plus level diagnostics for the Newton-loop multiply, are on local branches `jpp_gpu_uniswap_fix` in
  /workspace/jopasserat/{FIDESlib-chebyshevSIMD,advanced-arithmetic-fhe-cuda}. See UNISWAPV3_GPU_FIX.md there.

## TODO
- [ ] **GPU segfault at exit** after the batched ops run (not investigated yet). Diagnose with
      `cd build && gdb -batch -ex run -ex bt --args ./AdvancedFHEGPU --ring 16 --bits 128`. The suspect is static GPU caches
      being destroyed after the CUDA context. Details are in FIDESlib-chebyshevSIMD/UNISWAPV3_GPU_FIX.md (branch jpp_gpu_uniswap_fix).
- GPU benchmarks: advanced-arithmetic-fhe-cuda `scripts/run_benchmarks.sh` (branch jpp_gpu_uniswap_fix). Same layout as ours,
  plus peak GPU memory and the in-program hot-cache "took" timings.

## Layout
| Path | What |
|---|---|
| `scripts/` | `install.sh`, `run_benchmarks.sh` |
| `src/main.cpp` | CLI and experiments. Globals at :13-35, `main` :53, `read_arguments` :1151 |
| `src/CKKSController.{h,cpp}` | Wrapper around `CryptoContext`: context/keygen, the integer ops, bootstrapping, Chebyshev |
| `src/Utils.h` | Header-only helpers: bit/int conversion, plaintext `*_simd` reference ops, `read_vector_file` :227, `print_duration` :30, plaintext `ascon_permutation` :472 |
| `src/Logger.h` | `log(level)` colored stream gated by `verbose` |
| `src/Reciprocal.py` | Python helper for 257-bit reciprocal constants |
| `coeffs/Decompose8bits/p{1..8}-451.txt` | Chebyshev coefficients, degree 451, 8 polys: [0,255] to its bits (used by `--decompose`) |
| `coeffs/p{1..8}-norm-369.txt` | Older degree-369 decomposition polys |
| `coeffs/LUTs/<N> bits/division/` | Division LUTs per word size (about 500 files) |
| `notebooks/` | numpy reference implementations of the algorithms (see `notebooks/README.md`) |
| `results/` | Benchmark logs (M5 Max) |
| `shoptest.txt`, `imgs/` | Noise: a shell-setup scratch file, and README gif frames |

## CLI flags (`read_arguments`, main.cpp:1151)
`--ring <logN>` (default 12), `--bits <w>` (default 32), `--verbose <0-3>`, `--test`, `--input`,
and the experiment switches below. Each experiment runs, then calls `exit(0)`. Precedence order:
`--mev`, `--hash` (Ascon), `--noise`, `--itob`, `--btoi-itob`, `--uniswapv3`, `--decompose`.
With no switch it runs `random_operations_batched(wordsize)` (:890). `--input` runs `random_operations` (:1014).

Setup in `main()` that happens **before** any experiment, whatever the flags:
`generate_context_for_bootstrapping(2^ring, 14)`, followed by rotation keys and precomputations for add/mul/bit_length at `wordsize`.

## CKKSController.cpp hot spots
- Context :3. SPARSE_ENCAPSULATED secret, `HEStd_NotSet` (**no security guarantee at any ring size**), dcrtBits 36, firstMod 41,
  FLEXIBLEAUTO, 3 large digits, depth = 14 + boot depth {3,3}, full slots (N/2). Bootstrapping setup :43.
  **Security:** log QP = 1542 (31 Q + 7 P towers) at every N. With the same parameters under `HEStd_128_classic`, OpenFHE
  accepts N = 2^16 and 2^17 and rejects 2^14 and 2^15 ("does not comply with HE standards recommendation (65536)").
  So only `--ring 16` results are at 128-bit; smaller rings are fine for relative comparisons only.
- `clean` :369, `add_integer` :475, `sub_integer` :518, `mul_integer` :766, `shf_integer` :888, `bit_length` :905,
  `blind_rotation` :957, `div_integer` :984/:1193/:1222, `square_root_integer` :1253, `eq_integer` :1678,
  `ascon_permutation` :1710, `bootstrap` :1935, `binboot` :1943 (StC-first bits bootstrap), `chebyshev*` :1950-1960.

## Experiments in main.cpp
`experiment_hash_ascon` :121, `experiment_mev` :219, `experiment_decompose` :276, `experiment_uniswap_v3` :329,
`experiment_squareroot` :446, `experiment_division` :469, `experiment_noise_estimate` :497, `experiment_BtoI_ItoB` :684, `experiment_ItoB` :814.

### Naming across repos
The `--uniswapv3` experiment is the same computation as `backrun-sqrtfree` in fhe_backrun (TFHE): the sqrt-free
Uniswap V3 back-run amount after the 3.75 ETH user swap. `--mev` is the V2 back-run with a square root, which
corresponds to `backrun-newton`; it is not benchmarked yet. On c4d-highcpu-64 at N=2^16, TFHE with AVX-512 took 61.6 s
and CKKS with 32 threads took 622.0 s.

### `--decompose` (main.cpp:276)
zslots = N/16. It encrypts N/2 slots, laid out as zslots random bytes in [0,255], each repeated 8 times. It then calls
`EvalChebyshevSeriesPSBatchRepeated` with the 8 degree-451 polys over [0,255], so slot j of each group gets bit j.
That is followed by `binboot`, and it prints the first 100 slots before and after, plus the timing. `--bits` has no effect on the
experiment itself, but it still sizes the rotation keys generated up front (default 32).
