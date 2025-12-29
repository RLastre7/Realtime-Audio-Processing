#include <iostream>
#include "portaudio.h"

using std::cout;
using std::endl;
int main() {
    PaError err = Pa_Initialize();

    int num = Pa_GetDeviceCount();
    
    cout << num << endl;

    PaDeviceIndex inputDevice = Pa_GetDefaultInputDevice();

    

    

    Pa_Terminate();
    return 0;
}
