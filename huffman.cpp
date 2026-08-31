// huffman.cpp — a single-file, zero-dependency Huffman compressor (C++23)
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace huffman {

// ===== BITSTREAM I/O =====
class BitWriter {
 public:
  explicit BitWriter(std::ostream& out) : out_(out) {}
  void write_bit(bool bit) {
    byte_ = static_cast<std::uint8_t>((byte_ << 1U) | (bit ? 1U : 0U));
    if (++used_ == 8) flush_byte();
  }
  void finish() { if (used_ != 0) { byte_ <<= (8 - used_); flush_byte(); } }
 private:
  void flush_byte() { out_.put(static_cast<char>(byte_)); byte_ = 0; used_ = 0; }
  std::ostream& out_; std::uint8_t byte_ = 0; unsigned used_ = 0;
};

class BitReader {
 public:
  explicit BitReader(std::istream& in) : in_(in) {}
  bool read_bit() {
    if (left_ == 0) {
      const int value = in_.get();
      if (value == std::char_traits<char>::eof()) throw std::runtime_error("truncated compressed bitstream");
      byte_ = static_cast<std::uint8_t>(value); left_ = 8;
    }
    return ((byte_ >> --left_) & 1U) != 0;
  }
 private:
  std::istream& in_; std::uint8_t byte_ = 0; unsigned left_ = 0;
};

// ===== BINARY HEADER =====
template <class T> void write_le(std::ostream& out, T value) {
  for (unsigned i = 0; i < sizeof(T); ++i) out.put(static_cast<char>((value >> (i * 8U)) & 0xffU));
  if (!out) throw std::runtime_error("write error");
}
template <class T> T read_le(std::istream& in) {
  T value = 0;
  for (unsigned i = 0; i < sizeof(T); ++i) {
    const int c = in.get(); if (c == std::char_traits<char>::eof()) throw std::runtime_error("truncated header");
    value |= static_cast<T>(static_cast<std::uint8_t>(c)) << (i * 8U);
  }
  return value;
}

// ===== HUFFMAN TREE =====
struct Node {
  std::uint64_t frequency; int symbol; std::uint32_t order; std::unique_ptr<Node> left, right;
  Node(std::uint64_t f, int s, std::uint32_t o) : frequency(f), symbol(s), order(o) {}
  bool leaf() const { return !left && !right; }
};
struct NodeBefore {
  bool operator()(const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) const {
    if (a->frequency != b->frequency) return a->frequency > b->frequency;
    return a->order > b->order; // deterministic ties make output reproducible
  }
};
using Frequencies = std::array<std::uint64_t, 256>;

std::unique_ptr<Node> make_tree(const Frequencies& frequencies) {
  std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, NodeBefore> heap;
  std::uint32_t order = 0;
  for (int i = 0; i < 256; ++i) if (frequencies[i]) heap.push(std::make_unique<Node>(frequencies[i], i, order++));
  if (heap.empty()) return {};
  while (heap.size() > 1) {
    auto left = std::move(const_cast<std::unique_ptr<Node>&>(heap.top())); heap.pop();
    auto right = std::move(const_cast<std::unique_ptr<Node>&>(heap.top())); heap.pop();
    auto parent = std::make_unique<Node>(left->frequency + right->frequency,
                                         left->symbol < right->symbol ? left->symbol : right->symbol, order++);
    parent->left = std::move(left); parent->right = std::move(right); heap.push(std::move(parent));
  }
  auto root = std::move(const_cast<std::unique_ptr<Node>&>(heap.top())); heap.pop(); return root;
}
void make_codes(const Node* node, std::vector<std::uint8_t>& path,
                std::array<std::vector<std::uint8_t>, 256>& codes) {
  if (node->leaf()) { codes[node->symbol] = path.empty() ? std::vector<std::uint8_t>{0} : path; return; }
  path.push_back(0); make_codes(node->left.get(), path, codes);
  path.back() = 1; make_codes(node->right.get(), path, codes); path.pop_back();
}

// ===== ENCODE =====
struct Result { std::uint64_t input_bytes = 0, output_bytes = 0; double seconds = 0; bool raw = false; };
std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary); if (!in) throw std::runtime_error("cannot open input: " + path.string());
  const auto size = std::filesystem::file_size(path); std::vector<std::uint8_t> data(static_cast<size_t>(size));
  if (size) in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
  if (!in && size) throw std::runtime_error("failed reading input: " + path.string()); return data;
}
Result compress_file(const std::filesystem::path& input, const std::filesystem::path& output) {
  const auto started = std::chrono::steady_clock::now(); auto data = read_file(input); Frequencies freq{};
  for (auto b : data) ++freq[b]; auto tree = make_tree(freq);
  std::array<std::vector<std::uint8_t>, 256> codes{}; std::vector<std::uint8_t> path;
  if (tree) make_codes(tree.get(), path, codes);
  std::uint64_t bit_count = 0; for (int i = 0; i < 256; ++i) bit_count += freq[i] * codes[i].size();
  constexpr std::uint64_t base_header = 4 + 1 + 8, huffman_header = base_header + 256 * 8;
  const bool raw = data.size() + base_header <= huffman_header + (bit_count + 7) / 8;
  std::ofstream out(output, std::ios::binary | std::ios::trunc); if (!out) throw std::runtime_error("cannot open output: " + output.string());
  out.write("HFM1", 4); out.put(raw ? 0 : 1); write_le<std::uint64_t>(out, data.size());
  if (raw) out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  else { for (auto f : freq) write_le<std::uint64_t>(out, f); BitWriter bits(out); for (auto b : data) for (auto bit : codes[b]) bits.write_bit(bit); bits.finish(); }
  if (!out) throw std::runtime_error("write error: " + output.string());
  return {data.size(), static_cast<std::uint64_t>(std::filesystem::file_size(output)), std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count(), raw};
}

