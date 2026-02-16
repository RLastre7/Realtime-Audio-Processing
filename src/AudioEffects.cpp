#include "AudioEffects.h"


//get volume
float AudioEffects::getRMS(const float* data, unsigned long size) {
    float sum = 0.0f;
    for (unsigned long i = 0; i < size; i++) {
        sum += data[i] * data[i];
    }
    return sqrt(sum / size);
}

//set volume
void AudioEffects::gain(float& data, const float gain) {
    data *= gain;
}

void AudioEffects::overdrive(float& data, float drive) {
    drive = std::max(drive, 0.01f);
    data *= drive;
    data = std::tanh(data);
    data /= std::tanh(drive);
}

void AudioEffects::delay(float& data, int delaySamples, RingBuffer& buffer, float wet, float feedback = 0.5f) {
    float delayed = buffer.getRelativeToHead(-delaySamples); 
    float out = data + delayed * wet;                        
    buffer.push(data + delayed * feedback);                  
    data = out;                                              
}

void AudioEffects::fuzz(float& data,float drive) {
    data *= drive;
    data /= (1.0f + abs(data));
}

void AudioEffects::applyEffects(float& data, AudioState& audioState) {
    auto p_gain = audioState.effectParams.gain.load(std::memory_order_relaxed);
    auto p_drive = audioState.effectParams.drive.load(std::memory_order_relaxed);
    auto p_delaySamples = audioState.effectParams.delaySamples.load(std::memory_order_relaxed);
    auto p_wet = audioState.effectParams.wet.load(std::memory_order_relaxed);

    if (audioState.effectParams.gain_flag) gain(data, p_gain);
    if (audioState.effectParams.overdrive_flag) overdrive(data, p_drive);
    if (audioState.effectParams.fuzz_flag) fuzz(data, p_drive);
    if (audioState.effectParams.delay_flag) delay(data, p_delaySamples, audioState.ringBuffer, p_wet);
    else audioState.ringBuffer.push(data);
}



