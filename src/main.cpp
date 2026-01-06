//port audio
#include "portaudio.h"

//io
#include <iostream>
#include <conio.h>

//other includes
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <fstream>


//my helper classes
#include "AudioState.h"
#include "RingBuffer.h"




struct AudioEffects {
    
    //get volume
    static float getRMS(const float* data, unsigned long size) {
        float sum = 0.0f;
        for (auto i = 0; i < size; i++) {
            sum += data[i] * data[i];
        }
        return sqrt(sum / size);
    }

    //set volume
    static void gain(float& data,const float gain) {
        data *= gain;
    }

    static void distortion(float& data, const float drive) {
        data *= drive;
        data = std::tanh(data);
    }


     
};

enum StreamType {
    INPUT,
    OUTPUT,
};


static void displayVolumeBar(AudioState& audioState) {
    //display the audio bar
    {
        int width = 50;
        std::string box = "#";
        float v = audioState.currentVolume.load(std::memory_order_relaxed);
        //fill the bar
        int barLength = static_cast<int>(v * width);
        std::string bar(barLength, box[0]);
        std::string emptyBar(width - barLength, ' ');
        std::cout << "Volume: " << bar << emptyBar << std::endl;
    }
}

static void printData(AudioState& audioState) {

    //move cursor to line below menu
    std::cout << "\033[6;0H";

    // Clear current line
    std::cout << "\033[K";

    //volume bar
    displayVolumeBar(audioState);

    //set the recroding/playback status
    std::string status;
    if (audioState.recording.load(std::memory_order_relaxed)) status = "recording";
    else if (audioState.playing.load(std::memory_order_relaxed)) status = "playing     ";
    else status = "idle     ";

    //print information 
    std::cout << status << std::endl;
    std::cout << "Gain: " << audioState.gain.load(std::memory_order_relaxed) << std::endl;

}

void UILoop(AudioState& audioState) {
    char c = ' ';
    float gainChange = 0.1f;
    while (audioState.appRunning.load(std::memory_order_relaxed)) {
        
        printData(audioState);
        
        float g = audioState.gain.load(std::memory_order_relaxed);


        //read user input
        if (_kbhit()) {
            c = _getch();

            switch (c) {
                //quit
            case 'q':
                audioState.appRunning.store(false, std::memory_order_relaxed);
                break;
                //increase gain
            case '+':
            case '=':
                audioState.gain.store(g + gainChange, std::memory_order_relaxed);
                break;
                //decrease gain
            case '-':
            case '_':
                audioState.gain.store(std::max(0.0f, g - gainChange), std::memory_order_relaxed);
                break;
                //toggle recording
            case 'r':
                if (!audioState.recording) audioState.recordingHistory.clear();
                audioState.playing = false;
                audioState.recording = !audioState.recording;
                break;
                //play recording
            case 'p':
                audioState.recording = false;
                audioState.playing = true;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}




static void recordInput(AudioState* audioState, unsigned long framesPerBuffer, const  float* input) {
    //if recieving input then get the RMS and fill the ring buffer
    if (input) {
        //volume
        float volume = AudioEffects::getRMS(input, framesPerBuffer);
        audioState->currentVolume.store(volume, std::memory_order_relaxed);
        //populate the ring buffer
        for (size_t i = 0; i < framesPerBuffer; i++) {
            audioState->ringBuffer.push(input[i]);
            if (audioState->recording) audioState->recordingHistory.push_back(input[i]);
        }
    }
}

static void playRecording(AudioState* audioState, unsigned long framesPerBuffer, float* output) {
    if (audioState->playing) {

        float g = audioState->gain.load(std::memory_order_relaxed);
        float d = audioState->drive.load(std::memory_order_relaxed);
        auto recordingHistory = audioState->recordingHistory;
        //send data to output stream
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            if (!recordingHistory.size()) {
                audioState->playing.store(false, std::memory_order_relaxed);
                break;
            }
            size_t playbackIndex = audioState->playbackIndex.load(std::memory_order_relaxed);

            float x = recordingHistory[playbackIndex];

            playbackIndex = (playbackIndex + 1) % recordingHistory.size();
            audioState->playbackIndex.store(playbackIndex, std::memory_order_relaxed);
            if (playbackIndex >= recordingHistory.size() - 1) {
                audioState->playing.store(false, std::memory_order_relaxed);
            }

            AudioEffects::gain(x, g);
            AudioEffects::distortion(x, d);

            output[i] = x;
        }
    }
    else {
        for (unsigned long i = 0; i < framesPerBuffer; i++) output[i] = 0.0f;
    }
}




static int callback(
    const void* inputStream, void* outputStream,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
    void* userData) 
{
    auto* audioState = static_cast<AudioState*>(userData);
    const float* input = static_cast<const float*> (inputStream);
    float* output= static_cast<float*> (outputStream);

    
    recordInput(audioState, framesPerBuffer, input);

    playRecording(audioState, framesPerBuffer, output);

    

    return paContinue;
}



//returns all the input devices indexes in a set
std::unordered_set<PaDeviceIndex> enumerateDevices(StreamType streamType) {
    std::unordered_set<PaDeviceIndex> set;

    std::string streamTypeString = (streamType == INPUT) ? "\n\nInput " : "\n\nOuput ";
    std::cout << streamTypeString << "Devices:" << std::endl;

    for (auto i = 0; i < Pa_GetDeviceCount(); i++) {
        const PaDeviceInfo* device = Pa_GetDeviceInfo(i);
        if (streamType == INPUT && device->maxInputChannels > 0) {
            set.insert(i);
            std::cout << i << ":" << device->name  << " -> " << Pa_GetHostApiInfo(device->hostApi)->name << std::endl;
        }
        if (streamType == OUTPUT && device->maxOutputChannels > 0) {
            set.insert(i);
            std::cout << i << ":" << device->name << " -> " << Pa_GetHostApiInfo(device->hostApi)->name << std::endl;
        }

    }
    return set;
}

//test connection to ensure a device is valid
bool testConnection(PaDeviceIndex i,StreamType streamType) {
    PaStreamParameters params{};
    params.device = i;
    params.channelCount = 1;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowInputLatency;

    PaStream* testStream = nullptr;
    PaError err;
    if (streamType == INPUT) {
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
    std::cout << "Connection unsuccesful" << std::endl;
    return false;
}

PaDeviceIndex getDevice(StreamType streamType,bool useDefault) {
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
        isValidDevice = testConnection(i,streamType);
    }
    return i;
}

static PaStreamParameters setupStreamParameters(StreamType streamType,bool useDefault) {
    PaStreamParameters inputParams{};
    inputParams.device = getDevice(streamType,useDefault);
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    return inputParams;
}

static void cleanupStream(PaStream* stream) {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    std::cout << "Steam Cleanedup" << std::endl;
}


using std::cout;
using std::endl;
using std::flush;
int main() {


    Pa_Initialize();

    PaStreamParameters inputParams = setupStreamParameters(INPUT,true);
    PaStreamParameters outputParams = setupStreamParameters(OUTPUT,true);

    int sampleRate = 44100;
    int framesPerBuffer = 2048;

    PaStream* stream = nullptr;
    AudioState audioState(sampleRate,framesPerBuffer);


    Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, framesPerBuffer, paNoFlag, callback, &audioState);
    Pa_StartStream(stream);  

    std::thread uiThread(UILoop, std::ref(audioState));

    std::string menu = "'r' to toggle recording\n'p' to playback recording\n'+/=' or '-/_' to control the volume\n'q' to quit";
    std::cout << menu << std::endl;

    uiThread.join();

    cleanupStream(stream);


    return 0;
}
