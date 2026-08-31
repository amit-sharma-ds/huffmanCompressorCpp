CXX ?= c++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Wpedantic -DNDEBUG -frandom-seed=huffman-cpp -ffile-prefix-map=$(CURDIR)=.

.PHONY: build test clean verify-repro deps-proof
build: huffman
huffman: huffman.cpp
	$(CXX) $(CXXFLAGS) -s -o $@ $<
test: huffman_tests
	./huffman_tests
huffman_tests: tests/test_huffman.cpp huffman.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests/test_huffman.cpp
verify-repro:
	$(MAKE) clean
	$(MAKE) build
	sha256sum huffman > .huffman.sha.first
	$(MAKE) clean
	$(MAKE) build
	sha256sum huffman > .huffman.sha.second
	diff -u .huffman.sha.first .huffman.sha.second
	@echo "Reproducible: identical SHA-256 hashes."
deps-proof: huffman
	ldd huffman > deps-proof.txt || true
clean:
	rm -f huffman huffman_tests .huffman.sha.first .huffman.sha.second
