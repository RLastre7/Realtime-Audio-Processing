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

    PaStreamParameters inputParams = Stream::setupStreamParameters(INPUT,true);
    PaStreamParameters outputParams = Stream::setupStreamParameters(OUTPUT,true);

    int sampleRate = 44100;
    int framesPerBuffer = 256;

    PaStream* stream = nullptr;
    AudioState audioState(sampleRate,framesPerBuffer);


    Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, framesPerBuffer, paNoFlag, Stream::callback, &audioState);
    Pa_StartStream(stream);  

    std::thread uiThread(UserInterface::UILoop, std::ref(audioState));

    std::string menu =
        "'t' Toggle Recording | 'p' Play Recording | 'l' Live Playback | 'c' Clear Recording | 'q' Quit\n"
        "Gain: '+ / -' adjust | 'g' toggle ON/OFF\n"
        "Drive: '] / [' adjust | 'd' toggle ON/OFF\n";
        //"Delay: 'e / r' length adjust | 'w / s' wet adjust | 'f' toggle ON/OFF";
    std::cout << menu << std::endl;

    uiThread.join();

    Stream::cleanupStream(stream);


    return 0;
}
