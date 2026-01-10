#include "UserInterface.h"
#include "AudioEffects.h"
#include "AudioEffects.h"
#include <conio.h>
#include <chrono>
#include <thread>
#include <cmath>

static void displayVolumeBar(AudioState& audioState) {
    //display the audio bar
    int width = 50;
    std::string box = "#";
    float v = audioState.effectParams.currentVolume.load(std::memory_order_relaxed);
    //fill the bar
    int barLength = static_cast<int>(v * width);
    std::string bar(barLength, box[0]);
    std::string emptyBar(width - barLength, ' ');
    std::cout << "Volume: " << bar << emptyBar << std::endl;
}

static void handleInput(char c, AudioState& audioState) {
    //rate of change for each effect variable
    float sliderChange = 0.1f;
    float largeSliderChange = 1.f;
    float delayChange = 50.f;
    float wetChange = 0.05f;

    //audio state variables
    float g = audioState.effectParams.gain.load(std::memory_order_relaxed);
    float d = audioState.effectParams.drive.load(std::memory_order_relaxed);
    size_t delay = audioState.effectParams.delaySamples.load(std::memory_order_relaxed);
    float wet = audioState.effectParams.wet.load(std::memory_order_relaxed);


    switch (c) {

    case 'q': //quit
        audioState.appRunning.store(false, std::memory_order_relaxed);
        break;

        // Gain
    case '+': case '=':
        audioState.effectParams.gain.store(g + sliderChange, std::memory_order_relaxed);
        break;
    case '-': case '_':
        audioState.effectParams.gain.store(std::max(0.00f, g - sliderChange), std::memory_order_relaxed);
        break;
        // toggle gain effect
    case 'g':
        audioState.effectParams.gain_flag = !audioState.effectParams.gain_flag;
        break;

        // Drive / overdrive
    case ']': case '}':
        audioState.effectParams.drive.store(d + largeSliderChange, std::memory_order_relaxed);
        break;
    case '[': case '{':
        audioState.effectParams.drive.store(std::max(0.00f, d - largeSliderChange), std::memory_order_relaxed);
        break;
    case 'd': // toggle overdrive effect
        audioState.effectParams.overdrive_flag = !audioState.effectParams.overdrive_flag;
        break;
        // Delay
    case 'w': // increase wet
        audioState.effectParams.wet.store(std::min(1.00f, wet + wetChange), std::memory_order_relaxed);
        break;
    case 's': // decrease wet
        audioState.effectParams.wet.store(std::max(0.00f, wet - wetChange), std::memory_order_relaxed);
        break;
    case 'e': // increase delay
        audioState.effectParams.delaySamples.store(delay + static_cast<size_t>(delayChange), std::memory_order_relaxed);
        break;
    case 'r': // decrease delay
        audioState.effectParams.delaySamples.store((delay > delayChange) ? delay - static_cast<size_t>(delayChange) : 0, std::memory_order_relaxed);
        break;
    case 'f': // toggle delay effect
        audioState.effectParams.delay_flag = !audioState.effectParams.delay_flag;
        break;

        // Recording / playback
    case 't': //recording
        toggleMode(audioState.audioMode, AudioMode::Recording);
        break;
    case 'p': //play recording
        audioState.playbackIndex.store(0, std::memory_order_relaxed);
        toggleMode(audioState.audioMode, AudioMode::PlayingRecording);
        break;
    case 'l': //loop recording
        toggleMode(audioState.audioMode, AudioMode::Loop);
        break;
    case 'v': //live playback
        toggleMode(audioState.audioMode, AudioMode::LivePlayback);
        break;
    case 'c': //clear recording
        audioState.recordingHistory.clear();
        break;
    }


}

static void displayAudioMode(AudioState& audioState) {
    // Display audio mode
    std::string status;
    switch (audioState.audioMode.load(std::memory_order_relaxed)) {
    case AudioMode::Recording: status =        "Recording        "; break;
    case AudioMode::PlayingRecording: status = "Playing Recording"; break;
    case AudioMode::LivePlayback: status =     "Live Playback    "; break;
    case AudioMode::Loop: status =             "Loop             "; break;
    case AudioMode::Idle: status =             "Idle             "; break;
    }
    std::cout << "Status: " << status << std::endl;
}

static void displayEffectParameters(AudioState& audioState) {
    std::cout << audioState.effectParams.getParams() << std::endl;
}

static void displayEffectFlags(AudioState& audioState) {
    std::cout << audioState.effectParams.getEffectFlags() << std::endl;
}

void UserInterface::printData(AudioState& audioState) {
    // Move cursor to line below menu
    std::cout << "\033[6;0H";


    displayVolumeBar(audioState);

    displayAudioMode(audioState);

    displayEffectParameters(audioState);

    displayEffectFlags(audioState);

    //// Display effect parameters
    //std::cout << "Gain: " << audioState.effectParams.gain.load(std::memory_order_relaxed) << "           " << (audioState.activatedEffects[Effects::GAIN] ? " [ON]" : " [OFF]") << "           " << std::endl;

    //std::cout << "Drive:" << audioState.effectParams.drive.load(std::memory_order_relaxed) << "          " << (audioState.activatedEffects[Effects::DISTORTION] ? " [ON]" : " [OFF]") << "           " << std::endl;

    //std::cout << "Delay:" << audioState.delaySamples.load(std::memory_order_relaxed) << " samples" << "         " << (audioState.activatedEffects[Effects::DELAY] ? " [ON]" : " [OFF]") << "           " << std::endl;

    //std::cout << "Wet:" << audioState.wet.load(std::memory_order_relaxed) << std::endl;

    //blank line
    std::cout << std::endl;
}

void UserInterface::UILoop(AudioState& audioState) {
    char c = ' ';

    std::string menu =
        "'t' Toggle Recording | 'p' Play Recording | 'v' Live Playback | 'l' loop recording |'c' Clear Recording | 'q' Quit\n"
        "Gain: '+ / -' adjust | 'g' toggle ON/OFF\n"
        "Drive: '] / [' adjust | 'd' toggle ON/OFF\n";
    //"Delay: 'e / r' length adjust | 'w / s' wet adjust | 'f' toggle ON/OFF";
    std::cout << menu << std::endl;

    while (audioState.appRunning.load(std::memory_order_relaxed)) {

        printData(audioState);


        //read user input
        if (_kbhit()) {
            c = _getch();

            handleInput(c, audioState);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

