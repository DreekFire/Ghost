#include <iostream>
#define TSF_IMPLEMENTATION
#define TML_IMPLEMENTATION
#include "MainWindow.h"
#include "Listener.h"
#include "Matcher.h"
#include "Player.h"
#include "Scanner.h"
#include "Teacher.h"

int main() {
    // do UI stuff first
    // make UI
    // when user clicks load score, load score
    // give option to add more parts, etc.
    // give option to switch audio device
    // when user clicks ready, create listener, matcher, player, etc.
    // when user presses start, begin countdown and start playing
    SoundIo *soundio = soundio_create();
    int err = soundio_connect(soundio);
    if (err) {
        fprintf(stderr, "error connecting: %s", soundio_strerror(err));
        return 1;
    }
    soundio_flush_events(soundio);

    SoundIoDevice *selected_device = nullptr;

    int device_index = soundio_default_input_device_index(soundio);
    selected_device = soundio_get_input_device(soundio, device_index);
    if (!selected_device) {
        fprintf(stderr, "No input devices available.\n");
        return 1;
    }

    moodycamel::ReaderWriterQueue<std::vector<float>> q{};
    Listener listener = Listener(q, soundio);
    listener.start();
    sleep(5);
    listener.stop();

    std::cout << "Hello, World!" << std::endl;

    return 0;
}