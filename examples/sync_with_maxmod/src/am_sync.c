// SPDX-FileCopyrightText: Copyright 2026 copyrat90
// SPDX-License-Identifier: 0BSD

#include "am_sync.h"

#include <advgm.h>
#include <advgm_hardware.h>
#include <maxmod.h>
#include <mm_mas.h>

#include <stdint.h>

// To calculate the tempo, you need to reference the hidden maxmod fields directly.
#include <core/player_types.h>
extern mpl_layer_information mmLayerMain;
extern mm_word mm_mixlen;
extern mm_word mm_bpmdv;

#define AM_MEMORY_BARRIER asm volatile("" ::: "memory")

#define AM_DISABLE_TIMER1_IRQ \
    do \
    { \
        ADVGM_REG_IE &= ~ADVGM_IRQF_TIMER1; \
    } while (false)

#define AM_ENABLE_TIMER1_IRQ \
    do \
    { \
        /* Clear previous IF to avoid race condition because of */ \
        /* the delay between timer overflow and timer interrupt */ \
        if (ADVGM_REG_IF & ADVGM_IRQF_TIMER1) \
            ADVGM_REG_IF = ADVGM_IRQF_TIMER1; \
\
        ADVGM_REG_IE |= ADVGM_IRQF_TIMER1; \
    } while (false)

static ADVGM_EWRAM_BSS struct am_synced_play_states
{
    bool playing;
    bool paused;
    bool loop;

    bool vblank_handled;
    bool timer1_handled;

    bool startup_delay_published;

    uint16_t snddmgcnt_on_pause;

    // Base timer value (quotient of cycles/64)
    uint16_t regular_tm_data;

    // Startup-only timer value
    uint16_t startup_tm_data;

    unsigned startup_vblank_delay_counter;
    unsigned startup_vblank_delay_needed;

    // Accumulate the sub-tick of a 64Hz timer.
    //
    // Denominator is always 8 (= 64/8), so it "overflows" on 8.
    // when that happens, `REG_TM1D` is set to `-(regular_tm_data + 1)`, instead of `-regular_tm_data`
    unsigned timer_accumulator;

    // This is accumulated to `timer_accumulator` everytime the advgm playback is updated.
    unsigned timer_remainder;

    int advgm_update_counter;
    int maxmod_update_counter;

} play_states;

void am_sync_stop(void)
{
    // Disable the timer1 interrupt beforehand
    // to avoid race between timer overflow and timer interrupt.
    AM_DISABLE_TIMER1_IRQ;

    // Stop the timer1
    ADVGM_REG_TM1CNT = ADVGM_TMxCNT_STOP;

    // Reset the playing flag ASAP so that the
    // vblank interrupt handler can't do weird things.
    AM_MEMORY_BARRIER;
    play_states.playing = false;
    AM_MEMORY_BARRIER;

    mmStop();
    advgm_stop();

    play_states.startup_vblank_delay_counter = 0;
    play_states.paused = false;
    play_states.vblank_handled = false;
    play_states.timer1_handled = false;

    play_states.advgm_update_counter = 0;
    play_states.maxmod_update_counter = 0;

    play_states.startup_vblank_delay_needed = 0;
    AM_MEMORY_BARRIER;
    play_states.startup_delay_published = false;

    // Re-enable the timer1 interrupt again.
    AM_ENABLE_TIMER1_IRQ;
}

static void am_sync_start(void);

void am_sync_play(mm_word maxmod_module_id, const uint8_t* advgm_music, bool loop)
{
    am_sync_stop();

    play_states.playing = true;
    play_states.loop = loop;

    // Starts the playback.
    //
    // Note that the timer1 is not started yet, that's done in the delayed vblank callback.
    // Until then, advgm playback is never updated.
    advgm_play(advgm_music, loop);
    AM_MEMORY_BARRIER;
    mmStart(maxmod_module_id, loop ? MM_PLAY_LOOP : MM_PLAY_ONCE);

    am_sync_start();
}

