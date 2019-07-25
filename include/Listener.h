//
// Created by Derek Guo on 2019-06-28.
//

#ifndef GHOST_LISTENER_H
#define GHOST_LISTENER_H

#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <libsoundio/soundio.h>
#include <essentia/essentia.h>
#include <readerwriterqueue/readerwriterqueue.h>

class Listener {
private:
    std::atomic_flag listenFlag;
    moodycamel::ReaderWriterQueue<std::vector<float>> &outputQueue;
    std::thread listenThread;
    struct RecordContext {
        SoundIoRingBuffer* ring_buffer;
    } rc;
    SoundIoInStream* inStream;
    static void readCallback(struct SoundIoInStream *inStream, int frameCountMin, int frameCountMax);
    static void overflow_callback(struct SoundIoInStream *inStream);
    std::vector<float> constantQ(std::vector<float> slice);
    int getNote(std::vector<float> cqt);
public:
    static const enum SoundIoFormat prioritized_formats[19];
    static const int prioritized_sample_rates[5];
    Listener(moodycamel::ReaderWriterQueue<std::vector<float>> &outQ, SoundIo* soundio, char* deviceId=nullptr);
    ~Listener();
    void start();
    void stop();
    std::vector<float> listen();
};

#endif //GHOST_LISTENER_H
