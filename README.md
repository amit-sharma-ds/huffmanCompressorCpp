# huffman-compressor-cpp

**Hackathon:** Zero Dependency Hackathon (Aug 28–31, 2026)
**Author:** Amit Sharma
**Track:** F — Open / Wildcard

A C++23 file compressor/decompressor, built from scratch in one file (`huffman.cpp`), using Huffman coding. Zero third-party dependencies.

> A reasonable engineer assumes compression needs zlib. This is built purely on libc/STL — `priority_queue`, `fstream`, manual bitpacking.

---

## Problem it solves

Every language reaches for a package (`zlib`, `libzip`) to shrink files. This tool proves the same result — smaller files, correctly restored — is achievable with nothing but the standard library, giving a fully portable, dependency-free compressor.

## Tech stack

- **Language:** C++23
- **Libraries:** none — only `<fstream>`, `<queue>`, `<filesystem>`, `<chrono>`, `<memory>`
- **Build:** GNU Make + g++/clang++
- **Dependencies:** none (see `deps-proof.txt`)

## Package killed: zlib

zlib has millions of installs and is the default compression dependency almost everywhere. This isn't a DEFLATE-speed replacement — it's an honest, transparent compressor for when zero dependencies matter more than raw ratio.

## Build and run

```sh
make build
./huffman compress input.txt input.hfm
./huffman decompress input.hfm restored.txt
cmp input.txt restored.txt
make test
```

Exit codes: `0` success, `1` data/operational error, `2` bad CLI usage.

## How it works

1. Scan byte frequencies in the input
2. Build a Huffman tree with `std::priority_queue`
3. Pack bits manually into an `HFM1` file (magic + mode + size + frequency table + bitstream)

The frequency table is embedded, so decompression needs only the `.hfm` file. If Huffman encoding would make the file bigger, it falls back to raw storage automatically.

**Honest limits:** static byte-level Huffman only, no LZ77 — so it won't beat `gzip` on repetitive text. 2,061-byte header means tiny files are stored raw. No streaming, checksums, or encryption.

## gzip comparison (dev-only, not a dependency)

```sh
gzip -9 -c sample.txt > sample.txt.gz
wc -c sample.txt sample.hfm sample.txt.gz
```

## Reproducible build

Fixed `-frandom-seed`, stripped debug info, `-ffile-prefix-map` for paths — no timestamps baked in.

```sh
make verify-repro
```

Runs two clean builds and diffs their SHA-256 hashes.

```text
Build 1 SHA-256: <fill in on judging machine>
Build 2 SHA-256: <must match>
```

## Bonus challenges claimed

| Bonus | Pts | Where |
|-------|-----|-------|
| Single File | +5 | Whole implementation in `huffman.cpp` |
| Reproducible Build | +5 | `make verify-repro` |
| Package Killer | +3 | Replaces zlib, see `STDLIB.md` |
| STDLIB Log | +3 | 10+ substitutions in `STDLIB.md` |

## Layout

```text
huffman.cpp              implementation (single file)
tests/test_huffman.cpp   round-trip tests
Makefile                 build / test / repro
STDLIB.md                dependency substitutions
deps-proof.txt           ldd output
.zero-dep.toml           track metadata
```

## License

MIT
