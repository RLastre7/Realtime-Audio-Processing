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

    AudioState(const size_t buffSize,const size_t windSize) {
        bufferSize = buffSize;
        windowSize = windSize;
        ringBuffer.resize(buffSize);
    }

    float dotProductOnRingBuffer(size_t indexA, size_t indexB) {
        float sum = 0.0f;
        for (auto i = 0;i < windowSize;i++) {
            sum += (ringBuffer[(indexA + i) % bufferSize] * ringBuffer[(indexB + i) % bufferSize]);
        }
        return sum / windowSize;
    }

    int findSimilarityLag() {
        float maxSimilarity = 0.0f;
        int maxLagIndex = -1;


        //temp variable remove later
        int minLag = 70;
        int maxLag = 400;

        //most recent data window
        int windowStart = (writeIndex - windowSize + bufferSize) % bufferSize;

        //go through whole data
        for (auto lagIndex = minLag; lagIndex < maxLag; lagIndex++) {

            int comparisonWindowStart = (windowStart - lagIndex + bufferSize) % bufferSize;

            float similarity = dotProductOnRingBuffer(windowStart, comparisonWindowStart);

            //compare with current max and assign new max
            if (similarity > maxSimilarity) {
                maxLagIndex = lagIndex;
                maxSimilarity = similarity;
            }
        }
        return maxLagIndex;
    }

    float getFrequency() {
        int lag = findSimilarityLag();
        return static_cast<float>(bufferSize) / lag;
    }

};


static void dumpVectorCSV(const std::vector<float>& v, const std::string& name) {
    std::ofstream file(name, std::ios::trunc);
    for (size_t i = 0; i < v.size(); ++i) {
        file << i << "," << v[i] << "\n";
    }
}



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
    auto* audioState = static_cast<AudioState*>(userData);
    const float* input = static_cast<const float*> (inputStream);
    
    //if recieving input then get the RMS and fill the ring buffer
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


static PaStreamParameters setupStreamParameters() {
    PaStreamParameters inputParams{};
    inputParams.device = Pa_GetDefaultInputDevice();
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
}


using std::cout;
using std::endl;
using std::flush;
int main() {
    
    Pa_Initialize();


    PaStreamParameters inputParams = setupStreamParameters();

    int sampleRate = 44100;
    int framesPerBuffer = 2048;

    PaStream* stream = nullptr;
    AudioState audioState(sampleRate,framesPerBuffer);


    Pa_OpenStream(&stream, &inputParams, nullptr, sampleRate, framesPerBuffer, paNoFlag, callback, &audioState);
    Pa_StartStream(stream);  
    while (true) {

        ////display the audio bar
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
            float frequency = audioState.getFrequency();
            cout <<  "\rFrequency:   " << frequency << "         " << flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }


    cleanupStream(stream);

    return 0;
}
