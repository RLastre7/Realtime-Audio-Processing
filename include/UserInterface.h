#pragma once
#include "AudioState.h"

struct UserInterface {

    inline static void displayVolumeBar(AudioState& audioState) {
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

    inline static void printData(AudioState& audioState) {
        // Move cursor to line below menu
        std::cout << "\033[6;0H";

        // Clear lines (3 lines for volume, status, effects)
        for (int i = 0; i < 8; ++i) std::cout << "\033[K" << std::endl;

        displayVolumeBar(audioState);

        // Display audio mode
        std::string status;
        switch (audioState.audioMode.load(std::memory_order_relaxed)) {
        case AudioMode::Recording: status = "Recording        "; break;
        case AudioMode::PlayingRecording: status = "Playing Recording"; break;
        case AudioMode::LivePlayback: status = "Live Playback    "; break;
        case AudioMode::Idle: status = "Idle             "; break;
        }
        std::cout << "Status: " << status << std::endl;

        // Display effect parameters
        std::cout << "Gain: " << audioState.gain.load(std::memory_order_relaxed) << "           " << (audioState.activatedEffects[Effects::GAIN] ? " [ON]" : " [OFF]") << "           " << std::endl;

        std::cout << "Drive:" << audioState.drive.load(std::memory_order_relaxed) << "          " << (audioState.activatedEffects[Effects::DISTORTION] ? " [ON]" : " [OFF]") << "           " << std::endl;

        //std::cout << "Delay:" << audioState.delaySamples.load(std::memory_order_relaxed) << " samples" << "         " << (audioState.activatedEffects[Effects::DELAY] ? " [ON]" : " [OFF]") << "           " << std::endl;

        //std::cout << "Wet:" << audioState.wet.load(std::memory_order_relaxed) << std::endl;

        //blank line
        std::cout << std::endl;
    }

    inline static void UILoop(AudioState& audioState) {
        char c = ' ';
        float sliderChange = 0.1f;
        float largeSliderChange = 1.f;
        float delayChange = 50.f; // samples
        float wetChange = 0.05f;

        while (audioState.appRunning.load(std::memory_order_relaxed)) {

            printData(audioState);

            float g = audioState.gain.load(std::memory_order_relaxed);
            float d = audioState.drive.load(std::memory_order_relaxed);
            //size_t delay = audioState.delaySamples.load(std::memory_order_relaxed);
            //float wet = audioState.wet.load(std::memory_order_relaxed);

            //read user input
            if (_kbhit()) {
                c = _getch();

                switch (c) {

                case 'q': //quit
                    audioState.appRunning.store(false, std::memory_order_relaxed);
                    break;

                    // Gain
                case '+': case '=':
                    audioState.gain.store(g + sliderChange, std::memory_order_relaxed);
                    break;
                case '-': case '_':
                    audioState.gain.store(std::max(0.0f, g - sliderChange), std::memory_order_relaxed);
                    break;
                case 'g': // toggle gain effect
                    audioState.activatedEffects[Effects::GAIN] =
                        !audioState.activatedEffects[Effects::GAIN];
                    break;

                    // Drive / Distortion
                case ']': case '}':
                    audioState.drive.store(d + largeSliderChange, std::memory_order_relaxed);
                    break;
                case '[': case '{':
                    audioState.drive.store(std::max(0.0f, d - largeSliderChange), std::memory_order_relaxed);
                    break;
                case 'd': // toggle distortion effect
                    audioState.activatedEffects[Effects::DISTORTION] =
                        !audioState.activatedEffects[Effects::DISTORTION];
                    break;

                    //TODO FIX DELAY
                //    // Delay
                //case 'w': // increase wet
                //    audioState.wet.store(std::min(1.0f, wet + wetChange), std::memory_order_relaxed);
                //    break;
                //case 's': // decrease wet
                //    audioState.wet.store(std::max(0.0f, wet - wetChange), std::memory_order_relaxed);
                //    break;
                //case 'e': // increase delay
                //    audioState.delaySamples.store(delay + static_cast<size_t>(delayChange), std::memory_order_relaxed);
                //    break;
                //case 'r': // decrease delay
                //    audioState.delaySamples.store((delay > delayChange) ? delay - static_cast<size_t>(delayChange) : 0, std::memory_order_relaxed);
                //    break;
                //case 'f': // toggle delay effect
                //    audioState.activatedEffects[Effects::DELAY] =
                //        !audioState.activatedEffects[Effects::DELAY];
                //    break;

                    // Recording / playback
                case 't': // toggle recording
                    toggleMode(audioState.audioMode, AudioMode::Recording);
                    break;
                case 'p':
                    audioState.playbackIndex.store(0, std::memory_order_relaxed);
                    toggleMode(audioState.audioMode, AudioMode::PlayingRecording);
                    break;
                case 'l':
                    toggleMode(audioState.audioMode, AudioMode::LivePlayback);
                    break;
                case 'c':
                    audioState.recordingHistory.clear();
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

};