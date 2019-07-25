//
// Created by Derek Guo on 2019-06-28.
//

#include "Listener.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <string>

const enum SoundIoFormat Listener::prioritized_formats[] = {
    SoundIoFormatFloat32NE,
    SoundIoFormatFloat32FE,
    SoundIoFormatS32NE,
    SoundIoFormatS32FE,
    SoundIoFormatS24NE,
    SoundIoFormatS24FE,
    SoundIoFormatS16NE,
    SoundIoFormatS16FE,
    SoundIoFormatFloat64NE,
    SoundIoFormatFloat64FE,
    SoundIoFormatU32NE,
    SoundIoFormatU32FE,
    SoundIoFormatU24NE,
    SoundIoFormatU24FE,
    SoundIoFormatU16NE,
    SoundIoFormatU16FE,
    SoundIoFormatS8,
    SoundIoFormatU8,
    SoundIoFormatInvalid
};

const int Listener::prioritized_sample_rates[] = {
    48000,
    44100,
    96000,
    24000,
    0
};

Listener::Listener(moodycamel::ReaderWriterQueue<std::vector<float>> &outQ, SoundIo* soundio, char* deviceId) :
listenFlag(), outputQueue(outQ), rc() {
    listenFlag.clear();
    int err;

    if (!soundio) {
        throw std::runtime_error("out of memory\n");
    }

    soundio_flush_events(soundio);

    SoundIoDevice* device = nullptr;
    if (deviceId) {
        for (int i = 0; i < soundio_input_device_count(soundio); i += 1) {
            SoundIoDevice *tempDevice = soundio_get_input_device(soundio, i);
            if (!tempDevice->is_raw && std::strcmp(tempDevice->id, deviceId) == 0) {
                device = tempDevice;
                break;
            }
            soundio_device_unref(tempDevice);
        }
        if (!device) {
            char error_msg[64];
            sprintf(error_msg, "Invalid device id: %s\n", deviceId);
            throw std::runtime_error(error_msg);
        }
    } else {
        device = soundio_get_input_device(soundio, soundio_default_input_device_index(soundio));
        if (!device) {
            char error_msg[64];
            sprintf(error_msg, "No input devices available.\n");
            throw std::runtime_error(error_msg);
        }
    }

    fprintf(stderr, "Device: %s\n", device->name);

    if (device->probe_error) {
        char error_msg[64];
        sprintf(error_msg, "Unable to probe device: %s\n", soundio_strerror(device->probe_error));
        throw std::runtime_error(error_msg);
    }

    soundio_device_sort_channel_layouts(device);

    int sample_rate = 0;
    const int* sample_rate_ptr;
    for (sample_rate_ptr = &prioritized_sample_rates[0]; *sample_rate_ptr; sample_rate_ptr += 1) {
        if (soundio_device_supports_sample_rate(device, *sample_rate_ptr)) {
            sample_rate = *sample_rate_ptr;
            break;
        }
    }
    if (!sample_rate)
        sample_rate = device->sample_rates[0].max;

    SoundIoFormat chosenFormat = SoundIoFormatInvalid;
    for (const SoundIoFormat form : prioritized_formats) {
        if (soundio_device_supports_format(device, form)) {
            chosenFormat = form;
            break;
        }
    }
    if (chosenFormat == SoundIoFormatInvalid)
        chosenFormat = device->formats[0];

    inStream = soundio_instream_create(device);
    if (!inStream) {
        char error_msg[64];
        sprintf(error_msg, "out of memory\n");
        throw std::runtime_error(error_msg);
    }

    inStream->format = chosenFormat;
    inStream->sample_rate = sample_rate;
    inStream->read_callback = readCallback;
    inStream->overflow_callback = overflow_callback;
    inStream->userdata = &rc;

    if ((err = soundio_instream_open(inStream))) {
        char error_msg[64];
        sprintf(error_msg, "unable to open input stream: %s", soundio_strerror(err));
        throw std::runtime_error(error_msg);
    }

    fprintf(stderr, "%s %dHz %s interleaved\n",
            inStream->layout.name, sample_rate, soundio_format_string(chosenFormat));

    const int ring_buffer_duration_seconds = 30;
    int capacity = ring_buffer_duration_seconds * inStream->sample_rate * inStream->bytes_per_frame;
    rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!rc.ring_buffer) {
        throw std::runtime_error("out of memory\n");
    }

    if ((err = soundio_instream_start(inStream))) {
        char error_msg[64];
        sprintf(error_msg, "unable to start input device: %s", soundio_strerror(err));
        throw std::runtime_error(error_msg);
    }
}

