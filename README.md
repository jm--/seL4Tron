# seL4Tron

seL4Tron is a simple version of the snake-style game Tron specifically
designed to run as a main task on top of a seL4 microkernel.
I created seL4Tron because I wanted to learn about seL4 and to have fun
while doing so. It includes the following features:
* Graphics mode
* PS2 keyboard input (libplatsupport)
* File abstraction (libcpio)
* Timer interrupt (libplatsupport)


#License
seL4Tron is copyright (c) 2015, Josef Mihalits, and may be distributed and
modified according to the terms of the BSD 2-Clause license. Please see
"COPYING" for details.

The seL4 userland tools and libraries (not part of this distribution) are
mostly released under the BSD license. The seL4 microkernel, which is also not
included in this distribution, is released under GPL Version 2.
Please see http://sel4.systems/Download/license.pml for details.


# Preliminary Build Instructions
## A) Canonical build instructions
1) Install `repo` and all prerequisites as described here http://sel4.systems/Download/

2) Install additional prerequisites
* GRUB - boot loader (we use the tool `grub-mkrescue`)
* xorriso - ISO image manipulation used by GRUB

3) Download and build the source code (kernel, libraries, seL4Tron)
```
mkdir seL4Tron
cd seL4Tron
repo init -u ssh://git@github.com/jm--/seL4Tron.git -b manifest
repo sync
make tron_defconfig
make
```
If all went well, then the above steps created a bootable ISO cd-rom image
in the ./images folder.

4) Start seL4Tron
```
make run
```
Starts QEMU with a VM booting the ISO image.


#Instructions for Game Play

Navigation:
* The keys for the green player are: `j`, `i`, `l`, `k` (left, up, right, down)
* The keys for the blue player are : `a`, `w`, `d`, `s` (left, up, right, down)

On the start screen:
* Press `1` (or one of the green player's direction keys) to start the game
  in single player mode. Be prepared to play against a very simple-minded
  computer player :)
* Press `2` to start the game in two player mode
* Press `0` to watch two computer controlled players play against each other
* Press `ESC` to quit the game

During game play:
* Press `ESC` to go back to the main screen
* Press `SPACE` to pause the game


#Project Ideas
* Make the graphics look cool
* Improve the game AI, which is currently a simple classifier system:
  for example, add a self-learning component, or add a minimax evaluation
  strategy (that runs when the two players are close to each other)
* Add a "turbo" button (when pressed, a player moves faster for a brief period)
* Port the game to you favorite OS

___
Josef Mihalits, jm.public()gmail.com, August 2015
