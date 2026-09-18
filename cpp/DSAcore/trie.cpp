#include "trie.h"

// 1. CONSTRUCTOR - Khởi tạo cây
Trie::Trie() {
    root = new TrieNode();
}

// 2. DESTRUCTOR - Giải phóng bộ nhớ
Trie::~Trie() {
    clear(root);
}

// 3. HÀM clear - Giải phóng đệ quy
void Trie::clear(TrieNode* node) {
    if (!node) return;
    for (auto& pair : node->children) {
        clear(pair.second);
    }
    delete node;
}

// 4. HÀM insert - Thêm sản phẩm vào Trie
void Trie::insert(const string& productName, const string& productId) {
    TrieNode* current = root;
    for (char c : productName) {
        if (current->children.find(c) == current->children.end()) {
            current->children[c] = new TrieNode();
        }
        current = current->children[c];
    }
    current->isEndOfName = true;
    current->productIds.push_back(productId);
}

// 5. HÀM collectAllIds - Thu thập ID đệ quy
void Trie::collectAllIds(TrieNode* node, vector<string>& result) {
    if (!node) return;

    if (node->isEndOfName) {
        for (const string& id : node->productIds) {
            result.push_back(id);
        }
    }

    for (auto& pair : node->children) {
        collectAllIds(pair.second, result);
    }
}

// 6. HÀM searchByPrefix - Tìm kiếm theo tiền tố
vector<string> Trie::searchByPrefix(const string& prefix) {
    TrieNode* current = root;

    for (char c : prefix) {
        if (current->children.find(c) == current->children.end()) {
            return {};
        }
        current = current->children[c];
    }

    vector<string> result;
    collectAllIds(current, result);
    return result;
}

// 7. HÀM remove - Xóa sản phẩm khỏi Trie
bool Trie::remove(const string& productName, const string& productId) {
    TrieNode* current = root;
    for (char c : productName) {
        if (current->children.find(c) == current->children.end()) {
            return false;
        }
        current = current->children[c];
    }

    auto& ids = current->productIds;
    for (auto it = ids.begin(); it != ids.end(); ++it) {
        if (*it == productId) {
            ids.erase(it);
            if (ids.empty()) {
                current->isEndOfName = false;
            }
            return true;
        }
    }
    return false;
}
