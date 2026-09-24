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
pairs from `--verbose 3` output. Output is `results.csv`. At ring 12 with 4 cores: ops/16 about 185 s, decompose about 9 s.

## Known correctness issues (upstream, as of fa364f7)
- fa364f7 left a debug `return x;` in ct/ct `div_integer`, which returned the Newton hint. Removed on branch `jpp_improvements`.
- At ring 12 / 16 bits: plaintext division (`a / 42`) and `square_root_integer` give wrong values (smoke run, 8/10 checks passed).
- Fixed on `jpp_improvements`: the 128-bit division LUTs were named `-BITS-`, but the code loads `-bits-`, which only works
  on case-insensitive macOS (7b213dc). `Reciprocal.py` needed an undocumented numpy, so plaintext division silently used
  an empty reciprocal (da324e5).

- Fixed in 82945bd: the uniswapv3 driver never zero-padded `m_fx`, so it was encrypted with 8192 slots, and `binboot(m_fx - X_post)`
  aborted in `GetBootPrecom()` (no bootstrap keys for 8192 slots). term2/u/X_post were already correct before the abort.

## TODO
- [ ] **Plaintext division, multi-slot.** `div_integer(ct, uint128_t)` (CKKSController.cpp:1222) is correct only in slot 0.
      Every other slot comes out at about 0.66x the expected value (ring 12, 16 bits: 1173 correct, then 971 vs 1479, 144 vs 220, ...).
      Uniswap uses zslots=1, so it is unaffected. Suspect the zslots/replication of the reciprocal mask.
      Repro: `cd build && ./AdvancedFHE --ring 12 --bits 16 --verbose 3`, section "Quotient plaintext (a / 42)".
- [x] **`g_den_Y_prec` overflow** (fixed in 82945bd). main.cpp:360 has `1000000000000000000ULL * 1000`, which wraps in uint64 to 3875820019684212736
      instead of 10^21. Fix: `(uint128_t)1000000000000000000ULL * 1000`. It is encrypted and printed but never used, so results don't change.
- GPU port: github.com/lorenzorovida/FIDESlib-chebyshevSIMD, branch `uniswapv3`, with host program lorenzorovida/advanced-arithmetic-fhe-cuda.
  Fixes are on local branches `jpp_gpu_uniswap_fix` in /workspace/jopasserat/{FIDESlib-chebyshevSIMD,advanced-arithmetic-fhe-cuda}.

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
- `clean` :369, `add_integer` :475, `sub_integer` :518, `mul_integer` :766, `shf_integer` :888, `bit_length` :905,
  `blind_rotation` :957, `div_integer` :984/:1193/:1222, `square_root_integer` :1253, `eq_integer` :1678,
  `ascon_permutation` :1710, `bootstrap` :1935, `binboot` :1943 (StC-first bits bootstrap), `chebyshev*` :1950-1960.

## Experiments in main.cpp
`experiment_hash_ascon` :121, `experiment_mev` :219, `experiment_decompose` :276, `experiment_uniswap_v3` :329,
`experiment_squareroot` :446, `experiment_division` :469, `experiment_noise_estimate` :497, `experiment_BtoI_ItoB` :684, `experiment_ItoB` :814.

### `--decompose` (main.cpp:276)
zslots = N/16. It encrypts N/2 slots, laid out as zslots random bytes in [0,255], each repeated 8 times. It then calls
`EvalChebyshevSeriesPSBatchRepeated` with the 8 degree-451 polys over [0,255], so slot j of each group gets bit j.
That is followed by `binboot`, and it prints the first 100 slots before and after, plus the timing. `--bits` has no effect on the
experiment itself, but it still sizes the rotation keys generated up front (default 32).
