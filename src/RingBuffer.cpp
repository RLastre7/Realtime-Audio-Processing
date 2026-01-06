#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t n) : buffer(n) {}

//handles wrapping when indexing, allows writing
float& RingBuffer::operator[](size_t i) {
    return buffer[i % buffer.size()];
}

//handles wrapping for indexing, read only
const float RingBuffer::operator[](size_t i) const {
    return buffer[i % buffer.size()];
}

//get a value from an offset relative to the head
float RingBuffer::getRelativeToHead(size_t offset) const {
    return (*this)[offset + writeIndex];
}

//pushes a value to the next spot in the ring buffer
void RingBuffer::push(float value) {
    (*this)[writeIndex] = value;
    writeIndex = (writeIndex + 1) % buffer.size();
}

