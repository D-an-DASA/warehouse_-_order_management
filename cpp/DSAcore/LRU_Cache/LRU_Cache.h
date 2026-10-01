#ifndef BOUNDED_CACHE_H
#define BOUNDED_CACHE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <ctime>
#include "../Product.h"
using namespace std;

struct CacheItem
{
    string time;
    string operation;
    Product product;
};

class LRU_Cache
{

private:
    struct Node
    {
        CacheItem item;
        Node *next = nullptr;
        Node *prev = nullptr;
    };

    Node *head = nullptr;
    Node *tail = nullptr;

    unordered_map<string, Node *> table;

    static const size_t MAX_SIZE = 50;

    string Cur_Time()
    {
        char Buffer[100];

        time_t now = time(nullptr);
        tm *infoTime = localtime(&now);

        strftime(Buffer, sizeof(Buffer), "%H:%M:%S", infoTime);

        string result(Buffer);
        return result;
    }

    void Add_Front(const Product &prod, const string &operation)
    {
        Node *new_node = new Node;

        new_node->item.product = prod;
        new_node->item.time = Cur_Time();
        new_node->item.operation = operation;

        table[prod.id] = new_node;

        // TH1: List rỗng
        if (head == nullptr)
        {
            head = new_node;
            tail = new_node;
            return;
        }

        // TH2: List không rỗng
        new_node->next = head;
        head->prev = new_node;
        head = new_node;
    }

    void Pop_Back()
    {
        Node *old_tail = tail;
        table.erase(old_tail->item.product.id);
        tail = tail->prev;
        tail->next = nullptr;
        delete old_tail;
    }

    void Move_Front(const Product &prod, const string &operation)
    {
        Node *cur = table[prod.id];

        cur->item.product = prod;
        cur->item.time = Cur_Time();
        cur->item.operation = operation;

        if (cur == head)
            return;
        if (cur == tail)
        {
            tail = cur->prev;
            tail->next = nullptr;
        }
        else
        {
            cur->next->prev = cur->prev;
            cur->prev->next = cur->next;
        }

        cur->prev = nullptr;
        cur->next = head;
        head->prev = cur;
        head = cur;
    }

public:
    LRU_Cache() {}

    LRU_Cache(const LRU_Cache&) = delete; // Cấm tạo một LRU_Cache mới bằng cách sao chép LRU_Cache cũ.
    LRU_Cache& operator=(const LRU_Cache&) = delete; // Cấm gán một LRU_Cache vào LRU_Cache khác.

    ~LRU_Cache()
    {
        Node *cur = head;

        while (cur != nullptr)
        {
            Node *next = cur->next;

            delete cur;

            cur = next;
        }
    }

    bool IsContain(const string &id) const
    {
        return table.find(id) != table.end();
    }

    void Put(const Product &prod, const string &operation)
    {
        if (IsContain(prod.id))
            Move_Front(prod, operation);
        else
        {
            Add_Front(prod, operation);
            if (Size() > MAX_SIZE)
                Pop_Back();
        }
    }

    bool Remove(const string &id)
    {
        if (!IsContain(id))
        {
            return false;
        }

        Node *cur = table[id];

        if (cur->prev != nullptr)
        {
            cur->prev->next = cur->next;
        }
        else
        {
            head = cur->next;
        }

        if (cur->next != nullptr)
        {
            cur->next->prev = cur->prev;
        }
        else
        {
            tail = cur->prev;
        }

        table.erase(id);
        delete cur;

        return true;
    }

    size_t Size() const
    {
        return table.size();
    }

    vector<CacheItem> GetAll() const
    {
        vector<CacheItem> result;
        Node *cur = head;

        while (cur != nullptr)
        {
            CacheItem item;
            item.product = cur->item.product;
            item.time = cur->item.time;
            item.operation = cur->item.operation;
            result.push_back(item);

            cur = cur->next;
        }

        return result;
    }
};

#endif
