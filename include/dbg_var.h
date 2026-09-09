#ifndef DBG_VAR_H
#define DBG_VAR_H

#include "types.h"

// Range-limited variable of the debug tools (db_light.cpp instantiates cVarLoop<u8>). The value and
// its bounds come first, the vptr after them (GCC 2.95 layout); limitUpper/limitLower return the
// value clamped or wrapped after adding `d`.
template <class T>
class cVarRange {
public:
    T val;    // 0x00
    T lower;  // 0x01
    T upper;  // 0x02
    // 0x04 vptr

    void init(const T& lo, const T& hi) {
        lower = lo;
        upper = hi;
    }
    virtual T limitUpper(int d) {
        int v = val + d;
        if (v > upper) {
            v = upper;
        }
        return v;
    }
    virtual T limitLower(int d) {
        int v = val + d;
        if (v < lower) {
            v = lower;
        }
        return v;
    }
    int operator==(int x) { return val == x; }
    T operator--(int) {
        T old = val;
        val = limitLower(-1);
        return old;
    }
    T operator++(int) {
        T old = val;
        val = limitUpper(1);
        return old;
    }
    operator int() { return val; }
};

// Wrapping variant: stepping past a bound continues from the other one.
template <class T>
class cVarLoop : public cVarRange<T> {
public:
    cVarLoop(const T& lo, const T& hi, const T& v) {
        init(lo, hi);
        val = v;
        val = limitUpper(0);
        val = limitLower(0);
    }
    virtual T limitUpper(int d) {
        int v = val + d;
        int range = upper - lower + 1;
        while (v > upper) {
            v -= range;
        }
        return v;
    }
    virtual T limitLower(int d) {
        int v = val + d;
        int range = upper - lower + 1;
        while (v < lower) {
            v += range;
        }
        return v;
    }
};

#endif
