#ifndef CMANAGER_H
#define CMANAGER_H

#include "types.h"

// Base of every managed work object (cLight, cEsp, cObj, cEm, ...).
// GCC 2.95 places the vptr after the fields of the class that introduces it.
class cUnit {
public:
    u32 be_flag;  // 0x0  bit0: alive, bit9/10: reserved-alive bits (0x601 = in use)
    cUnit* next;  // 0x4  active list link
    // 0x8 vptr

    cUnit() {}
    virtual ~cUnit() { be_flag &= ~0x601; }
    virtual void beginEvent() {}
    virtual void endEvent() {}
    // Works are pool-managed: `delete work` only runs the destructor (be_flag cleared).
    // size_t is `unsigned int` for this compiler; with u32 (unsigned long) GCC 2.95 would not
    // treat this as the usual deallocation function.
    void operator delete(void*, unsigned int) {}
};

// Fixed array work manager. Element stride is the runtime field `size`
// (derived work classes share the manager of their base type).
template <class T>
class cManager {
public:
    T* pArray;         // 0x00 work array
    u32 nArray;        // 0x04 number of works
    u32 size;          // 0x08 sizeof one work
    u8 flag;           // 0x0C
    T* pAlive;         // 0x10 head of active list (cUnit::next)
    u32 x14;           // 0x14
    u32 x18;           // 0x18
    u32 x1C;           // 0x1C
    u32 maxAlive;      // 0x20 peak active count
    const char* name;  // 0x24
    u32 warnDiv;       // 0x28 countActiveWork() warns when free works < nArray / warnDiv
    u32 x2C;           // 0x2C set by init()
    // 0x30 vptr

    cManager(u32 size, u8 flag);
    virtual ~cManager();
    virtual void* memAlloc(u32 size) = 0;
    virtual void memFree() = 0;
    virtual void memClear(T* p, u32 size) = 0;
    virtual void log(const char* fmt, ...);
    virtual void destroy(T* p);
    virtual int construct(T* p, int id) = 0;

    void setName(const char* n);
    int roomInit();
    void init(u32 x);
    u32 countActiveWork();
    T* create(int id);
    T* create();
    T* create(int id, u32 no);
    T* createBack(int id);
    int dieCheck();
    void* arrayAlloc(u32 n);
    void arrayFree();
    void dispWorkNum(int x, int y);

    void addListFront(T* p) {
        T* q;
        for (q = pAlive; q; q = (T*)q->next) {
            if (q == p) {
                log("%s::addListFront() ERROR SET x2 0x%08X", name, p);
                return;
            }
        }
        p->next = pAlive;
        pAlive = p;
    }
    void addListBack(T* p) {
        T* q;
        for (q = pAlive; q; q = (T*)q->next) {
            if (q == p) {
                log("%s::addListBack() ERROR 0x%08X", name, p);
                return;
            }
        }
        if (pAlive == 0) {
            pAlive = p;
            return;
        }
        for (q = pAlive; q->next; q = (T*)q->next) {}
        q->next = p;
        p->next = 0;
    }
};

template <class T>
cManager<T>::cManager(u32 size, u8 flag)
{
    this->size = size;
    this->flag = flag;
    maxAlive = 0;
    name = "Mgr";
    warnDiv = 10;
    pArray = 0;
    nArray = 0;
    pAlive = 0;
    x14 = 0;
    x18 = 0;
    x1C = 0;
}

template <class T>
cManager<T>::~cManager()
{
}

template <class T>
void cManager<T>::log(const char* fmt, ...)
{
}

template <class T>
void cManager<T>::setName(const char* n)
{
    name = n;
}

template <class T>
int cManager<T>::roomInit()
{
    maxAlive = 0;
    pArray = 0;
    nArray = 0;
    pAlive = 0;
    x14 = 0;
    x18 = 0;
    x1C = 0;
    return 1;
}

template <class T>
void cManager<T>::init(u32 x)
{
    x2C = x;
    roomInit();
}

template <class T>
u32 cManager<T>::countActiveWork()
{
    u32 n = 0;
    u32 i;
    for (i = 0; i < nArray; i++) {
        T* p = (T*)((u8*)pArray + size * i);
        if (p->be_flag & 0x601) {
            n++;
        }
    }
    if (n > maxAlive) {
        maxAlive = n;
    }
    if (nArray - n < nArray / warnDiv) {
        log("countActiveWork() warning, %s work remain under 1/%d", name, warnDiv);
    }
    return n;
}

template <class T>
T* cManager<T>::create(int id)
{
    u32 i;
    for (i = 0; i < nArray; i++) {
        T* p = (T*)((u8*)pArray + size * i);
        if (!(p->be_flag & 0x601)) {
            memClear(p, size);
            if (construct(p, id) == 0) {
                log("create()->construct() failed. %s id:%d", name, id);
                return 0;
            }
            addListFront(p);
            countActiveWork();
            return p;
        }
    }
    log("create() failed. %s work is full id:%d", name, id);
    return 0;
}

template <class T>
T* cManager<T>::create()
{
    return create(0);
}

template <class T>
T* cManager<T>::create(int id, u32 no)
{
    if (no >= nArray) {
        return 0;
    }
    T* p = (T*)((u8*)pArray + size * no);
    if (p->be_flag & 0x601) {
        log("create() failed %s id:%d", name, id);
        return 0;
    }
    memClear(p, size);
    construct(p, id);
    addListFront(p);
    countActiveWork();
    return p;
}

template <class T>
T* cManager<T>::createBack(int id)
{
    int i;
    for (i = nArray - 1; i >= 0; i--) {
        T* p = (T*)((u8*)pArray + size * i);
        if (!(p->be_flag & 0x601)) {
            memClear(p, size);
            if (construct(p, id) == 0) {
                log("create()->construct() failed. %s id:%d", name, id);
                return 0;
            }
            addListBack(p);
            countActiveWork();
            return p;
        }
    }
    log("create() failed. %s work is full id:%d", name, id);
    return 0;
}

#endif
