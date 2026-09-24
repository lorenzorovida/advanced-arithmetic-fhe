# A unified toolkit for advanced arithmetic in FHE

[![GitHub CI](https://github.com/lorenzorovida/advanced-arithmetic-fhe/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/lorenzorovida/advanced-arithmetic-fhe/actions/workflows/c-cpp.yml)
<br>
<a href="https://eprint.iacr.org/2026/450"><img src="imgs/preprint_icon.svg" alt="Link to the preprint PDF" ></a>

<!--<img src="imgs/console.gif" width="900"/>-->
<img src="imgs/terminal.gif" width="900">

<sup><sub>(To replicate this toy example, simply run `./AdvancedFHE --ring 12 --bits 64` after installation) </sub></sup>

---

This repository contains the implementation of the [A unified toolkit for advanced arithmetic in FHE](https://ia.cr/2026/450) paper. It allows to perform computation on integers of word sizes 16/32/64/128/256 bits. In particular, the supported operations are the following:
- Addition ($a+b$) 
- Subtraction ($a-b$)
- Multiplication ($a \cdot b$)
- Logical shift, or multiplication by $2^b$ ($a \ll b$)
- Quotient ($a/b$)
- Square root ($\sqrt{a}$)
- Blind logical shift (i.e., shift by $b$, with $b$ encrypted) 
- Bit length

Additionally, also binary operations are easily supported:
- AND ($a \land b$)
- XOR ($a \oplus b$)
- OR ($a \lor b$)
- NOT (~ $a$)

Notice that the code is meant to be used with high-level APIs, therefore users can use functions such as `add_integer`, `xor_boolean` or `sqrt_integer`. Of course,  an interested user can have a look into the functions to see the implementation of the techniques described in the paper. The latter description may be slighly simpler from the actual code for readability reasons, but feel free to open issues if you have any question.


| :exclamation: Keep in mind!                                  |
|:-------------------------------------------------------------|
| This repo is still WIP, so feel free to report any issue! ^^ |

## Fresh start
One of the key ideas behind this work is *simplicity*. It can indeed be used by simply installing a custom OpenFHE fork and by compiling the CMake project.

We require a custom fork of OpenFHE (that is updated to v1.5.1 and it includes the StC-first bootstrapping), this allows to use the two functions `EvalChebyshevSeriesPSBatchRepeated` and `EvalBootstrapStCFirstBits`. The first generalizes the functionality to evaluate a Chebyshev polynomial over a ciphertext to evaluate $n$ Chebyshev polynomials, one for each slot of the ciphertext. The second allows to evaluate a cleaning bootstrapping operation à la [[BCKS24]](https://eprint.iacr.org/2024/767).


### Scripted install (no sudo)

```
./scripts/install.sh
```

This clones the fork at a pinned commit into `.deps/src`, builds it without tests, examples or benchmarks, and installs it to `.deps/openfhe`.
It then builds `build/AdvancedFHE` against that install and runs `--test`. Knobs: `PREFIX=`, `OPENFHE_REF=`, `NATIVE=1` (`-march=native`), `JOBS=`.
It needs `git`, `cmake` (>= 3.23), a C++17 compiler with OpenMP (`build-essential`, plus `libomp-dev` for clang), and `python3`.
A 4-core machine takes about 6 minutes. Always run the binary from `build/`, because the coefficient files are loaded from `../coeffs`.

### Manual install

1) Install the `repeated_poly_and_stcboot` branch from [this](https://github.com/lorenzorovida/openfhe-development-chebyshevSIMD) custom fork of OpenFHE 

```
git clone --branch repeated_poly_and_stcboot --single-branch https://github.com/lorenzorovida/openfhe-development-chebyshevSIMD
cd openfhe-development-chebyshevSIMD
mkdir build && cd build
cmake .. -DBUILD_UNITTESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF
sudo make install
```


2. Now, navigate to your desired folder and install this repository

```
git clone https://github.com/lorenzorovida/flexible-integer-arithmetic-ckks
cd flexible-integer-arithmetic-ckks
mkdir build && cd build
```

CMake must find the fork, not a stock OpenFHE. A fork installed to `/usr/local` is found automatically. For any other prefix, pass `-DCMAKE_PREFIX_PATH=<prefix>` to `cmake`. Then build:
```
cmake ..
make
```



> [!IMPORTANT]  
> The compilation will fail if you are using the original version of OpenFHE, use the fork!


3. Finally, you can test if everything works by running

```
./AdvancedFHE --test
```


## Benchmarks

```
./scripts/run_benchmarks.sh [outdir] [runs]        # defaults: benchmarks/<utc-date>, 3
RINGS="12 14 16" BITS="32 64" WORKLOADS="ops decompose uniswapv3" ./scripts/run_benchmarks.sh
```

This uses [hyperfine](https://github.com/sharkdp/hyperfine) to time one process per (ring, workload, bits). By default it covers rings 12 to 16, the batched `ops` run at 64 bits, `--decompose`, and `--uniswapv3` (ring 14 and up only).
Key generation happens in every process, so `--test` is timed for each (ring, bits), and `median_net` subtracts it.
The script also parses the per-operation timings the program prints and checks each Expected/Obtained pair. Everything goes into `<outdir>/results.csv`, next to `env.txt`, `hyperfine.json` and the raw logs.
Parameters use `HEStd_NotSet`, so no ring size carries a security guarantee. Ring 16 needs tens of GB of RAM.

## Custom parameters
There are three parameters that can be passed to `./AdvancedFHE`

1) `--ring <size>`

Sets the (logarithm of the) ring size. `<size>` must be an integer in (12, 13, 14, 15, 16).
Example:
```
./AdvancedFHE --ring 16
```

2) `--bits <bits>` 

Sets the word size (number of bits per word). Only the following values are supported: 8, 16, 32, 64, 128, 256.
If an unsupported value is provided, the program will display an error.
Example:
```
./AdvancedFHE --bits 64
```

3) `--verbose <value>` 

Sets the verbosity level of program output. `<value>` must be either 0, 1 or 2. Higher numbers mean more detailed output.
Example:
```
./AdvancedFHE --verbose 2
```

4) `--input`

Sets the program in input mode, i.e., the program will ask you to set the number of desired bits and to manually insert the values you want to compute operations on.

5) Experiment switches: `--decompose`, `--uniswapv3`, `--mev`, `--hash` (Ascon), `--noise`, `--itob`, `--btoi-itob`. Each one runs a single experiment from `src/main.cpp` and then exits.

## Examples of usage

#### Toy parameters
You can test 64-bits operation, in the $N=2^{12}$ ring, with the following code:
```
./AdvancedFHE --ring 12 --bits 64
```
This allows to test the code in a unsecure environment, useful if playing around with the code.

#### More secure parameters
You can test 64-bits operation, in the $N=2^{16}$ ring, with the following code:
```
./AdvancedFHE --ring 16 --bits 64
```

#### Input mode
You can use the program and manually insert some values to operate on
```
./AdvancedFHE --ring 12 --input
```
