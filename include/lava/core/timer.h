#ifndef LAVA_TIMER_H
#define LAVA_TIMER_H


/**
 * @brief Simple high-precision stopwatch.
 * 
 * Backed by `QueryPerformanceCounter` on Windows and `clock_gettime()
 * (CLOCK_REALTIME)` on POSIX platforms. Resolution should be sub-microsecond on
 * both, though actual achievable precision depends on the OS scheduler.
 * 
 * `elapsed` is also cached on the struct after each stop() call, in case
 * you need to re-read the last measured duration without calling stop()
 * again.
 */
typedef struct _lvPrecisionTimer lvPrecisionTimer;

/**
 * @brief Starts (or restarts) the timer.
 * 
 * @param timer Pointer to lvPrecisionTimer.
 */
static inline void lvPrecisionTimer_start(lvPrecisionTimer *timer);

/**
 * @brief Measures the time elapsed since the last call to @ref lvPrecisionTimer_start.
 * 
 * Can be called multiple times after a single start() to get successively
 * larger elapsed values. Because this function doesn't reset or invalidate
 * the state of the timer, it only recomputes `now - start`.
 * 
 * @param timer Pointer to lvPrecisionTimer.
 * @return Elapsed time in seconds.
 */
static inline double lvPrecisionTimer_stop(lvPrecisionTimer *timer);


#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)

    #include <windows.h>

    typedef struct _lvPrecisionTimer {
        double elapsed;
        LARGE_INTEGER _start;
        LARGE_INTEGER _end;
    } lvPrecisionTimer;

    static inline void lvPrecisionTimer_start(lvPrecisionTimer *timer) {
        QueryPerformanceCounter(&timer->_start);
    }

    static inline double lvPrecisionTimer_stop(lvPrecisionTimer *timer) {
        QueryPerformanceCounter(&timer->_end);

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        timer->elapsed = (double)(timer->_end.QuadPart - timer->_start.QuadPart) / (double)frequency.QuadPart;
        return timer->elapsed;
    }

#else

    #include <time.h>
    #include <unistd.h>

    // TODO: On OSX, frequency can be milliseconds instead of nanoseconds? Needs researching
    #define LV_PRECISION_TIMER_NS_PER_SECOND 1e9

    typedef struct _lvPrecisionTimer {
        double elapsed;
        struct timespec _start;
        struct timespec _end;
        struct timespec _delta;
    } lvPrecisionTimer;

    static inline void lvPrecisionTimer_start(lvPrecisionTimer *timer) {
        clock_gettime(CLOCK_REALTIME, &timer->_start);
    }

    static inline double lvPrecisionTimer_stop(lvPrecisionTimer *timer) {
        clock_gettime(CLOCK_REALTIME, &timer->_end);

        timer->_delta.tv_nsec = timer->_end.tv_nsec - timer->_start.tv_nsec;
        timer->_delta.tv_sec = timer->_end.tv_sec - timer->_start.tv_sec;

        if (timer->_delta.tv_sec > 0 && timer->_delta.tv_nsec < 0) {
            timer->_delta.tv_nsec += LV_PRECISION_TIMER_NS_PER_SECOND;
            timer->_delta.tv_sec--;
        }
        else if (timer->_delta.tv_sec < 0 && timer->_delta.tv_nsec > 0) {
            timer->_delta.tv_nsec -= LV_PRECISION_TIMER_NS_PER_SECOND;
            timer->_delta.tv_sec++;
        }

        timer->elapsed = (double)timer->_delta.tv_sec + (double)timer->_delta.tv_nsec / LV_PRECISION_TIMER_NS_PER_SECOND;
        return timer->elapsed;
    }

#endif


#endif // LAVA_TIMER_H