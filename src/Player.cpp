//
// Created by Derek Guo on 2019-06-28.
//

#include <iostream>
#include "Player.h"

Player::Player() {
    int err;
    struct SoundIo *soundIo = soundio_create();
    if ((err = soundio_connect(soundIo))) {
        std::cout << "An error occurred: " << err << '\n';
    }
    soundio_flush_events(soundIo);
    int defaultOutDeviceId = soundio_default_output_device_index(soundIo);
    //error checking
    struct SoundIoDevice *device = soundio_get_output_device(soundIo, defaultOutDeviceId);
    //error checking

    struct SoundIoOutStream *outStream = soundio_outstream_create(device);
    outStream->format = SoundIoFormatFloat32NE;
    outStream->write_callback = writeCallback;
}

/*Player::~Player() {
    // clean up soundio stuff
}*/

void Player::writeCallback(struct SoundIoOutStream *outStream, int frameCountMin, int frameCountMax) {

}