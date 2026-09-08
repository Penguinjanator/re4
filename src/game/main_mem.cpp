#include "types.h"
#include "global.h"
#include "main_mem.h"
#include "db_log.h"

extern "C" {
void OSReport(const char* fmt, ...);
void* OSGetArenaLo();
void* OSGetArenaHi();
void OSSetArenaLo(void* lo);
void OSSetArenaHi(void* hi);
void* OSInitAlloc(void* lo, void* hi, int maxHeaps);
int OSCreateHeap(void* start, void* end);
void OSDestroyHeap(int heap);
void OSSetCurrentHeap(int heap);
void* OSAllocFromHeap(int heap, u32 size);
void OSFreeToHeap(int heap, void* p);
s32 OSCheckHeap(int heap);
void OSSetSaveRegion(void* start, void* end);
extern int __OSCurrHeap;
int strcmp(const char* a, const char* b);
char* strcpy(char* dst, const char* src);
char* strrchr(const char* s, int c);
int sprintf(char* buf, const char* fmt, ...);
void* memset(void* dst, int c, unsigned int n);
}

extern char* pRK;

// Fixed memory map of the debug build.
struct SystemMemMap {
    u32 x0;         // 0x00
    u32 elf_end;    // 0x04
    u32 dvd;        // 0x08
    u32 sound;      // 0x0C
    u32 fifo;       // 0x10
    u32 xfb;        // 0x14
    u32 core;       // 0x18
    u32 option;     // 0x1C
    u32 player;     // 0x20
    u32 weapon;     // 0x24  start of the main heap
    u32 heap_end;   // 0x28
    u32 arena_lo;   // 0x2C  OSGetArenaLo() at boot
    u32 usb;        // 0x30
    u32 debug;      // 0x34
};

#define HALT()                                                    \
    {                                                             \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
        *(volatile u32*) 0x11111111 = 0;                          \
    }


MemHeap Heap[MEM_HEAP_NUM];
static SystemMemMap SysMem;
OSHeapDescriptor heap_backup[MEM_HEAP_NUM];  // OSAlloc descriptors of suspended heaps

OSHeapCell* cell_main = NULL;
OSHeapCell* cell_game = NULL;
static OSHeapCell* cell_stage = NULL;
OSHeapCell* cell_dll = NULL;
static u8 Dalloc_flg = 0;
u32 _epy_base = 40;

static OSHeapDescriptor* HeapHead;
u32 arenaLo;
u32 arenaHi;
u8 CurrentHeap;
static u8 CurrentDbgHeap;
void* pMemTile;
static u32 _epy;

void* operator new(unsigned int size)
{
    return mem_calloc(size, "operator new", 0, 1, MEM_HEAP_CURRENT);
}

void* operator new[](unsigned int size)
{
    return mem_calloc(size, "operator new", 0, 1, MEM_HEAP_CURRENT);
}

void operator delete(void* p)
{
    Mem_free(p);
}

void operator delete[](void* p)
{
    Mem_free(p);
}

void SystemMemInit()
{
    SysMem.heap_end = 0x817F4000;
    SysMem.elf_end = 0x80350000;
    SysMem.dvd = 0x80370000;
    SysMem.sound = 0x803F0000;
    SysMem.fifo = 0x80460000;
    SysMem.xfb = 0x80578000;
    SysMem.core = 0x807AC000;
    SysMem.option = 0x807EC000;
    SysMem.player = 0x80904000;
    SysMem.weapon = 0x80974000;
    SysMem.arena_lo = (u32) OSGetArenaLo();
    SysMem.usb = 0x81800000;
    SysMem.debug = 0x8181FB00;
    if (SysMem.arena_lo > 0x8034FFFF) {
        OSReport("ELF size overflow\n");
#line 100 "D:/Bio4/Prog/main_mem.cpp"
        HALT();
    }
    arenaLo = SysMem.weapon;
    arenaHi = (u32) OSGetArenaHi();
    if (SysMem.heap_end > arenaHi) {
        arenaHi = SysMem.heap_end;
    }
    arenaLo = (arenaLo + 0x1F) & ~0x1F;
    arenaHi &= ~0x1F;
    HeapHead = (OSHeapDescriptor*) arenaLo;
    arenaLo = (u32) OSInitAlloc((void*) arenaLo, (void*) arenaHi, MEM_HEAP_NUM);
    OSSetArenaLo((void*) arenaLo);
    OSSetArenaHi((void*) arenaHi);
    memInitHeapTbl();
    MemCreateHeap(0, arenaLo, SysMem.heap_end);
    MemSetCurrentHeap(0);
    pMemTile = NULL;
#line 145
    pRK = (char*) MEM_ALLOC(0x40, 1, MEM_HEAP_CURRENT);
    OSSetSaveRegion(pRK, pRK + 0x40);
    if (strcmp(pRK, "_reset_keep_") != 0) {
        memclr_asm(pRK, 0x40);
        strcpy(pRK, "_reset_keep_");
        OSReport("RESET_KEEP_WORK memory clear...\n");
    }
}

