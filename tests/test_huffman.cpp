#define HUFFMAN_NO_MAIN
#include "../huffman.cpp"
#include <cassert>
#include <fstream>
#include <iostream>

int main() {
  const auto dir = std::filesystem::temp_directory_path() / "huffman_cpp_tests";
  std::filesystem::create_directories(dir);
  const std::vector<std::vector<std::uint8_t>> cases = {
    {}, {'h','e','l','l','o',' ','h','u','f','f','m','a','n','\n'},
    std::vector<std::uint8_t>(10000, 0xA5),
    [] { std::vector<std::uint8_t> x(4096); std::uint32_t s=0x12345678; for (auto& b:x) { s=s*1664525U+1013904223U; b=static_cast<std::uint8_t>(s>>24); } return x; }(),
    [] { std::vector<std::uint8_t> x; for(int i=0;i<256;++i) x.push_back(static_cast<std::uint8_t>(i)); return x; }()
  };
  for (size_t i = 0; i < cases.size(); ++i) {
    auto source=dir/("source"+std::to_string(i)), packed=dir/("packed"+std::to_string(i)), restored=dir/("restored"+std::to_string(i));
    { std::ofstream out(source, std::ios::binary); out.write(reinterpret_cast<const char*>(cases[i].data()), cases[i].size()); }
    huffman::compress_file(source, packed); huffman::decompress_file(packed, restored);
    assert(huffman::read_file(restored) == cases[i]);
  }
  std::filesystem::remove_all(dir); std::cout << "All Huffman round-trip tests passed.\n";
}
