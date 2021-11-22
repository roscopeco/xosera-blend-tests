# Full-screen, half-frame video tech demo for Xosera

This is a simple, naive, and brute-force full-screen video
test/demo for Xark's Xosera on rosco_m68k.

https://github.com/XarkLabs/Xosera

Much of the code herein is either written by Xark, or 
heavily based on Xark's code. Portions copyright 
2021 Xark, other portions copyright 2021 Ross Bamford.

The `logo-ball` animation is original work. 

The `walk` animation comes from an uncredited GIF I 
found on the Internet. If it belongs to you and you'd
like credit, or want it removed, please get in touch 
with Ross Bamford.

There's a utility (in the `utils` directory) that converts
individual PNGs into the individual frame XMBs expected
by the demo. These should be named e.g. `0001.xmb`, `0002.xmb`
etc and placed in a directory on your SD card (the directory
is configurable in the main C file, but by default should be
`/walk`). A maximum of 30 frames will be loaded (due to
memory constraints).

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

This will build `myprogram.bin`, which can be uploaded to a board that
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

