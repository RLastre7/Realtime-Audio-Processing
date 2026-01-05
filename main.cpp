#include <iostream>
#include <vector>
#include "portaudio.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <fstream>


struct RingBuffer {
    std::vector<float> buffer;
    size_t writeIndex = 0;

    RingBuffer() = default;

    RingBuffer(size_t n) : buffer(n) {}

    //handles wrapping when indexing, allows writing
    float& operator[](size_t i) {
        return buffer[i % buffer.size()];
    }
    //handles wrapping for indexing, read only
    const float operator[](size_t i) const {
        return buffer[i % buffer.size()];
    }

    //get a value from an offset relative to the head
    float getRelativeToHead(size_t offset) const {
        return (*this)[offset + writeIndex];
    }
    
    //pushes a value to the next spot in the ring buffer
    void push(float value) {
        (*this)[writeIndex] = value;
        writeIndex =  (writeIndex+1) % buffer.size();
    }
};

struct AudioState {
    RingBuffer ringBuffer;
    size_t windowSize;
    std::atomic<float> currentVolume;
    std::atomic<float> gain = 1.0f;
    std::atomic<float> drive = 1.0f;

    AudioState(const size_t buffSize,const size_t windSize) {
        ringBuffer.buffer.resize(buffSize);
        windowSize = windSize;
    }

    float dotProductOnRingBuffer(size_t indexA, size_t indexB) {
        float sum = 0.0f;
        for (auto i = 0;i < windowSize;i++) {
            sum += (ringBuffer[indexA + i] * ringBuffer[indexB + i]);
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
        int windowStart = 0;

        //go through whole data
        for (auto lagIndex = minLag; lagIndex < maxLag; lagIndex++) {

            int comparisonWindowStart = 0;

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
        return static_cast<float>(ringBuffer.buffer.size()) / lag;
    }

};

struct AudioEffects {

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


static void dumpVectorCSV(const std::vector<float>& v, const std::string& name) {
    std::ofstream file(name, std::ios::trunc);
    for (size_t i = 0; i < v.size(); ++i) {
        file << i << "," << v[i] << "\n";
    }
}



void UILoop(AudioState& audioState) {
    char c = ' ';
    std::string menu;
    std::cout << menu << std::endl;

    while (c != 'q' ) {


        std::cin >> c;

        switch (c) {
            case 'q':
                break;
            case '+':
                audioState.gain.store( audioState.gain.load() + 0.01);
                break;
            case '-':
                audioState.gain.store(audioState.gain.load() - 0.01);
                break;
        }

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
    float* output= static_cast<float*> (outputStream);
    
    //if recieving input then get the RMS and fill the ring buffer
    if (input) {
        //volume
        float volume = getRMS(input, framesPerBuffer);
        audioState->currentVolume.store(volume, std::memory_order_relaxed);
        
        //populate the ring buffer
        for (size_t i = 0;i < framesPerBuffer;i++) {
            audioState->ringBuffer.push(input[i]);
        }

        float g = audioState->gain.load(std::memory_order_relaxed);
        float d = audioState->drive.load(std::memory_order_relaxed);

        //send data to output stream
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            float x = input ? input[i] : 0.0f;
            AudioEffects::gain(x, g);
            AudioEffects::distortion(x, d);
            output[i] = x;
        }
    }

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

    while (true) {

        //display the audio bar
        {
            int width = 50;
            std::string box = "#";
            float v = audioState.currentVolume.load();
            //fill the bar
            int barLength = static_cast<int>(v * width);
            std::string bar(barLength, box[0]);
            std::string emptyBar(width - barLength, ' ');
            std::cout << "\rVolume: " << bar << emptyBar << flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        ////get the pitch
        //{
        //    float frequency = audioState.getFrequency();
        //    cout <<  "\rFrequency:   " << frequency << "         " << flush;
        //}
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }


    cleanupStream(stream);

    uiThread.join();

    return 0;
}
