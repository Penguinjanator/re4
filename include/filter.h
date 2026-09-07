#ifndef FILTER_H
#define FILTER_H

#include "types.h"

// Screen post-process filters (game/filter.cpp dispatches to game/filter00.cpp .. filter0b.cpp).
int FilterInit();
int FilterRoomInit();
void FilterTrans();

void Filter00Init();
void Filter00RoomInit();
void Filter00Trans();
void Filter01Init();
void Filter01RoomInit();
void Filter01Trans();
void Filter02Init();
void Filter02RoomInit();
void Filter02Trans();
void Filter03Init();
void Filter03RoomInit();
void Filter03Trans();
void Filter04Init();
void Filter04RoomInit();
void Filter04Trans();
void Filter05Init();
void Filter05RoomInit();
void Filter05Trans();
void Filter05SetParam(int a, int b, int c, int d, int e, int f, int g, f32 x, f32 y, f32 z);
void Filter06Init();
void Filter06RoomInit();
void Filter06Trans();
void Filter07Init();
void Filter07RoomInit();
void Filter07Trans();
void Filter08Init();
void Filter08RoomInit();
void Filter08Trans();
void Filter09Init();
void Filter09RoomInit();
int Filter09GetbUse();
void Filter0aInit();
void Filter0aRoomInit();
void Filter0aTrans();
void Filter0bInit();
void Filter0bRoomInit();
void Filter0bTrans();

#endif
