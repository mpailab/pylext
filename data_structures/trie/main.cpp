#include "TestTrie.h"
#include "Trie.h"

int main() {
    const std::string kwfile = "tests/kw.txt";
    const std::string textfile = "tests/text.txt";
    test_trie<TrieM<int>>("TrieM", kwfile, textfile);
}