static void am_sync_start(void)
{
    // These are fetched after `mmStart()` call,
    // because you need to reference the currently playing module info.
    // But `vblank_interrupt_handler()` will be called, so you can have a race condition if you're not careful.
    //
    // Though, it won't be so problematic because you need to at least wait for 2 vblanks.
    const mm_hword samples_per_tick = mmLayerMain.tickrate;
    const mm_hword sample_position = mmLayerMain.sampcount;

    // Calculate the startup delay.
    int samples_to_delay_on_startup = (int)samples_per_tick - sample_position;
    if (samples_to_delay_on_startup < 0)
        samples_to_delay_on_startup = 0;

    samples_to_delay_on_startup +=
        (int)samples_per_tick * (play_states.advgm_update_counter - play_states.maxmod_update_counter);

    // Reference the mixing length from maxmod.
    const mm_word mixlen = mm_mixlen;

    // Don't forget the 2 more vblank delays!
    samples_to_delay_on_startup += 2 * mixlen;
    if (samples_to_delay_on_startup < 0)
        samples_to_delay_on_startup = 0;

    const unsigned startup_delay_quotient = (unsigned)samples_to_delay_on_startup / mixlen;
    const unsigned startup_delay_remainder = (unsigned)samples_to_delay_on_startup % mixlen;

    play_states.startup_vblank_delay_needed = startup_delay_quotient;

    // Reverse the Maxmod sample count calculation to obtain the timer value of the tempo.
    //
    // `MM_MAIN` layer calculations for `mpp_setbpm()` in `maxmod/source/core/mas.c`:
    //   0. layer_info->tickrate := (mm_bpmdv / raw_bpm) & ~1
    //   1. tempo := mm_bpmdv / layer_info->tickrate
    //   2. tick_rate (Hz) := tempo * 2 / 5
    //   3. clock_cycles := (1 << 24) / tick_rate
    //   4. truncated_clock_cycles : truncate above so that it's multiple of 8
    //   5. REG_TM1D := -truncated_clock_cycles / 64 (using 64Hz timer)
    //
    // Maxmod allows the tempo from 16 to 510 (considering `mm_mastertempo` multiplier),
    // So the `REG_TM1D` value for 64Hz timer is between -40947.5 and -1226.625
    const mm_word bpmdv = mm_bpmdv;

    unsigned cycles = (unsigned)((1ULL << 23) * samples_per_tick * 5 / bpmdv);

    // Truncate the clock cycles to multiple of 8
    unsigned timer_numerator = cycles / 8;
    const unsigned timer_denominator = 64 / 8;

    unsigned timer_quotient = timer_numerator / timer_denominator;
    unsigned timer_remainder = timer_numerator % timer_denominator;

    play_states.regular_tm_data = (uint16_t)timer_quotient;
    play_states.timer_remainder = timer_remainder;

    // The startup delay should be converted to timer value, much like above.
    cycles = (unsigned)((1ULL << 23) * startup_delay_remainder * 5 / bpmdv);

    timer_numerator = cycles / 8; // trunc to multiple of 8

    timer_quotient = timer_numerator / timer_denominator;
    timer_remainder = timer_numerator % timer_denominator;

    play_states.startup_tm_data = (uint16_t)timer_quotient;
    play_states.timer_accumulator = timer_remainder;

    // Publish the startup delay values.
    AM_MEMORY_BARRIER;
    play_states.startup_delay_published = true;
}

void am_sync_pause(void)
{
    if (play_states.paused)
        return;

    // You could be already waiting for the startup delay when paused right after resume.
    // In that case, you should invalidate the startup delay first.
    //
    // It must be done before stopping timer, because VBlank interrupt might start a timer in any moment.
    AM_MEMORY_BARRIER;
    play_states.startup_delay_published = false;

    AM_MEMORY_BARRIER;

    // Disable the timer1 interrupt beforehand.
    //
    // This is required, because there's a delay between the timer overflow and the interrupt fire.
    // If you don't do this, it's possible to get the timer interrupt AFTER stopping the timer.
    // Pretty nasty race condition, I learned it the hard way.
    AM_DISABLE_TIMER1_IRQ;

    AM_MEMORY_BARRIER;

    // Stop the timer1
    ADVGM_REG_TM1CNT = ADVGM_TMxCNT_STOP;

    AM_MEMORY_BARRIER;
    mmPause();
    AM_MEMORY_BARRIER;

    // Store this before muting all channels.
    play_states.snddmgcnt_on_pause = ADVGM_REG_SNDDMGCNT;

    // Mute all channels to avoid fast-forward audio pops.
    ADVGM_REG_SNDDMGCNT &=
        ~(ADVGM_SNDDMGCNT_PSG_1_ENABLE_LEFT | ADVGM_SNDDMGCNT_PSG_1_ENABLE_RIGHT | ADVGM_SNDDMGCNT_PSG_2_ENABLE_LEFT |
          ADVGM_SNDDMGCNT_PSG_2_ENABLE_RIGHT | ADVGM_SNDDMGCNT_PSG_3_ENABLE_LEFT | ADVGM_SNDDMGCNT_PSG_3_ENABLE_RIGHT |
          ADVGM_SNDDMGCNT_PSG_4_ENABLE_LEFT | ADVGM_SNDDMGCNT_PSG_4_ENABLE_RIGHT);

    // The next time playback is resumed,
    // the sample position calculations assume that advgm is not fall behind too much.
    //
    // Ideally the advgm playback should be paused later when it catches up the Maxmod's,
    // but that is too painful to manage. (e.g. How to deal with resume right after pause?)
    // so we just fast-forward the playback here.
    while (play_states.advgm_update_counter < play_states.maxmod_update_counter)
    {
        advgm_update();
        ++play_states.advgm_update_counter;
    }

    AM_MEMORY_BARRIER;
    advgm_pause();

    play_states.paused = true;

    AM_MEMORY_BARRIER;

    // Re-enable the timer1 interrupt again.
    AM_ENABLE_TIMER1_IRQ;
}

