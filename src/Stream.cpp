#include "Stream.h"
#include "AudioState.h"
#include "AudioEffects.h"
#include "portaudio.h"
#include <chrono>


void Stream::recordInput(AudioState* audioState, unsigned long framesPerBuffer, const  float* input, float* output) {
        //if recieving input then get the RMS and fill the ring buffer
        if (input) {
            //volume
            float volume = AudioEffects::getRMS(input, framesPerBuffer);
            audioState->effectParams.currentVolume.store(volume, std::memory_order_relaxed);
            //populate the ring buffer
            for (size_t i = 0; i < framesPerBuffer; i++) {
                float x = input[i];
                AudioEffects::applyEffects(x, *audioState);
                //push to recording
                if (audioState->audioMode == AudioMode::Recording) audioState->recordingHistory.push_back(x);
                //play live audio
                if (audioState->audioMode == AudioMode::LivePlayback) output[i] = x;
            }
        }
        else {
            for (unsigned long i = 0; i < framesPerBuffer; i++) {
                output[i] = 0.0f;
            }
        }
    }

void Stream::playRecording(AudioState* audioState, unsigned long framesPerBuffer, float* output) {
        auto audioMode = audioState->audioMode.load(std::memory_order_relaxed);
        //play recording
        if (audioMode == AudioMode::PlayingRecording || audioMode == AudioMode::Loop) {
            auto& recordingHistory = audioState->recordingHistory;
            //send data to output stream
            for (unsigned long i = 0; i < framesPerBuffer; i++) {

                size_t playbackIndex = audioState->playbackIndex.load(std::memory_order_relaxed);

                if (recordingHistory.size() == 0 || (playbackIndex >= recordingHistory.size() && audioMode == AudioMode::PlayingRecording)) {
                    audioState->audioMode.store(AudioMode::Idle, std::memory_order_relaxed);
                    return;
                }

                if (playbackIndex >= recordingHistory.size()) {
                    playbackIndex = 0;
                }


                float x = recordingHistory[playbackIndex++];

                audioState->playbackIndex.store(playbackIndex, std::memory_order_relaxed);


                output[i] = x;
            }
        }
        //neither (just output 0)
        if (audioState->audioMode == AudioMode::Idle || audioState->audioMode == AudioMode::Recording) {
            for (unsigned long i = 0; i < framesPerBuffer; i++) output[i] = 0.0f;
        }


    }

int Stream::callback(
        const void* inputStream, void* outputStream,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
        void* userData)
    {
        auto* audioState = static_cast<AudioState*>(userData);
        const float* input = static_cast<const float*> (inputStream);
        float* output = static_cast<float*> (outputStream);

        auto start = std::chrono::steady_clock::now();

        recordInput(audioState, framesPerBuffer, input, output);
        playRecording(audioState, framesPerBuffer, output);

        auto end = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        audioState->processTime.store(duration.count(), std::memory_order_relaxed);

        return paContinue;
    }


PaDeviceIndex Stream::getDevice(StreamType streamType, bool) {
        return (streamType == INPUT) ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
    }

//set up stream parameters
PaStreamParameters Stream::setupStreamParameters(StreamType streamType, bool useDefault) {
        PaStreamParameters inputParams{};
        inputParams.device = getDevice(streamType, useDefault);
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = (streamType == INPUT) ? Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency : Pa_GetDeviceInfo(inputParams.device)->defaultLowOutputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;

        return inputParams;
    }

//for after stream is done running
void Stream::cleanupStream(PaStream* stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }
