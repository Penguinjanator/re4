#ifndef TRANS_H
#define TRANS_H

#include "types.h"
#include "model.h"

// game/trans.cpp (C linkage)
extern "C" {
void ModelTrans(cModel* model);
}

#endif
