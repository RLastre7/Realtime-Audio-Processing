#include <iostream>
#include <vector>
#include "portaudio.h"
#include <chrono>
#include <thread>
#include <atomic>

#include <fstream>


struct AudioState {
    std::atomic<float> currentVolume;
    std::vector<float> ringBuffer;
    size_t writeIndex = 0;
    size_t bufferSize;
    size_t windowSize;
};


static void dumpVectorCSV(const std::vector<float>& v, const std::string& name) {
    std::ofstream file(name, std::ios::trunc);
    for (size_t i = 0; i < v.size(); ++i) {
        file << i << "," << v[i] << "\n";
    }
}

using std::vector;
static float dotProductOnRingBuffer(const vector<float>& buffer,size_t windowSize ,size_t indexA, size_t indexB) {
    float sum = 0.0f;
    for (auto i = 0;i < windowSize;i++) {
        sum += (buffer[(indexA + i) % buffer.size()] * buffer[(indexB + i) % buffer.size()]);
    }
    return sum / windowSize;
}

//get volume
static float getRMS(const float* data,unsigned long size) {
    float sum = 0.0f;
    for (auto i=0; i < size; i++) {
        sum += data[i] * data[i];
    }
    return sqrt(sum / size);
}

using std::vector;
static int findSimilarityLag(const AudioState& audioState) {
    float maxSimilarity = 0.0f;
    int maxLagIndex = -1;
    
    const vector<float>& buffer = audioState.ringBuffer;
    size_t bufferSize = buffer.size();
    size_t windowSize = audioState.windowSize;

    //temp variable remove later
    int minLag = 70;
    int maxLag = 400;

    //most recent data window
    int windowStart = (audioState.writeIndex - audioState.windowSize + bufferSize) % bufferSize;

    //go through whole data
    for (auto lagIndex = minLag; lagIndex < maxLag; lagIndex++) {
       
        int comparisonWindowStart = (windowStart - lagIndex + bufferSize) % bufferSize;
      
        float similarity = dotProductOnRingBuffer(buffer, windowSize, windowStart, comparisonWindowStart);

        //compare with current max and assign new max
        if (similarity > maxSimilarity) {
            maxLagIndex = lagIndex;
            maxSimilarity = similarity;
        }
    }
    return maxLagIndex;
}

static float getFrequency(const AudioState& audioState, float sampleRate) {
    int lag = findSimilarityLag(audioState);
    return static_cast<float>(sampleRate) / lag;
}

static int callback(
    const void* inputStream, void* outputStream,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags,
    void* userData) 
{
    auto* audioState = static_cast<AudioState*>(userData);
    const float* input = static_cast<const float*> (inputStream);
    
    //if recieving input then update the user data buffer with the volume
    if (input) {
        //volume
        float volume = getRMS(input, framesPerBuffer);
        audioState->currentVolume.store(volume, std::memory_order_relaxed);
        
        //populate the ring buffer
        for (auto i = 0;i < framesPerBuffer;i++) {
            audioState->ringBuffer[audioState->writeIndex] = input[i];
            audioState->writeIndex = (audioState->writeIndex+1) % audioState->ringBuffer.size();
            
        }
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

    
    int sampleRate = 44100;
    int framesPerBuffer= 2048;
    int windowSize = framesPerBuffer;

    std::string box = "#";
    int width = 50;

    PaStream* stream = nullptr;
    AudioState audioState;
    audioState.bufferSize = sampleRate;
    audioState.ringBuffer.resize(audioState.bufferSize);
    audioState.windowSize = framesPerBuffer;
    std::vector<float> window(windowSize);

    Pa_OpenStream(&stream, &inputParams, nullptr, sampleRate, framesPerBuffer, paNoFlag, callback, &audioState);
    Pa_StartStream(stream);  
    int count = 0;
    while (true) {

        //display the audio bar
        //{
        //    float v = audioState.currentVolume.load();
        //    //fill the bar
        //    int barLength = static_cast<int>(v * width);
        //    std::string bar(barLength, box[0]);
        //    std::string emptyBar(width - barLength, ' ');
        //    std::cout << "\rVolume: " << bar << emptyBar << flush;
        //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //}

        //get the pitch
        {
            float frequency = getFrequency(audioState, sampleRate);
            cout <<  "\rFrequency:   " << frequency << "         " << flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (count % 10 == 0) {
            cout << "Writing to csv" << endl;
            dumpVectorCSV(audioState.ringBuffer, "data.csv");
        }
        count++;
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);



    Pa_Terminate();
    return 0;
}
