#pragma once
#include <atomic>

class SimpleIDGenerator {
private:
    static std::atomic<uint64_t> nextId;

public:
    // Generate sequential IDs starting from 1
    static uint64_t generateId() {
        return nextId.fetch_add(1, std::memory_order_relaxed);
    }

    // Reset ID counter (mainly for testing)
    static void reset() {
        nextId.store(1);
    }
};

// Initialize static member
std::atomic<uint64_t> SimpleIDGenerator::nextId{1}; 