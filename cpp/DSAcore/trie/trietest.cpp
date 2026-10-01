#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "trie.h"

using namespace std;

int main() {
    Trie trie;
    trie.insert("sua tuoi", "P00001");
    trie.insert("sua tuoi", "P00002");
    trie.insert("sua chua", "P00003");
    trie.insert("banh mi", "P00004");

    vector<string> results = trie.searchByPrefix("sua");
    sort(results.begin(), results.end());
    assert((results == vector<string>{"P00001", "P00002", "P00003"}));

    assert(trie.remove("sua tuoi", "P00001"));
    assert(!trie.remove("sua tuoi", "P99999"));
    results = trie.searchByPrefix("sua tuoi");
    assert(results.size() == 1);
    assert(results[0] == "P00002");
    assert(trie.searchByPrefix("khong co").empty());

    cout << "TrieTest: PASS\n";
    return 0;
}
