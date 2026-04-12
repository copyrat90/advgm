# Synchronizing with Maxmod

It is possible to synchronize advgm playback with [Maxmod](https://codeberg.org/blocksds/maxmod),
so that you can use PCM channels along with the PSG channels for your music.

[![](https://img.youtube.com/vi/1q3ZnINw7EY/hqdefault.jpg)](https://www.youtube.com/watch?v=1q3ZnINw7EY "Youtube video: GBA PSG+PCM audio juggling")


## Preparing music

Maxmod only allows `*.it`, `*.xm`, `*.s3m`, `*.mod` as the input module,
so you need to use *two different trackers* to create a combined music.

Obviously, two modules must have the same order of patterns/rows/speed and loop points to be synced properly.

What's not obvious is that **you need to set the tempo to 150 for the Furnace module when exporting VGM, regardless of the target tempo**.\
This includes virtual tempo too, so just make sure that every tempo field is filled with 150 when exporting VGM:

![](papers/export_furnace_sync.png)

For the Maxmod tracker module, on the other hand, you would want to set it to the target tempo:

![](papers/export_openmpt_sync.png)


## Synchronizing

### Play / Stop

In order to synchronize the playback, you need to use a *timer interrupt* to update the advgm playback, instead of updating it once per frame.\
See `am_sync_play()`, `am_sync_vblank_interrupt_handler()` and `am_sync_timer1_interrupt_handler()` in [`src/am_sync.c`](src/am_sync.c) to know how to setup the timer.

To align the tunes properly, You need to start updating advgm playback on the *second* VBlank callback after a Maxmod playback has been started.\
This is because Maxmod prepares a frame of audio for the next VBlank, and swaps the mixing buffer in VBlank to actually start playing it.

But as your game logic started the music, the buffer swapped on the first `mmVBlank()` callback has no audio mixed, because `mmFrame()` was never called yet to mix the samples.\
So you need to wait for an additional VBlank, hence you need to wait for the second one.

That's the basics, but actually there's more to it.\
Initially, Maxmod starts mixing the sample *without processing its first tick*, so the actual audible playback is further delayed.

How to calculate this is somewhat complicated, so just check out `am_sync_play()` in [`src/am_sync.c`](src/am_sync.c).


### Pause / Resume

If you also want to support pause/resume, you also need to consider the tick difference between advgm and Maxmod when the playback is paused.\
See `am_sync_pause()`, `am_sync_resume()` and `am_sync_maxmod_tick_callback_handler()` in [`src/am_sync.c`](src/am_sync.c) for that.

For less headaches, I just fast-forward the advgm playback in `am_sync_pause()` so that the tick is the same as Maxmod.


## Note

It seems that **libtonc's interrupt handler misses interrupts** when 2 or more interrupts happen too close to each other.\
We're using both VBlank and Timer1 interrupt, so this could happen (I learned this the hard way.)

**Don't use libtonc's interrupt handler**, use other implementations instead.

I'm using [libugba](https://codeberg.org/SkyLyrac/libugba)'s interrupt handler for this example.


## Known Bug

If you play a music at a *super slow* tempo, the advgm playback slowly falls behind with the Maxmod playback.

But you can only see this bug when you slow down the music with `mmSetModuleTempo()` for an already pretty slow music.\
For example, slowing down the 41 bpm music to 0.5x speed (i.e. `mmSetModuleTempo(0x200)`), so that it's 20.5 bpm, would cause this bug.

But I don't think there's a practical use for such a slow tempo, so I'm leaving the bug for now.\
And it takes at least a million of elapsed frames to notice the discrepancy, anyways.


## Licenses

* The track used in this example is made by [potatoTeto](https://www.potatoteto.com/), and is licensed under the [CC BY-NC 4.0 International](licenses/galactic_quest_mus_theme_c.txt).
* [Maxmod](https://codeberg.org/blocksds/maxmod) is licensed under [its own permissive license](licenses/maxmod.txt).
* [libugba](https://codeberg.org/SkyLyrac/libugba) is licensed under the [MIT license](licenses/libugba.txt).
