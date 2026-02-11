#include "Stream.h"
#include "AudioState.h"
#include "AudioEffects.h"
#include "portaudio.h"
#include <unordered_set>
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

        float g = audioState->effectParams.gain.load(std::memory_order_relaxed);
        float d = audioState->effectParams.drive.load(std::memory_order_relaxed);
        float x;
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


//returns all the input devices indexes in a set
std::unordered_set<PaDeviceIndex> Stream::enumerateDevices(StreamType streamType) {
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
bool Stream::testConnection(PaDeviceIndex i, StreamType streamType) {
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
PaDeviceIndex Stream::getDevice(StreamType streamType, bool useDefault) {
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
PaStreamParameters Stream::setupStreamParameters(StreamType streamType, bool useDefault) {
        PaStreamParameters inputParams{};
        inputParams.device = getDevice(streamType, useDefault);
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;

        return inputParams;
    }

//for after stream is done running
void Stream::cleanupStream(PaStream* stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }
