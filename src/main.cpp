//port audio
#include "portaudio.h"
#ifdef __linux__
#include <alsa/asoundlib.h>
#endif

//io
#include <iostream>

//other includes
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <fstream>


//my helper classes
#include "AudioState.h"
#include "RingBuffer.h"
#include "AudioMode.h"
#include "AudioEffects.h"
#include "UserInterface.h"
#include "Stream.h"



#ifdef __linux__
static void noopAlsaHandler(const char*, int, const char*, int, const char*, ...) {}
#endif

int main() {

#ifdef __linux__
    snd_lib_error_set_handler(noopAlsaHandler);
#endif

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    PaStreamParameters inputParams = Stream::setupStreamParameters(INPUT, true);
    PaStreamParameters outputParams = Stream::setupStreamParameters(OUTPUT, true);

    double sampleRate = 48000;
    int framesPerBuffer = 64;

    PaStream* stream = nullptr;
    AudioState audioState(sampleRate, framesPerBuffer, inputParams.device, outputParams.device);

    err = Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, framesPerBuffer, paNoFlag, Stream::callback, &audioState);
    if (err != paNoError) {
        std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::thread uiThread(UserInterface::UILoop, std::ref(audioState));

    while (audioState.appRunning.load(std::memory_order_relaxed)) {
        if (audioState.deviceChangeRequested.exchange(false, std::memory_order_relaxed)) {
            Pa_StopStream(stream);
            Pa_CloseStream(stream);

            PaDeviceIndex newInput = audioState.targetInputDevice.load(std::memory_order_relaxed);
            PaDeviceIndex newOutput = audioState.targetOutputDevice.load(std::memory_order_relaxed);

            if (newInput >= 0) {
                audioState.inputDevice = newInput;
                inputParams.device = newInput;
                inputParams.suggestedLatency = Pa_GetDeviceInfo(newInput)->defaultLowInputLatency;
            }
            if (newOutput >= 0) {
                audioState.outputDevice = newOutput;
                outputParams.device = newOutput;
                outputParams.suggestedLatency = Pa_GetDeviceInfo(newOutput)->defaultLowOutputLatency;
            }

            err = Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, framesPerBuffer, paNoFlag, Stream::callback, &audioState);
            if (err == paNoError) {
                Pa_StartStream(stream);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    uiThread.join();

    Stream::cleanupStream(stream);


    return 0;
}
