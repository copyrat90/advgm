#include "advgm.h"

#include <tonc.h>

extern const unsigned char hell_owo_rld[];
static const unsigned char* my_music = hell_owo_rld;

void my_vblank_callback()
{
    bool success = advgm_vblank_callback();
    ((void)success);
}

int main(void)
{
    irq_init(NULL);
    irq_add(II_VBLANK, my_vblank_callback);
    irq_enable(II_VBLANK);

    VBlankIntrWait();

    advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
    advgm_play(my_music, true);

    for (;;)
    {
        VBlankIntrWait();

        key_poll();

        if (key_hit(1 << KI_A))
        {
            if (advgm_playing())
            {
                if (advgm_paused())
                    advgm_resume();
                else
                    advgm_pause();
            }
            else
                advgm_play(my_music, true);
        }

        if (key_hit(1 << KI_B))
        {
            advgm_stop();
        }

        if (key_hit(1 << KI_START))
        {
            const advgm_master_volume previous_master_volume = advgm_get_master_volume();

            switch (previous_master_volume)
            {
            case ADVGM_MASTER_VOLUME_QUARTER:
                advgm_set_master_volume(ADVGM_MASTER_VOLUME_HALF);
                break;
            case ADVGM_MASTER_VOLUME_HALF:
                advgm_set_master_volume(ADVGM_MASTER_VOLUME_FULL);
                break;
            case ADVGM_MASTER_VOLUME_FULL:
                advgm_set_master_volume(ADVGM_MASTER_VOLUME_QUARTER);
                break;
            default:
                break;
            }
        }
    }
}
