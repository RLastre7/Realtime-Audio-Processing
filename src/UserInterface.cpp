#include "UserInterface.h"
#include "AudioEffects.h"
#include <chrono>
#include <thread>
#include <cmath>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

static void populateDeviceLists(AudioState& audioState) {
    audioState.inputDeviceList.clear();
    audioState.inputDeviceNameList.clear();
    audioState.outputDeviceList.clear();
    audioState.outputDeviceNameList.clear();

    for (PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); i++) {
        const PaDeviceInfo* device = Pa_GetDeviceInfo(i);
        if (device->maxInputChannels > 0) {
            audioState.inputDeviceList.push_back(i);
            audioState.inputDeviceNameList.push_back(
                std::string(device->name) + " -> " + Pa_GetHostApiInfo(device->hostApi)->name);
        }
        if (device->maxOutputChannels > 0) {
            audioState.outputDeviceList.push_back(i);
            audioState.outputDeviceNameList.push_back(
                std::string(device->name) + " -> " + Pa_GetHostApiInfo(device->hostApi)->name);
        }
    }
}

static void ImGuiComponents(AudioState& audioState){
    ImVec2 work = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowSize(ImVec2(work.x - 15, work.y * 0.62f), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    ImGui::Begin("Effects");

    ImGui::SeparatorText("Gain");
    bool gainOn = audioState.effectParams.gain_flag.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enable##gain", &gainOn)) {
        audioState.effectParams.gain_flag.store(gainOn, std::memory_order_relaxed);
    }
    float gain = audioState.effectParams.gain.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Gain##gain", &gain, 0.0f, 5.0f, "%.2f")) {
        audioState.effectParams.gain.store(std::max(0.0f, gain), std::memory_order_relaxed);
    }

    ImGui::SeparatorText("Distortion");
    bool overdriveOn = audioState.effectParams.overdrive_flag.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Overdrive", &overdriveOn)) {
        audioState.effectParams.overdrive_flag.store(overdriveOn, std::memory_order_relaxed);
    }
    ImGui::SameLine();
    bool fuzzOn = audioState.effectParams.fuzz_flag.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Fuzz", &fuzzOn)) {
        audioState.effectParams.fuzz_flag.store(fuzzOn, std::memory_order_relaxed);
    }
    float drive = audioState.effectParams.drive.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Drive##drive", &drive, 0.0f, 20.0f, "%.1f")) {
        audioState.effectParams.drive.store(std::max(0.0f, drive), std::memory_order_relaxed);
    }

    ImGui::SeparatorText("Delay");
    bool delayOn = audioState.effectParams.delay_flag.load(std::memory_order_relaxed);
    if (ImGui::Checkbox("Enable##delay", &delayOn)) {
        audioState.effectParams.delay_flag.store(delayOn, std::memory_order_relaxed);
    }
    float wet = audioState.effectParams.wet.load(std::memory_order_relaxed);
    if (ImGui::SliderFloat("Wet/Dry##wet", &wet, 0.0f, 1.0f, "%.2f")) {
        audioState.effectParams.wet.store(std::clamp(wet, 0.0f, 1.0f), std::memory_order_relaxed);
    }
    float sr = static_cast<float>(audioState.effectParams.sampleRate.load(std::memory_order_relaxed));
    if (sr > 0.0f) {
        float delayMs = (static_cast<float>(audioState.effectParams.delaySamples.load(std::memory_order_relaxed)) / sr) * 1000.0f;
        if (ImGui::SliderFloat("Delay Time (ms)", &delayMs, 0.0f, 1000.0f, "%.0f ms")) {
            audioState.effectParams.delaySamples.store(static_cast<size_t>((delayMs / 1000.0f) * sr), std::memory_order_relaxed);
        }
    }

    float vol = audioState.effectParams.currentVolume.load(std::memory_order_relaxed);
    ImGui::SeparatorText("Volume");
    ImGui::ProgressBar(vol, ImVec2(-1.0f, 20.0f), "");

    ImGui::Text("Process Time: %lld ns", (long long)audioState.processTime.load(std::memory_order_relaxed));
    ImGui::End();


    ImGui::SetNextWindowSize(ImVec2(work.x - 15, work.y * 0.33f), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(5, 5 + work.y * 0.62f + 5), ImGuiCond_Once);
    ImGui::Begin("Playback");
    AudioMode mode = audioState.audioMode.load(std::memory_order_relaxed);
    const char* modeNames[] = { "Idle", "Recording", "Playing", "Live", "Loop" };
    int modeInt = static_cast<int>(mode);
    if (ImGui::Combo("Mode", &modeInt, modeNames, IM_ARRAYSIZE(modeNames))) {
        if (modeInt == static_cast<int>(AudioMode::PlayingRecording)) {
            audioState.playbackIndex.store(0, std::memory_order_relaxed);
        }
        audioState.audioMode.store(static_cast<AudioMode>(modeInt), std::memory_order_relaxed);
    }

    if (ImGui::Button("Clear Recording")) {
        audioState.recordingHistory.clear();
    }

    ImGui::SeparatorText("Devices");
    {
        int current = -1;
        for (int i = 0; i < (int)audioState.inputDeviceList.size(); i++) {
            if (audioState.inputDeviceList[i] == audioState.inputDevice) {
                current = i;
                break;
            }
        }
        const char* preview = current >= 0 ? audioState.inputDeviceNameList[current].c_str() : "None";
        if (ImGui::BeginCombo("Input Device", preview)) {
            for (int i = 0; i < (int)audioState.inputDeviceList.size(); i++) {
                bool isSelected = (i == current);
                if (ImGui::Selectable(audioState.inputDeviceNameList[i].c_str(), isSelected)) {
                    if (i != current) {
                        audioState.targetInputDevice.store(audioState.inputDeviceList[i], std::memory_order_relaxed);
                        audioState.deviceChangeRequested.store(true, std::memory_order_relaxed);
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    {
        int current = -1;
        for (int i = 0; i < (int)audioState.outputDeviceList.size(); i++) {
            if (audioState.outputDeviceList[i] == audioState.outputDevice) {
                current = i;
                break;
            }
        }
        const char* preview = current >= 0 ? audioState.outputDeviceNameList[current].c_str() : "None";
        if (ImGui::BeginCombo("Output Device", preview)) {
            for (int i = 0; i < (int)audioState.outputDeviceList.size(); i++) {
                bool isSelected = (i == current);
                if (ImGui::Selectable(audioState.outputDeviceNameList[i].c_str(), isSelected)) {
                    if (i != current) {
                        audioState.targetOutputDevice.store(audioState.outputDeviceList[i], std::memory_order_relaxed);
                        audioState.deviceChangeRequested.store(true, std::memory_order_relaxed);
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::End();
}

static void ImGuiLoop(GLFWwindow* window, int& window_w,int& window_h, AudioState& audioState){
    //start frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //frame components
    ImGuiComponents(audioState);

    //render
    ImGui::Render();
    glfwGetFramebufferSize(window, &window_w, &window_h);
    glViewport(0, 0, window_w, window_h);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void UserInterface::UILoop(AudioState& audioState) {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return;
    }

    int window_w = 650, window_h = 620;
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Realtime Audio Processing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, 460, 500, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(1);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    populateDeviceLists(audioState);

    while (audioState.appRunning.load(std::memory_order_relaxed)) {

        glfwPollEvents();
        if (glfwWindowShouldClose(window)) {
            audioState.appRunning.store(false, std::memory_order_relaxed);
            break;
        }

        ImGuiLoop(window,window_w,window_h,audioState);

    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

