#pragma once
#include "AudioState.h"
#include "portaudio.h"
#include <unordered_set>

enum StreamType {
    INPUT,
    OUTPUT,
};

struct Stream {
    inline static void recordInput(AudioState* audioState, unsigned long framesPerBuffer, const  float* input) {
        //if recieving input then get the RMS and fill the ring buffer
        if (input) {
            //volume
            float volume = AudioEffects::getRMS(input, framesPerBuffer);
            audioState->currentVolume.store(volume, std::memory_order_relaxed);
            //populate the ring buffer
            for (size_t i = 0; i < framesPerBuffer; i++) {
                audioState->ringBuffer.push(input[i]);
                if (audioState->audioMode == AudioMode::Recording) audioState->recordingHistory.push_back(input[i]);
            }
        }
    }

    inline static void playRecording(AudioState* audioState, unsigned long framesPerBuffer, float* output, const float* input) {

        float g = audioState->gain.load(std::memory_order_relaxed);
        float d = audioState->drive.load(std::memory_order_relaxed);
        float x;
        auto audioMode = audioState->audioMode.load(std::memory_order_relaxed);
        //play recording
        if (audioMode == AudioMode::PlayingRecording) {
            auto& recordingHistory = audioState->recordingHistory;
            //send data to output stream
            for (unsigned long i = 0; i < framesPerBuffer; i++) {

                size_t playbackIndex = audioState->playbackIndex.load(std::memory_order_relaxed);

                if (recordingHistory.size() == 0 || playbackIndex >= recordingHistory.size()) {
                    audioState->audioMode.store(AudioMode::Idle, std::memory_order_relaxed);
                    audioState->playbackIndex = 0;
                    return;
                }


                float x = recordingHistory[playbackIndex++];

                audioState->playbackIndex.store(playbackIndex, std::memory_order_relaxed);

                AudioEffects::applyEffects(x, *audioState);

                output[i] = x;
            }
            return;
        }
        //play live input feed
        if (audioMode == AudioMode::LivePlayback) {
            for (unsigned long i = 0; i < framesPerBuffer; ++i) {
                x = input[i];
                AudioEffects::applyEffects(x, *audioState);
                output[i] = x;
            }
            return;
        }
        //neither (just output 0)
        for (unsigned long i = 0; i < framesPerBuffer; i++) output[i] = 0.0f;



    }

    inline static  int callback(
        const void* inputStream, void* outputStream,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
        void* userData)
    {
        auto* audioState = static_cast<AudioState*>(userData);
        const float* input = static_cast<const float*> (inputStream);
        float* output = static_cast<float*> (outputStream);



        recordInput(audioState, framesPerBuffer, input);
        playRecording(audioState, framesPerBuffer, output, input);



        return paContinue;
    }



    //returns all the input devices indexes in a set
    inline static std::unordered_set<PaDeviceIndex> enumerateDevices(StreamType streamType) {
        std::unordered_set<PaDeviceIndex> set;

        std::string streamTypeString = (streamType == INPUT) ? "\n\nInput " : "\n\nOuput ";
        std::cout << streamTypeString << "Devices:" << std::endl;

        for (auto i = 0; i < Pa_GetDeviceCount(); i++) {
            const PaDeviceInfo* device = Pa_GetDeviceInfo(i);
            if (streamType == INPUT && device->maxInputChannels > 0) {
                set.insert(i);
                std::cout << i << ":" << device->name << " -> " << Pa_GetHostApiInfo(device->hostApi)->name << std::endl;
            }
            if (streamType == OUTPUT && device->maxOutputChannels > 0) {
                set.insert(i);
                std::cout << i << ":" << device->name << " -> " << Pa_GetHostApiInfo(device->hostApi)->name << std::endl;
            }

        }
        return set;
    }

    //test connection to ensure a device is valid
    inline static  bool testConnection(PaDeviceIndex i, StreamType streamType) {
        PaStreamParameters params{};
        params.device = i;
        params.channelCount = 1;
        params.sampleFormat = paFloat32;

        PaStream* testStream = nullptr;
        PaError err;
        if (streamType == INPUT) {
            params.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowInputLatency;
            err = Pa_OpenStream(
                &testStream,
                &params,
                nullptr,
                Pa_GetDeviceInfo(i)->defaultSampleRate,
                256,
                paNoFlag,
                nullptr,
                nullptr
            );
        }
        if (streamType == OUTPUT) {
            params.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowOutputLatency;
            err = Pa_OpenStream(
                &testStream,
                nullptr,
                &params,
                Pa_GetDeviceInfo(i)->defaultSampleRate,
                256,
                paNoFlag,
                nullptr,
                nullptr
            );
        }

        //test device connected
        if (err == paNoError) {
            Pa_CloseStream(testStream);
            std::cout << "Connection Succesful" << std::endl;
            return true;
        }
        std::cout << "Connection unsuccesful:" << err << std::endl;

        return false;
    }

    //lets user select device or picks default
    inline static PaDeviceIndex getDevice(StreamType streamType, bool useDefault) {
        int i = -1;
        bool isValidDevice = false;

        if (useDefault && streamType == INPUT) return  Pa_GetDefaultInputDevice();
        if (useDefault && streamType == OUTPUT) return Pa_GetDefaultOutputDevice();

        std::unordered_set<int> inputDevices = enumerateDevices(streamType);
        while (inputDevices.find(i) == inputDevices.end() || !isValidDevice) {
            std::cout << "\nSelect a device:";
            std::cin >> i;
            if (inputDevices.find(i) == inputDevices.end()) {
                std::cout << "invalid device" << std::endl;
                continue;
            }
            //if picked a device then try to open the stream
            isValidDevice = testConnection(i, streamType);
        }
        system("cls");
        return i;
    }

    //set up stream parameters
    inline static  PaStreamParameters setupStreamParameters(StreamType streamType, bool useDefault) {
        PaStreamParameters inputParams{};
        inputParams.device = getDevice(streamType, useDefault);
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;

        return inputParams;
    }

    //for after stream is done running
    inline static void cleanupStream(PaStream* stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }
};