void am_sync_resume(void)
{
    if (!play_states.paused)
        return;

    play_states.startup_vblank_delay_counter = 0;
    play_states.paused = false;
    play_states.vblank_handled = false;
    play_states.timer1_handled = false;

    play_states.startup_vblank_delay_needed = 0;
    AM_MEMORY_BARRIER;
    play_states.startup_delay_published = false;

    AM_MEMORY_BARRIER;
    advgm_resume();
    AM_MEMORY_BARRIER;

    // Unmute channels.
    ADVGM_REG_SNDDMGCNT = play_states.snddmgcnt_on_pause;

    AM_MEMORY_BARRIER;
    mmResume();
    AM_MEMORY_BARRIER;

    am_sync_start();
}

bool am_sync_playing(void)
{
    return play_states.paused || mmActive();
}

bool am_sync_paused(void)
{
    return play_states.paused;
}

void am_sync_vblank_interrupt_handler(void)
{
    // Skips if not currently playing with sync.
    if (!play_states.playing || !mmLayerMain.isplaying)
        return;

    // Skips if already started the advgm update.
    if (play_states.vblank_handled)
        return;

    // Checks if enough vblank has been passed or not.
    //
    // Startup delay values might not be set to valid values,
    // so we need to check if they are valid to avoid race condition.
    const bool startup_delay_published = play_states.startup_delay_published;
    AM_MEMORY_BARRIER;
    if (!startup_delay_published ||
        play_states.startup_vblank_delay_counter + 1 < play_states.startup_vblank_delay_needed)
    {
        ++play_states.startup_vblank_delay_counter;
        return;
    }

    play_states.startup_vblank_delay_needed = 0;
    AM_MEMORY_BARRIER;
    play_states.startup_delay_published = false;

    // Enough vblank waited, we'll now start the timer
    // (i.e. start the advgm update)
    play_states.vblank_handled = true;

    // Additionally delay the playback considering Maxmod startup delay.
    if (play_states.startup_tm_data != 0)
        ADVGM_REG_TM1D = -(uint16_t)play_states.startup_tm_data;
    else
    {
        play_states.timer1_handled = true;
        am_sync_timer1_interrupt_handler();
    }

    // Start the advgm timer.
    ADVGM_REG_TM1CNT = ADVGM_TMxCNT_PRESCALER_F_DIV_64 | ADVGM_TMxCNT_IRQ_ENABLE | ADVGM_TMxCNT_START;
}

void am_sync_timer1_interrupt_handler(void)
{
    // Accumulate the sub-tick of the 64Hz timer
    play_states.timer_accumulator += play_states.timer_remainder;

    // If sub-tick has been accumulated to form an additional tick
    if (play_states.timer_accumulator >= 8)
    {
        play_states.timer_accumulator -= 8;

        // Wait an additional tick
        ADVGM_REG_TM1D = -(play_states.regular_tm_data + 1);
    }
    else
    {
        ADVGM_REG_TM1D = -play_states.regular_tm_data;
    }

    // If this was the first time advgm updated after play/resume,
    // the timer must be using old delay value, which is invalid now.
    if (!play_states.timer1_handled)
    {
        // Disable the timer1 interrupt beforehand
        // to avoid race between timer overflow and timer interrupt.
        AM_DISABLE_TIMER1_IRQ;

        // Stop the timer, currently running with the invalid delay.
        ADVGM_REG_TM1CNT = ADVGM_TMxCNT_STOP;

        // I've moved `AM_DISABLE_TIMER1_IRQ` and `AM_ENABLE_TIMER1_IRQ` apart as far as possible,
        // so that it's impossible for the previous timer overflow
        // to trigger the re-enabled timer1 interrupt.
        //
        // I heard that it takes 2 instructions for that to happen,
        // So I believe stalling here is not necessary.

        // Restart the timer to use `regular_tm_data` instead of `startup_tm_data`.
        ADVGM_REG_TM1CNT = ADVGM_TMxCNT_PRESCALER_F_DIV_64 | ADVGM_TMxCNT_IRQ_ENABLE | ADVGM_TMxCNT_START;

        AM_MEMORY_BARRIER;
    }

    ++play_states.advgm_update_counter;

    // The above if statement was seperated, to avoid previously described race condition.
    if (!play_states.timer1_handled)
    {
        AM_MEMORY_BARRIER;
        play_states.timer1_handled = true;
        AM_MEMORY_BARRIER;

        // Re-enable the timer1 interrupt.
        AM_ENABLE_TIMER1_IRQ;
    }

    // Update advgm playback last, for the restarted timer accuracy.
    AM_MEMORY_BARRIER;
    const bool success = advgm_update();
    ((void)success);
}

mm_word am_sync_maxmod_tick_callback_handler(mm_word msg, mm_word param)
{
    if (msg != MMCB_SONGTICK || (param & 0xFF) != MM_MAIN)
        return 0;

    // Skip if not played with sync.
    if (!play_states.playing)
        return 0;

    ++play_states.maxmod_update_counter;

    return 0;
}
