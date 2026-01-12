#pragma once
#include "AudioState.h"
#include "portaudio.h"
#include <unordered_set>

enum StreamType {
    INPUT,
    OUTPUT,
};

struct Stream {
    
    static void recordInput(AudioState* audioState, unsigned long framesPerBuffer, const  float* input, float* output);
    
    static void playRecording(AudioState* audioState, unsigned long framesPerBuffer, float* output);

    static  int callback(const void* inputStream, void* outputStream,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
        void* userData);


    //returns all the input devices indexes in a set
    static std::unordered_set<PaDeviceIndex> enumerateDevices(StreamType streamType);

    //test connection to ensure a device is valid
    static bool testConnection(PaDeviceIndex i, StreamType streamType);

    //lets user select device or picks default
    static PaDeviceIndex getDevice(StreamType streamType, bool useDefault);

    //set up stream parameters
    static  PaStreamParameters setupStreamParameters(StreamType streamType, bool useDefault);

    //for after stream is done running
    static void cleanupStream(PaStream* stream);
};