Listener::~Listener() {
    soundio_destroy(inStream->device->soundio);
    soundio_instream_destroy(inStream);
}

void Listener::start() {
    listenFlag.test_and_set(std::memory_order_release);
    listenThread = std::thread(&Listener::listen, this);
}

void Listener::stop() {
    listenFlag.clear(std::memory_order_release);
    listenThread.join();
}

std::vector<float> Listener::listen() {
    // Note: in this example, if you send SIGINT (by pressing Ctrl+C for example)
    // you will lose up to 1 second of recorded audio data. In non-example code,
    // consider a better shutdown strategy.
    FILE *out_f = fopen("test.raw", "wb");
    while (listenFlag.test_and_set(std::memory_order_acquire)) {
        soundio_flush_events(inStream->device->soundio);
        sleep(1);
        int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
        char *read_buf = soundio_ring_buffer_read_ptr(rc.ring_buffer);
        size_t amt = fwrite(read_buf, 1, fill_bytes, out_f);
        if ((int)amt != fill_bytes) {
            char error_msg[64];
            sprintf(error_msg, "write error: %s\n", strerror(errno));
            throw std::runtime_error(error_msg);
        }
        soundio_ring_buffer_advance_read_ptr(rc.ring_buffer, fill_bytes);
    }

    return std::vector<float>();
}

void Listener::readCallback(struct SoundIoInStream* inStream, int frameCountMin, int frameCountMax) {
    auto rc = static_cast<RecordContext*>(inStream->userdata);
    struct SoundIoChannelArea* areas;
    int err;

    char *write_ptr = soundio_ring_buffer_write_ptr(rc->ring_buffer);
    int freeBytes = soundio_ring_buffer_free_count(rc->ring_buffer);
    int freeCount = freeBytes / inStream->bytes_per_frame;

    if (freeCount < frameCountMin) {
        fprintf(stderr, "ring buffer overflow\n");
        exit(1);
    }

    int writeFrames = std::min<int>(freeCount, frameCountMax);
    int framesLeft = writeFrames;

    for (;;) {
        int frameCount = framesLeft;

        if ((err = soundio_instream_begin_read(inStream, &areas, &frameCount))) {
            fprintf(stderr, "begin read error: %s", soundio_strerror(err));
            exit(1);
        }

        if (!frameCount)
            break;

        if (!areas) {
            // Due to an overflow there is a hole. Fill the ring buffer with
            // silence for the size of the hole.
            memset(write_ptr, 0, frameCount * inStream->bytes_per_frame);
        } else {
            for (int frame = 0; frame < frameCount; frame += 1) {
                for (int ch = 0; ch < inStream->layout.channel_count; ch += 1) {
                    memcpy(write_ptr, areas[ch].ptr, inStream->bytes_per_sample);
                    areas[ch].ptr += areas[ch].step;
                    write_ptr += inStream->bytes_per_sample;
                }
            }
        }

        if ((err = soundio_instream_end_read(inStream))) {
            fprintf(stderr, "end read error: %s", soundio_strerror(err));
            exit(1);
        }

        framesLeft -= frameCount;
        if (framesLeft <= 0)
            break;
    }

    int advanceBytes = writeFrames * inStream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(rc->ring_buffer, advanceBytes);
}

void Listener::overflow_callback(struct SoundIoInStream *inStream) {
    static int count = 0;
    fprintf(stderr, "overflow %d\n", ++count);
}

std::vector<float> Listener::constantQ(std::vector<float> slice) {
    return std::vector<float>();
}

int Listener::getNote(std::vector<float> cqt) {
    return 0;
}

