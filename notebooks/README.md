# Notebooks

This folder contains notebooks that implement the algorithm used in the encrypted domain, but using Python and numpy vectors. This can be useful to better understand the procedures underlying the framework.

- `64-bits multiplier (readability).ipynb`: this notebook contains the non-optimized implementation.
- `Plain framework (optimized).ipynb`: on the other hand, this implementation is optimized to execute a single masking operation before the base case. This is the version that is ultimately implemented in OpenFHE.

N.b. the two versions, although written differently, yield the same result (and contain the same logic in terms of ideas)
