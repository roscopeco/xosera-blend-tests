# Demo using playfields A and B and various blend modes

This is a simple test/demo of dual playfield blending for 
Xark's Xosera on rosco_m68k.

https://github.com/XarkLabs/Xosera

All graphic assets (in `assets` directory) are original work.

There's a utility (in the `utils` directory) that converts
individual PNGs into the individual frame XMBs expected
by the demo. These should be named e.g. `0001.xmb`, `0002.xmb`
etc and placed in the appropriate directory on your SD card.

It's recommended to pre-process your PNGs to add dithering
etc for best results, as the utility will just average the
RGB pixels to either black or white if you don't.

Note that this is **not** an example, and does not demonstrate
any 'best practice' or 'right way' to do things - it's just
a fun bit of visual pop I hacked together in a few hours 
spread over a couple of days... 

## Building

```
ROSCO_M68K_DIR=/path/to/rosco_m68k make clean all
```

This will build `xosera-blend-demo.bin`, which can be uploaded to a board that
is running the `serial-receive` firmware.

If you're feeling adventurous (and have ckermit installed), you
can try:

```
ROSCO_M68K_DIR=/path/to/rosco_m68k SERIAL=/dev/some-serial-device make load
```

which will attempt to send the binary directly to your board (which
must obviously be connected and waiting for the upload).

This sample uses UTF-8. It's recommended to run minicom with colour
and UTF-8 enabled, for example:

```
minicom -D /dev/your-device -c on -R utf-8
```