void memInitHeapTbl()
{
    int i;

    for (i = 0; i < MEM_HEAP_NUM; i++) {
        Heap[i].handle = -1;
        Heap[i].start = 0;
        Heap[i].end = 0;
        Heap[i].status = 0;
    }
    memclr_asm(heap_backup, sizeof(heap_backup));
}

void MemSuspendHeap(int no)
{
    int h = Heap[no].handle;

    if (memGetHeapSattus(no) == 0 && h >= 0) {
        heap_backup[h] = HeapHead[h];
        HeapHead[h].free = NULL;
        HeapHead[h].allocated = NULL;
        Heap[no].status = 1;
    }
}

void MemSignalHeap(int no)
{
    int h = Heap[no].handle;

    if (memGetHeapSattus(no) == 1 && h >= 0) {
        HeapHead[h] = heap_backup[h];
        Heap[no].status = 0;
    }
}

int memGetHeapSattus(int no)
{
    return Heap[no].status;
}

int memCheckHeapActive(int no)
{
    return memGetHeapSattus(no) == 0;
}

int MemSetCurrentHeap(int no)
{
    if (memCheckHeapActive(no) && Heap[no].handle >= 0) {
        CurrentHeap = no;
        OSSetCurrentHeap(Heap[CurrentHeap].handle);
        CurrentDbgHeap = CurrentHeap;
        return 1;
    }
    return 0;
}

int MemSetCurrentDbgHeap(int no)
{
    if (memCheckHeapActive(no) && Heap[no].handle >= 0) {
        CurrentDbgHeap = CurrentHeap;
        return 1;
    }
    return 0;
}

u8 MemGetCurrentHeap()
{
    return CurrentHeap;
}

u8 MemGetCurrentDbgHeap()
{
    return CurrentDbgHeap;
}

u32 MemGetHeapStartAddr(int no)
{
    return Heap[no].start;
}

u32 MemGetHeapEndAddr(int no)
{
    return Heap[no].end;
}

u32 MemCheckHeapEnd(int no)
{
    int h = Heap[no].handle;
    OSHeapCell* cell;
    u32 end;

    if (!memCheckHeapActive(no) || h < 0) {
        return 0;
    }
    if (HeapHead[h].allocated == NULL) {
        end = (u32) HeapHead[h].free;
    } else {
        end = 0;
    }
    for (cell = HeapHead[h].allocated; cell != NULL; cell = cell->next) {
        if (end < (u32) cell + cell->size) {
            end = (u32) cell + cell->size;
        }
    }
    return end;
}

int MemCreateHeap(int no, u32 start, u32 end)
{
    if (!memCheckHeapActive(no)) {
        return 0;
    }
    if (Heap[no].handle >= 0) {
        MemDestroyHeap(no);
    }
    OSReport("-- MemCreateHeap %d %08x - %08x  ", no, start, end);
    Heap[no].handle = OSCreateHeap((void*) start, (void*) end);
    if (Heap[no].handle >= 0) {
        Heap[no].start = start;
        Heap[no].end = end;
        OSReport("succeed!!\n");
        return 1;
    }
    OSReport("failed!!\n");
    return 0;
}

int MemDestroyHeap(int no)
{
    if (!memCheckHeapActive(no)) {
        MemSignalHeap(no);
    }
    OSReport("-- MemDestroyHeap %d  ", no);
    if (Heap[no].handle >= 0) {
        OSDestroyHeap(Heap[no].handle);
        Heap[no].handle = -1;
        OSReport("succeed!!\n");
        return 1;
    }
    OSReport("failed!!\n");
    return 0;
}

int MemReplaceHeap(int from, int to)
{
    u32 start;
    u32 end;

    if (CurrentHeap == 0) {
        cell_main = HeapHead[Heap[CurrentHeap].handle].allocated;
    }
    if (CurrentHeap == 1) {
        cell_game = HeapHead[Heap[CurrentHeap].handle].allocated;
    }
    if (CurrentHeap == 2) {
        cell_stage = HeapHead[Heap[CurrentHeap].handle].allocated;
    }
    if (CurrentHeap == 3) {
        cell_dll = HeapHead[Heap[CurrentHeap].handle].allocated;
    }
    if (!memCheckHeapActive(from)) {
        return 0;
    }
    if (!memCheckHeapActive(to)) {
        return 0;
    }
    if (Heap[from].handle >= 0) {
        start = MemCheckHeapEnd(from);
        end = Heap[from].end;
        MemDestroyHeap(from);
    } else {
        start = Heap[to].start;
        end = Heap[to].end;
    }
    if (start == 0) {
        return 0;
    }
    return MemCreateHeap(to, start, end);
}

