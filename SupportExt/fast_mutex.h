/* -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
Copyright (c) 2010-2012 Marcus Geelnard

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef _FAST_MUTEX_H_
#define _FAST_MUTEX_H_

#if __cplusplus > 199711L           // C++11
//#warning "fast_mutex should not be used with C++ >= C++11, consider switching to std::thread"
#include <atomic>

namespace tthread {

// A spinlock based on std::atomic_flag
// Ref: Antony Williams, "C++ Concurrency in Action" (Listing 5.1)
class fast_mutex
{
 private:
#if defined(_MSC_VER) && (_MSC_VER <= 1700)
  std::atomic_flag _lockFlag; // VC++ 2012 does not support non-static data initializers
#else
  std::atomic_flag _lockFlag = ATOMIC_FLAG_INIT;
#endif

 public:
  fast_mutex()
  {
#if defined(_MSC_VER) && (_MSC_VER <= 1700)
    _lockFlag.clear(); // circumvent the lack of non-static data initializers in VC++ 2012
#endif
  }

  void lock()
  {
    while(_lockFlag.test_and_set(std::memory_order_acquire))
    { }
  }

  bool try_lock()
  {
    return !_lockFlag.test_and_set(std::memory_order_acquire);
  }

  void unlock()
  {
    _lockFlag.clear(std::memory_order_release);
  }
};

}

#else // !C++11

/// @file

// Which platform are we on?
#if !defined(_TTHREAD_PLATFORM_DEFINED_)
  #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    #define _TTHREAD_WIN32_
  #else
    #define _TTHREAD_POSIX_
  #endif
  #define _TTHREAD_PLATFORM_DEFINED_
#endif

// Check if we can support the assembly language level implementation (otherwise
// revert to the system API)
// Note: on MSVC, inline assembly is not supported on the ARM and x64 processors
// (see https://docs.microsoft.com/en-us/cpp/assembler/inline/inline-assembler)
#if ((defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))) || \
    (defined(_MSC_VER) && (defined(_M_IX86))) || \
    (defined(__GNUC__) && (defined(__ppc__)))) && !defined(TTHREAD_DISABLE_ASM)
  #define _FAST_MUTEX_ASM_
#else
  #define _FAST_MUTEX_SYS_
#endif

#if defined(_TTHREAD_WIN32_)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #define __UNDEF_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #ifdef __UNDEF_LEAN_AND_MEAN
    #undef WIN32_LEAN_AND_MEAN
    #undef __UNDEF_LEAN_AND_MEAN
  #endif
#else
  #ifdef _FAST_MUTEX_ASM_
    #include <sched.h>
  #else
    #include <pthread.h>
  #endif
#endif

namespace tthread {

/// Fast mutex class.
/// This is a mutual exclusion object for synchronizing access to shared
/// memory areas for several threads. It is similar to the tthread::mutex class,
/// but instead of using system level functions, it is implemented as an atomic
/// spin lock with very low CPU overhead.
///
/// The \c fast_mutex class is NOT compatible with the \c condition_variable
/// class (however, it IS compatible with the \c lock_guard class). It should
/// also be noted that the \c fast_mutex class typically does not provide
/// as accurate thread scheduling as a the standard \c mutex class does.
///
/// Because of the limitations of the class, it should only be used in
/// situations where the mutex needs to be locked/unlocked very frequently.
///
/// @note The "fast" version of this class relies on inline assembler language,
/// which is currently only supported for 32/64-bit Intel x86/AMD64 and
/// PowerPC architectures on a limited number of compilers (GNU g++ and MS
/// Visual C++).
/// For other architectures/compilers, system functions are used instead.
class fast_mutex {
  public:
    /// Constructor.
#if defined(_FAST_MUTEX_ASM_)
    fast_mutex() : mLock(0) {}
#else
    fast_mutex()
    {
  #if defined(_TTHREAD_WIN32_)
      InitializeCriticalSection(&mHandle);
  #elif defined(_TTHREAD_POSIX_)
      pthread_mutex_init(&mHandle, NULL);
  #endif
    }
#endif

#if !defined(_FAST_MUTEX_ASM_)
    /// Destructor.
    ~fast_mutex()
    {
  #if defined(_TTHREAD_WIN32_)
      DeleteCriticalSection(&mHandle);
  #elif defined(_TTHREAD_POSIX_)
      pthread_mutex_destroy(&mHandle);
  #endif
    }
#endif

    /// Lock the mutex.
    /// The method will block the calling thread until a lock on the mutex can
    /// be obtained. The mutex remains locked until \c unlock() is called.
    /// @see lock_guard
    inline void lock()
    {
#if defined(_FAST_MUTEX_ASM_)
      bool gotLock;
      do {
        gotLock = try_lock();
        if(!gotLock)
        {
  #if defined(_TTHREAD_WIN32_)
          Sleep(0);
  #elif defined(_TTHREAD_POSIX_)
          sched_yield();
  #endif
        }
      } while(!gotLock);
#else
  #if defined(_TTHREAD_WIN32_)
      EnterCriticalSection(&mHandle);
  #elif defined(_TTHREAD_POSIX_)
      pthread_mutex_lock(&mHandle);
  #endif
#endif
    }

    /// Try to lock the mutex.
    /// The method will try to lock the mutex. If it fails, the function will
    /// return immediately (non-blocking).
    /// @return \c true if the lock was acquired, or \c false if the lock could
    /// not be acquired.
    inline bool try_lock()
    {
#if defined(_FAST_MUTEX_ASM_)
      int oldLock;
  #if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
      asm volatile (
        "movl $1,%%eax\n\t" // move 1 to eax
        "xchg %%eax,%0\n\t" // try to set the lock bit
        "movl %%eax,%1\n\t" // export our result to a test var
        : "=m" (mLock), "=r" (oldLock) // use a register for output [used to be "=m" (oldLock)]
        : "m" (mLock) // mLock is both input and output [used to be empty]
        : "%eax", "memory"
      );
  #elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
      int *ptrLock = &mLock;
      __asm {
        mov eax,1
        mov ecx,ptrLock
        xchg eax,[ecx]
        mov oldLock,eax
      }
  #elif defined(__GNUC__) && (defined(__ppc__))
      int newLock = 1;
      asm volatile (
        "\n1:\n\t"
        "lwarx  %0,0,%1\n\t"
        "cmpwi  0,%0,0\n\t"
        "bne-   2f\n\t"
        "stwcx. %2,0,%1\n\t"
        "bne-   1b\n\t"
        "isync\n"
        "2:\n\t"
        : "=&r" (oldLock)
        : "r" (&mLock), "r" (newLock)
        : "cr0", "memory"
      );
  #endif
      return (oldLock == 0);
#else
  #if defined(_TTHREAD_WIN32_)
      return TryEnterCriticalSection(&mHandle) ? true : false;
  #elif defined(_TTHREAD_POSIX_)
      return (pthread_mutex_trylock(&mHandle) == 0) ? true : false;
  #endif
#endif
    }

    /// Unlock the mutex.
    /// If any threads are waiting for the lock on this mutex, one of them will
    /// be unblocked.
    inline void unlock()
    {
#if defined(_FAST_MUTEX_ASM_)
  #if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
      asm volatile (
        "movl $0,%%eax\n\t" // move 1 to eax
        "xchg %%eax,%0\n\t" // unset the lock bit
        : "=m" (mLock)
        :
        : "%eax", "memory"
      );
  #elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
      int *ptrLock = &mLock;
      __asm {
        mov eax,0
        mov ecx,ptrLock
        xchg eax,[ecx]
      }
  #elif defined(__GNUC__) && (defined(__ppc__))
      asm volatile (
        "sync\n\t"  // Replace with lwsync where possible?
        : : : "memory"
      );
      mLock = 0;
  #endif
#else
  #if defined(_TTHREAD_WIN32_)
      LeaveCriticalSection(&mHandle);
  #elif defined(_TTHREAD_POSIX_)
      pthread_mutex_unlock(&mHandle);
  #endif
#endif
    }

  private:
#if defined(_FAST_MUTEX_ASM_)
    int mLock;
#else
  #if defined(_TTHREAD_WIN32_)
    CRITICAL_SECTION mHandle;
  #elif defined(_TTHREAD_POSIX_)
    pthread_mutex_t mHandle;
  #endif
#endif
};

}

#endif // !C++11

#endif // _FAST_MUTEX_H_
