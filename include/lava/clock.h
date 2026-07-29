#ifndef LAVA_CLOCK_H
#define LAVA_CLOCK_H

#include "lava/timer.h"


typedef struct {
    lvPrecisionTimer timer;
 
    double deltatime; /**< Seconds elapsed during the last frame. */
    int target_fps; /**< Target FPS cap. */
    double target_frametime; /**< 1.0 / target_fps */
 
    double fps; /**< Most recently reported FPS value. */
    double fps_accum_time; /**< Seconds accumulated since FPS was last refreshed. */
    int fps_accum_frames; /**< Frames counted since FPS was last refreshed */
    double fps_update_interval; /**< How often the reported fps value refreshes. */
 
    int initialized; /**< Flag for setting if the timer been started yet. */
} lvClock;

/**
 * @brief Initialize a new clock.
 * 
 * @return lvClock 
 */
lvClock lvClock_new();

/**
 * @brief Update the clock.
 * 
 * This should be called once per frame, at the top (or bottom) of your main loop.
 * Blocks just long enough to keep the app at approximately the targeted FPS.
 * 
 * @param clock Clock.
 * @param target_fps FPS cap. 
 */
void lvClock_tick(lvClock *clock, int target_fps);

/**
 * @brief Seconds elapsed during the last frame (after framerate capping).
 * 
 * @param clock Clock.
 * @return Elapsed time in seconds.
 */
double lvClock_get_delta_time(const lvClock *clock);

/**
 * @brief Most recently measured frames-per-second.
 * 
 * @param clock Clock.
 * @return Most recent FPS value.
 */
double lvClock_get_fps(const lvClock *clock);

/**
 * @brief Set what frequency the FPS is updated in seconds. 
 * 
 * @param clock Clock.
 * @param interval FPS update interval in seconds.
 */
void lvClock_set_fps_interval(lvClock *clock, double interval);


#endif // LAVA_CLOCK_H