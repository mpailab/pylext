#include "TestTrie.h"
#include "Trie.h"

int main() {
    const std::string kwfile = "tests/kw.txt";
    const std::string textfile = "tests/text.txt";
    test_trie<TrieM<int>>("TrieM", kwfile, textfile);
    test_trie<TrieUM<int>>("TrieUM", kwfile, textfile);
    //test_trie<TrieL<int>>("TrieL", kwfile, textfile);
    test_trie<TrieV<int>>("TrieV", kwfile, textfile);
    test_trie<TrieVP<int>>("TrieVP", kwfile, textfile);
    test_trie<TrieCUM<int>>("TrieCUM", kwfile, textfile);
}
