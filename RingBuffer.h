#pragma once
#include <vector>
struct RingBuffer {
    std::vector<float> buffer;
    size_t writeIndex = 0;

    RingBuffer() = default;

    RingBuffer(size_t n);

    //handles wrapping when indexing, allows writing
    float& operator[](size_t i);
    //handles wrapping for indexing, read only
    const float operator[](size_t i) const;

    //get a value from an offset relative to the head
    float getRelativeToHead(size_t offset) const;

    //pushes a value to the next spot in the ring buffer
    void push(float value);

};
