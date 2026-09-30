#ifndef TRIE_H
#define TRIE_H

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

    Trie(const Trie&) = delete; // Cấm tạo một Trie mới bằng cách sao chép Trie cũ.
    Trie& operator=(const Trie&) = delete; // Cấm gán một Trie vào Trie khác.

    void insert(const string& productName, const string& productId);
    vector<string> searchByPrefix(const string& prefix);
    bool remove(const string& productName, const string& productId);
};

#endif
