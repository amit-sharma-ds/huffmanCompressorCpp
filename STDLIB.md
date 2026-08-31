# Standard-library substitution log

| Common dependency/package | Replacement here | Rationale |
|---|---|---|
| zlib/miniz/libzip | Hand-rolled Huffman codec plus `std::priority_queue` | Compression is implemented from first principles without a codec library. |
| Bit-manipulation/bitstream library | `BitWriter` and `BitReader` | Eight-bit packing and unpacking are small, auditable local code. |
| CLI parser library (CLI11/cxxopts) | Raw `argc`/`argv` parsing | The two-command interface needs no dependency-heavy parser. |
| Binary serialization library | Custom HFM1 binary header | The fixed header directly embeds mode, size, and frequencies. |
| Benchmark framework | `std::chrono::steady_clock` | Monotonic standard timing is enough for CLI elapsed-time reporting. |
| Heap/min-queue library | `std::priority_queue` | The standard adaptor builds the Huffman min-heap via a comparator. |
| Manual memory helpers | `std::unique_ptr` and `std::make_unique` | Tree ownership is explicit and automatic, with no `new`/`delete`. |
| File-size utility package | `std::filesystem::file_size` | Standard filesystem APIs obtain input/output sizes portably. |
| File I/O wrapper | `std::ifstream` / `std::ofstream` | Binary files are read and written using RAII streams. |
| Dynamic array/byte-buffer library | `std::vector` and `std::array` | Input bytes and 256-symbol tables have standard containers. |
| Error-handling framework | `std::runtime_error` plus `try`/`catch` | Errors become clear stderr messages and reliable nonzero exits. |
| Unit-test framework | Small assert-based test executable | The test suite remains dependency-free and exercises real round trips. |
| Hashing library for build proof | External `sha256sum` (development-only) | Build verification uses a ubiquitous tool; it is not shipped or linked. |
| Platform dependency inspector | External `ldd` (development-only) | It records dynamic system libraries without changing the program. |

Only development commands (`sha256sum`, `diff`, `ldd`, and `make`) sit outside the runtime binary. The shipped compressor itself has no third-party dependency.
