#include <iostream>
#include <vector>
#include "portaudio.h"
#include <chrono>
#include <thread>
#include <atomic>

//get volume
static float getRMS(const float* data,unsigned long size) {
    float sum = 0.0f;
    for (auto i=0; i < size; i++) {
        sum += data[i] * data[i];
    }
    return sqrt(sum / size);
}


static int callback(
    const void* inputStream, void* outputStream,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
    void* userData) 
{
    auto* currentVolume = static_cast<std::atomic<float>*>(userData);
    const float* input = static_cast<const float*> (inputStream);
    
    //if recieving input then update the user data buffer with the volume
    if (input) {
        float volume = getRMS(input, framesPerBuffer);
        currentVolume->store(volume, std::memory_order_relaxed);
    }

    return paContinue;
}



using std::cout;
using std::endl;
using std::flush;
int main() {
    PaError err = Pa_Initialize();

    PaStreamParameters inputParams{};
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* stream = nullptr;
    std::atomic<float> currentVolume{ 0.0f };
    
    int sampleRate = 44100;
    int framesPerBuffer = 256;

    std::string box = "#";
    int width = 50;


    Pa_OpenStream(&stream, &inputParams, nullptr, sampleRate, framesPerBuffer, paNoFlag, callback, &currentVolume);
    Pa_StartStream(stream);    
    while (true) {
        float v = currentVolume.load();
        //fill the bar
        int barLength = static_cast<int>(currentVolume * width);
        std::string bar(barLength, box[0]);
        std::string emptyBar(width - barLength, ' ');
        std::cout << "\rVolume: " << bar  << emptyBar << flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);



    Pa_Terminate();
    return 0;
}
