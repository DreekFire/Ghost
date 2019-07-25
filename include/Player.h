//
// Created by Derek Guo on 2019-06-28.
//

#ifndef GHOST_PLAYER_H
#define GHOST_PLAYER_H
#include "libsoundio/soundio.h"
#include "tinysoundfont/tsf.h"


class Player {
    Player();
    //~Player();
    //void playNote(int note);
    static void writeCallback(struct SoundIoOutStream *outStream, int frameCountMin, int frameCountMax);
};


#endif //GHOST_PLAYER_H