// ===== DECODE =====
Result decompress_file(const std::filesystem::path& input, const std::filesystem::path& output) {
  const auto started = std::chrono::steady_clock::now(); std::ifstream in(input, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open input: " + input.string()); char magic[4]{}; in.read(magic, 4);
  if (in.gcount() != 4 || std::string(magic, 4) != "HFM1") throw std::runtime_error("not an HFM1 compressed file");
  const int mode = in.get(); if (mode != 0 && mode != 1) throw std::runtime_error("unsupported storage mode");
  const auto original_size = read_le<std::uint64_t>(in); std::ofstream out(output, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot open output: " + output.string());
  if (mode == 0) {
    std::uint64_t written = 0; std::array<char, 8192> buffer;
    while (written < original_size) {
      const auto wanted = static_cast<std::streamsize>(std::min<std::uint64_t>(buffer.size(), original_size - written));
      in.read(buffer.data(), wanted); const auto got = in.gcount();
      if (got != wanted) throw std::runtime_error("truncated raw payload");
      out.write(buffer.data(), got); written += static_cast<std::uint64_t>(got);
    }
  }
  else {
    Frequencies freq{}; for (auto& f : freq) f = read_le<std::uint64_t>(in); auto tree = make_tree(freq);
    std::uint64_t total = 0; for (auto f : freq) total += f; if (total != original_size || (original_size && !tree)) throw std::runtime_error("invalid frequency header");
    if (tree && tree->leaf()) { for (std::uint64_t i = 0; i < original_size; ++i) out.put(static_cast<char>(tree->symbol)); }
    else if (tree) { BitReader bits(in); for (std::uint64_t n = 0; n < original_size; ++n) { const Node* node = tree.get(); while (!node->leaf()) node = bits.read_bit() ? node->right.get() : node->left.get(); out.put(static_cast<char>(node->symbol)); } }
  }
  if (!out) throw std::runtime_error("write error: " + output.string());
  return {static_cast<std::uint64_t>(std::filesystem::file_size(input)), static_cast<std::uint64_t>(std::filesystem::file_size(output)), std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count(), mode == 0};
}
} // namespace huffman

// ===== CLI / MAIN =====
#ifndef HUFFMAN_NO_MAIN
int main(int argc, char* argv[]) {
  if (argc != 4 || (std::string(argv[1]) != "compress" && std::string(argv[1]) != "decompress")) {
    std::cerr << "Usage: " << argv[0] << " <compress|decompress> <input_file> <output_file>\n"; return 2;
  }
  try {
    const bool compress = std::string(argv[1]) == "compress";
    const auto r = compress ? huffman::compress_file(argv[2], argv[3]) : huffman::decompress_file(argv[2], argv[3]);
    std::cout << (compress ? "Compressed" : "Decompressed") << " " << r.input_bytes << " -> " << r.output_bytes << " bytes";
    if (compress && r.input_bytes) std::cout << " (" << (100.0 * r.output_bytes / r.input_bytes) << "% of original";
    if (compress && r.raw) std::cout << " [raw fallback]"; std::cout << " in " << r.seconds << " s\n"; return 0;
  } catch (const std::exception& e) { std::cerr << "huffman: " << e.what() << '\n'; return 1; }
}
#endif
