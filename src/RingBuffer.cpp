#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t n) : buffer(n,0.0f) {}

//handles wrapping when indexing, allows writing
float& RingBuffer::operator[](size_t i) {
    i = i % buffer.size();
    return buffer[i];
}

//handles wrapping for indexing, read only
const float RingBuffer::operator[](size_t i) const {
    i = i % buffer.size();
    return buffer[i];
}

//get a value from an offset relative to the head
float RingBuffer::getRelativeToHead(int offset) const {
    int index = (writeIndex + offset + buffer.size()) % buffer.size();
    return (*this)[index];
}

//pushes a value to the next spot in the ring buffer
void RingBuffer::push(float value) {
    (*this)[writeIndex] = value;
    writeIndex = (writeIndex + 1) % buffer.size();
}

