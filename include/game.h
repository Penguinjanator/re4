#ifndef GAME_H
#define GAME_H

#include "types.h"

// game/game.cpp: save data front end.
class cGameSave {
public:
    u8 pad_0;

    void* alloc();
    void save(void* data);
};

extern cGameSave GameSave;
extern void* pSaveData;

extern "C" {
void GameTask();
void primInit();
void primFree();
void GameLoad();
void GameContinue(int mode);
void GamePointInit(int mode);
}

#endif
