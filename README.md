# advgm

[advgm](https://github.com/copyrat90/advgm) is a DMG VGM music player for the Game Boy Advance.


## Usage

1. Export the Gameboy VGM from [furnace](https://github.com/tildearrow/furnace).

   ![](papers/export_furnace.png)

   1. Make sure the `System` is set to `Game Boy`,\
      and the `Base Tempo` is set to `150` (= `Tick Rate` is set to `60`).
   1. `file > export...`
   1. Select `VGM` tab.
   1. Set the `format version` to `1.61`.
   1. Click `Export`.

1. Run [`tools/advgm_converter.py`](tools/advgm_converter.py) to turn the exported VGM into the binary format.
   ```py
   # Convert as a binary.
   python tools/advgm_converter.py --input my_music.vgm --output my_output.bin

   # Convert as a C array source.
   #
   # The identifier of the C array would be same as the output filename
   # (In this case, it would be `my_output`.)
   python tools/advgm_converter.py --input my_music.vgm --output my_output.c --c-array

   # You can also give a custom identifier to this C array.
   python tools/advgm_converter.py --input my_music.vgm --output my_output.c --c-array my_music_identifier
   ```

1. Add the converted music binary/source to your project, and pass a pointer of it to the [`advgm_play()`](include/advgm.h#L45).

1. Add [`advgm_vblank_callback()`](include/advgm.h#L40) call in your vblank callback handler.

See [`include/advgm.h`](include/advgm.h) for the API.


## License

This project is licensed under the [BSD Zero Clause License](LICENSE).

You can use this freely without attribution.


## History

This project started as a fork of [akkera102's VGM player (vgm2gba)](https://github.com/akkera102/gbadev-ja-test/tree/main/116_vgm2gba_vblank),
but I've turned it into a seperate repository to properly manage it.
