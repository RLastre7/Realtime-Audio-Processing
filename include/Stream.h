#pragma once
#include "AudioState.h"
#include "portaudio.h"

enum StreamType {
    INPUT,
    OUTPUT,
};

struct Stream {
    static void recordInput(AudioState* audioState, unsigned long framesPerBuffer, const float* input, float* output);
    static void playRecording(AudioState* audioState, unsigned long framesPerBuffer, float* output);

    static int callback(const void* inputStream, void* outputStream,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
        void* userData);

    static PaDeviceIndex getDevice(StreamType streamType, bool useDefault);
    static PaStreamParameters setupStreamParameters(StreamType streamType, bool useDefault);
    static void cleanupStream(PaStream* stream);
};