#include "Min_heap.h"
#include <cassert>
#include <iostream>

#ifdef NDEBUG
#error Tests require assertions enabled.
#endif

int main() {
    ProductMinHeap heap;
    assert(heap.empty());
    assert(heap.top() == nullptr && heap.pop() == nullptr);
    Product late{"P00004", "Item", "2026-01-01", "2026-01-01", "2099-02-01", "AVAILABLE"};
    Product arrivedLate{"P00003", "Item", "2026-01-01", "2026-01-03", "2099-01-01", "AVAILABLE"};
    Product idLater{"P00002", "Item", "2026-01-01", "2026-01-01", "2099-01-01", "AVAILABLE"};
    Product first{"P00001", "Item", "2026-01-01", "2026-01-01", "2099-01-01", "AVAILABLE"};
    heap.push(&late);
    heap.push(&arrivedLate);
    heap.push(&idLater);
    heap.push(&first);
    assert(heap.size() == 4 && heap.top() == &first);
    assert(heap.top() == &first && heap.size() == 4);
    assert(heap.pop() == &first);
    assert(heap.pop() == &idLater);
    assert(heap.pop() == &arrivedLate);
    assert(heap.pop() == &late);
    assert(heap.empty() && heap.pop() == nullptr);
    heap.push(&late);
    assert(heap.top() == &late && heap.pop() == &late);
    assert(heap.empty());
    std::cout << "PASS: MinHeap empty, priority/date/arrival/ID, peek, pop, reuse\n";
}
