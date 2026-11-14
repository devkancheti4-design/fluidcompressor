/* k32.h -- the kernel32 calls and structures wincrt.c uses, declared by hand (no windows.h on this machine).
   Shared by wincrt.c (the Windows build) and k32_posix.c (the shim that runs wincrt.c on macOS for testing), so the
   two can never disagree on a layout.  Types are fixed-width: DWORD is 32 bits on every platform. */
#ifndef K32_H
#define K32_H
#include <stdint.h>
#include <stddef.h>
typedef void *HANDLE;
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef uint16_t WORD;
#if defined(_WIN32)
#define K32 __declspec(dllimport)
#else
#define K32
#endif
#define K32_INVALID ((HANDLE)(intptr_t)-1)
#define K32_STD_INPUT ((DWORD)-10)
#define K32_STD_OUTPUT ((DWORD)-11)
#define K32_STD_ERROR ((DWORD)-12)
#define K32_GENERIC_READ 0x80000000u
#define K32_GENERIC_WRITE 0x40000000u
#define K32_SHARE_READ 1u
#define K32_SHARE_WRITE 2u
#define K32_CREATE_ALWAYS 2u
#define K32_OPEN_EXISTING 3u
#define K32_OPEN_ALWAYS 4u
#define K32_ATTR_NORMAL 0x80u
#define K32_HEAP_ZERO 8u
#define K32_MEM_COMMIT_RESERVE 0x3000u
#define K32_MEM_RELEASE 0x8000u
#define K32_PAGE_READWRITE 4u
#define K32_INFINITE 0xFFFFFFFFu

typedef struct {                  /* SYSTEM_INFO, 48 bytes on 64-bit */
    WORD arch, reserved; DWORD page_size; void *min_addr, *max_addr; size_t active_mask;
    DWORD n_processors, processor_type, granularity; WORD level, revision;
} K32_SYSTEM_INFO;
typedef struct {                  /* MEMORYSTATUSEX, 64 bytes */
    DWORD length, load; uint64_t total_phys, avail_phys, total_page, avail_page, total_virtual, avail_virtual, avail_ext;
} K32_MEMORYSTATUSEX;
typedef struct {                  /* PROCESS_MEMORY_COUNTERS, 72 bytes on 64-bit */
    DWORD cb, page_faults; size_t peak_working_set, working_set, peak_paged, paged, peak_nonpaged, nonpaged, pagefile, peak_pagefile;
} K32_PROCESS_MEMORY_COUNTERS;
typedef DWORD (*K32_THREAD_FN)(void *);

K32 HANDLE GetStdHandle(DWORD which);
K32 BOOL WriteFile(HANDLE h, const void *p, DWORD n, DWORD *done, void *overlapped);
K32 BOOL ReadFile(HANDLE h, void *p, DWORD n, DWORD *done, void *overlapped);
K32 HANDLE CreateFileA(const char *path, DWORD access, DWORD share, void *sa, DWORD disposition, DWORD flags, HANDLE tmpl);
K32 BOOL CloseHandle(HANDLE h);
K32 BOOL GetFileSizeEx(HANDLE h, int64_t *size);
K32 BOOL SetFilePointerEx(HANDLE h, int64_t dist, int64_t *pos, DWORD method);   /* LARGE_INTEGER by value = 8 bytes in a register */
K32 BOOL DeleteFileA(const char *path);
K32 HANDLE GetProcessHeap(void);
K32 void *HeapAlloc(HANDLE heap, DWORD flags, size_t n);
K32 BOOL HeapFree(HANDLE heap, DWORD flags, void *p);
K32 void *VirtualAlloc(void *addr, size_t n, DWORD type, DWORD protect);
K32 BOOL VirtualFree(void *addr, size_t n, DWORD type);
K32 DWORD GetEnvironmentVariableA(const char *name, char *buf, DWORD size);
K32 char *GetCommandLineA(void);
K32 DWORD GetModuleFileNameA(HANDLE module, char *buf, DWORD size);
K32 __attribute__((noreturn)) void ExitProcess(uint32_t code);
K32 DWORD GetLastError(void);
K32 BOOL QueryPerformanceCounter(int64_t *count);
K32 BOOL QueryPerformanceFrequency(int64_t *freq);
K32 void GetSystemInfo(K32_SYSTEM_INFO *si);
K32 BOOL GlobalMemoryStatusEx(K32_MEMORYSTATUSEX *ms);
K32 BOOL K32GetProcessMemoryInfo(HANDLE process, K32_PROCESS_MEMORY_COUNTERS *pmc, DWORD cb);
K32 HANDLE CreateThread(void *sa, size_t stack, K32_THREAD_FN fn, void *arg, DWORD flags, DWORD *id);
K32 DWORD WaitForSingleObject(HANDLE h, DWORD ms);
#endif
