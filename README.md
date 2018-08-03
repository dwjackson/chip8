<!--
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

<!--
Copyright 2018 David Jackson
-->

# CHIP-8

This is an emulator for the
[CHIP-8 Virtual CPU](https://en.wikipedia.org/wiki/CHIP-8). It is a simple,
virtual, 8-bit CPU that is primarily meant to be "easy to program in
hexadecimal." This emulator was based on
[Cowgod's technical specification](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
for the CHIP-8. It implements the basic instruction set as well as the 00F9
("EXIT") instruction.

## Components

This system consists of several small programs:

* `chip8` - the actual CHIP-8 emulator that will run programs
* `dis8` - disassessembler for CHIP-8 binary files, outputs pseudo-assembly
* `txt2hex` - convert a hexadecimal text file (hex digits) to a binary file
* `dump8` - dump the hexadecimal content of a binary file

## Building

### Prerequisites

This CHIP-8 interpreter depends on:

* [SDL2](https://www.libsdl.org/index.php)
* pthreads (and the POSIX system in general)

### Compiling

If you cloned the git repo (or even if you just don't see a `configure` file
in the directory) you will need the GNU Autotools to build the software. To
generate the `configure` file (and hence allow building):

```sh
autoreconf -iv
```

After that, use the normal build sequence of:

```sh
$ ./configure
$ make
# make install
```

## License

This project and all its associated files are licensed under the 
[Mozilla Public License v2.0](https://www.mozilla.org/en-US/MPL/2.0/). This
license has an [explanatory FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/).
The license used is compatible with the GPL.
