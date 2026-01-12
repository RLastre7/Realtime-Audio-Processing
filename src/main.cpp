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
#include "AudioMode.h"
#include "AudioEffects.h"
#include "UserInterface.h"
#include "Stream.h"



int main() {


    Pa_Initialize();

    PaStreamParameters inputParams = Stream::setupStreamParameters(INPUT,false);
    PaStreamParameters outputParams = Stream::setupStreamParameters(OUTPUT,true);

    int sampleRate = Pa_GetDeviceInfo(inputParams.device)->defaultSampleRate;
    int framesPerBuffer = 64;

    PaStream* stream = nullptr;
    AudioState audioState(sampleRate,framesPerBuffer,inputParams.device,outputParams.device);


    Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, framesPerBuffer, paNoFlag, Stream::callback, &audioState);
    Pa_StartStream(stream);  

    std::thread uiThread(UserInterface::UILoop, std::ref(audioState));

    uiThread.join();

    Stream::cleanupStream(stream);


    return 0;
}
