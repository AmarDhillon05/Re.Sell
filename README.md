# Re.Sell

A C++ command-line tool for tracking sneaker resale prices. It pulls sale history from the
[KicksDB](https://kicks.dev) API, stores it locally in an on-disk B-tree, plots it in the
terminal, and fits a price model with a Hamiltonian Monte Carlo sampler written from
scratch.

```
$ ./main demo
Fitted demo shoe
  observations   240
  mean price     $318.27  (+/- 2.26)
  volatility     $32.25  (+/- 1.45)
  typical range  $255.07 - $381.47
  (2000 draws, acceptance 0.57)
```

## What's in it

| Part | Where | What it does |
|---|---|---|
| HMC sampler | `models/hmc.cpp` | Samples the posterior of a Normal(μ, σ) price model using leapfrog integration, burn-in and step-size adaptation. Written against the [Stan reference](https://mc-stan.org/docs/reference-manual/mcmc.html#hamiltonian-monte-carlo), with no stats libraries. |
| Local store | `database/dbclient.cpp` | Paged B-tree on disk with fixed-size records keyed by title. Branching factor comes from `database/config.json`, and a versioned header rejects stale files instead of misreading them. |
| API client | `clients/` | HTTPS client for KicksDB. Parsing lives in `kicksdb_parse.cpp`, apart from the network code, so it can be tested offline against a saved response. |
| Terminal plots | `plot/plot.cpp` | ASCII scatter and line plots of sale history and posterior draws. |

## How the sampler is checked

The posterior has a closed form in this model, which makes it a good test target. The suite
compares the sampler against things that are known exactly:

- **Gradients**: the analytic gradient of the potential matches central finite differences.
- **Recovery**: on synthetic Normal data, the posterior means land on the sample moments
  and near the generating parameters.
- **Uncertainty**: posterior standard deviations track the analytic standard errors,
  σ/√n for μ and σ/√(2n) for σ.
- **Robustness**: it works across price scales several orders of magnitude apart, every
  draw stays in the support (σ > 0), the chain actually moves, and a fixed seed reproduces
  the same chain.

The suite also covers the B-tree (inserts, splits, persistence), the plotter, and parsing
of a real KicksDB response, including malformed and partial records.

## Build and run

Requires g++ with C++17, `make`, `curl`, and OpenSSL headers (`sudo apt install libssl-dev`).
The two single-header dependencies, nlohmann/json and cpp-httplib, are downloaded at pinned
versions into `include/` on first build.

```sh
make test     # 54 tests; no network or API key needed
make          # builds ./main
./main demo   # plotter and sampler on synthetic data, offline
```

For live data, put `KICKSDB_API_KEY=...` in a `.env` file, then:

```sh
./main track "jordan 4 black cat"   # search, pull sale history, fit, save
./main history "jordan 4 black cat" # plot the stored sale history
./main list                         # everything tracked locally
```

`./main` with no arguments lists every command.

## Limitations

- The model is a single Normal over sale prices. It ignores time, size and condition, so
  it describes where a shoe has traded, not where it will trade.
- With fewer than about 10 sales, the flat prior dominates and the fitted range is wide. The CLI
  prints a warning in that case.
- For this model the posterior could be computed analytically. The sampler is here because
  it generalizes to models that can't, and the closed form is what makes it testable.