void MemClearAllHeap()
{
    u32 i;

    for (i = 0; i < MEM_HEAP_NUM; i++) {
        if (Heap[i].handle >= 0) {
            MemDestroyHeap(i);
        }
    }
}

void* mem_alloc(u32 size, const char* file, int line, int flag, int heap)
{
    char str[64] = "";
    u8* p;
    char* name;
    u8* tag;

    if (size == 0) {
        return NULL;
    }
    if (heap == MEM_HEAP_CURRENT) {
        heap = CurrentHeap;
    }
    if (!memCheckHeapActive(heap)) {
        return NULL;
    }
    size = (size + 0x1F) & ~0x1F;
    if (file == NULL) {
        p = (u8*) OSAllocFromHeap(Heap[heap].handle, size);
    } else {
        p = (u8*) OSAllocFromHeap(Heap[heap].handle, size + 0x20);
        name = strrchr(file, '/');
        if (name == NULL) {
            name = (char*) file;
        } else {
            name++;
        }
        sprintf(str, "%s(%d)", name, line);
        str[0x1B] = 0;
        if (p != NULL) {
            tag = p + size;
            tag[0] = 0;
            tag[1] = 'M';
            tag[2] = 'A';
            tag[3] = 'D';
            strcpy((char*) tag + 4, str);
        }
    }
    if (flag == 1 && p == NULL) {
        pLog->err(0, 0, "alloc[%x]:free[%x] %s", size, OSCheckHeap(Heap[heap].handle), str);
    }
    return p;
}

void* mem_calloc(u32 size, const char* file, int line, int flag, int heap)
{
    void* p = mem_alloc(size, file, line, flag, heap);

    if (p != NULL) {
        memclr_asm(p, size);
    }
    return p;
}

void Mem_free(void* p)
{
    Mem_free_h(p, CurrentHeap);
}

void Mem_free_h(void* p, int heap)
{
    if (heap == MEM_HEAP_CURRENT) {
        heap = CurrentHeap;
    }
    if (memCheckHeapActive(heap)) {
        OSFreeToHeap(__OSCurrHeap, p);
    }
}

void SetDebugAlloc()
{
    Dalloc_flg = 1;
}

// The cell header fields the debug code reads (the real OSHeapCell is 0x20 bytes).
struct MemCellHead {
    OSHeapCell* prev;  // 0x00
    OSHeapCell* next;  // 0x04
    s32 size;          // 0x08
};

void ResetDebugAlloc()
{
    OSHeapCell* cell;
    MemCellHead tmp;

    Dalloc_flg = 0;
    for (cell = HeapHead[Heap[CurrentDbgHeap].handle].allocated; cell != NULL; cell = tmp.next) {
        tmp = *(MemCellHead*) cell;
        if (strcmp((char*) cell + cell->size - 8, "toolmem") == 0) {
            Debug_free((u8*) cell + 0x20);
        }
    }
}

void* Debug_alloc(u32 size, int flag)
{
    u8* p;

    if (size == 0) {
        return NULL;
    }
    if (!memCheckHeapActive(CurrentDbgHeap)) {
        return NULL;
    }
    if (Dalloc_flg == 1 && flag == 1) {
        size += 8;
    }
    size = (size + 0x1F) & ~0x1F;
    p = (u8*) OSAllocFromHeap(Heap[CurrentDbgHeap].handle, size);
    if (p != NULL) {
        memclr_asm(p, size);
        if (Dalloc_flg == 1 && flag == 1) {
            strcpy((char*) p + size - 8, "toolmem");
        }
    }
    return p;
}

void Debug_free(void* p)
{
    Debug_free_h(p, CurrentDbgHeap);
}

void Debug_free_h(void* p, int heap)
{
    if (heap == MEM_HEAP_CURRENT) {
        heap = CurrentDbgHeap;
    }
    if (memCheckHeapActive(heap)) {
        memclr_asm(p, ((OSHeapCell*) ((u8*) p - 0x20))->size - 0x20);
        OSFreeToHeap(Heap[CurrentDbgHeap].handle, p);
    }
}

void* MemAlloc(u32 size, int flag)
{
    void* p;

    if (!(pG->flags_6C & 0x200000)) {
#line 646
        p = MEM_ALLOC(size, 1, MEM_HEAP_CURRENT);
    } else {
        p = Debug_alloc(size, flag);
    }
    return p;
}

void MemFree(void* p)
{
    if (pG->flags_6C & 0x200000) {
        Debug_free(p);
    } else {
        Mem_free(p);
    }
}
