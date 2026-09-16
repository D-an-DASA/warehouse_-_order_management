#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct TrieNode {
    unordered_map<char, TrieNode*> children;
    vector<string> productIds;
    bool isEndOfName = false;
};

class Trie {
private:
    TrieNode* root;
    void collectAllIds(TrieNode* node, vector<string>& result);
    void clear(TrieNode* node);

public:
    Trie();
    ~Trie();

    void insert(const string& name, const string& id);
    vector<string> searchByPrefix(const string& prefix);
    bool remove(const string& name, const string& id);
};
