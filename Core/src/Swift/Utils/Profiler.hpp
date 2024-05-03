#pragma once

#ifdef APP_PLATFORM_WINDOWS
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#include <new>
#include <cstdlib>

// Note(Jorben): Profiling leaks memory, so don't keep on during any real tests. // TODO: Fix
#define APP_ENABLE_PROFILING 1
#define APP_MEM_PROFILING 0

#if !defined(APP_DIST) && APP_ENABLE_PROFILING

#define APP_MARK_FRAME FrameMark
#define APP_PROFILE_SCOPE(name) ZoneScopedN(name)

#if APP_MEM_PROFILING
void* operator new(size_t size);
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
#endif

#else

#define APP_MARK_FRAME
#define APP_PROFILE_SCOPE

#endif
#endif