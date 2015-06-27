Altera Orbis 
------------

Thanks for playing Altera Orbis!
grinninglizard.com/altera

How to Play
-----------

http://alteraorbis.wikispaces.com/


About
-----

Altera is a simulation of an alternate world; a world inside
the computer. You play as a Domain Core trying to grow and
survive in an ever changing digital world.

Altera Orbis is a Dungeon-Keeperish / Dwarf Fortressy sort of game set in a 
Star Wars-ian & Tron-esqe world. It keeps changing and developing.

Altera Orbis is in long term playable Beta. The game is playable, the game 
mechanics are in place, and there is a lot to do and explore. But it keeps
changing and growing. Thanks for playing and checking the game out. 
Feedback on the forums is appreciated!


Installing
----------

# Windows

Run the installer; it will install the game into your Program Files (x86) and 
save files into the Documents/AlteraOrbis directory. The Microsoft 
Visual Studio redistributables will be installed as well. You can skip the 
redistributable installation if you already have them, but if in doubt a 
re-install of the redistributables is harmless. You can safely install over 
previous Altera Orbis game installations.

# Linux

Linux is more experimental. It works on the dev Ubuntu box, and your mileage may
vary. Please feel free to bring up any bugs on the forums or an issue in 
Github.

You need the SDL and SDL-mixer runtimes to run Altera Orbis. They are at:

https://www.libsdl.org/

On the dev Ubuntu machine, these commands will automatically install SDL and
the mixer:

```
sudo apt-get install libsdl2-2.0
sudo apt-get install libsdl2-mixer
```

Download and install the linux AlteraOrbisLinx_<version>.tar.gz file. You can
extract it with:

```
tar -zxvf AlteraOrbisLinx_<version>.tar.gz
```

And then to run:
```
cd AlteraOrbis
./alteraorbis
```

Save and log files will be placed in the "save" subdirectory.

Changes in Beta5a
-----------------

- Support back to OpenGL 3.0
- Basic Diplomacy
  - Relationships between teams account for strategic & technical relationship
  - War and Peace treaties
  - Both player & AI can conquor other domains
- UI tweaks
- MOBs will collect fruit, and keep it to eat and heal HP
- Avatar now automatically picks up items

FAQ
---

Why do denizens just stand there or get stuck?

	Make sure you don't have a porch that isn't accessible. They may be 
	trying to get to a facility and can't.

Why is everyone carrying fruit?

	You don't have a distillery, or the distillery can't keep up
	with production. Build another distillery.

Why is a strange blocky flag thing on my core?

	That is a squad destination flag. (Your own look the same.)
	Someone is attacking you.


Controls
--------

'Tab' key toggles between top down and isometric views.
'Home' key centers the camera on your core.
'End' key centers the camera on your avatar; if already centered, the
  end key teleports the avatar home.
'Escape' key backs out of the menus back to 'view' mode.
'Space' pauses the game.

Left-click moves the avatar, selects things, builds.
Left-click & drag builds lots of pavement or ice at once, or clears an area.
Shift-left-click on rock or plants attack the rock/plants.

Right-click & drag moves the camera.
Ctrl-Right-click & drag rotates and zooms the camera.

The arrow keys or WASD moves the camera.
Ctrl-Arrow keys or ctrl-WASD rotates and zooms.

If in build mode, but no building selected (so click "Build" but don't select
a building), then left-click & drag rotates buildings.

F11 toggles full screen.
F3 saves a screenshots. (In Documents/Altera Orbis)

Important Notes
---------------

1. I try to keep save game compatibility in major release #s. (So Beta2a-2c
   all have compatible save games.) This hasn't gone so well with Beta3.
   The game won't let you load an incompatible save file; so if the only
   option is "Generate World" when you upgrade...that really is the only 
   option.
2. The game is intended to run on a tablet (Windows tablets only at this point) 
   but touch controls aren't implemented yet. Although the new control scheme 
   is intended to make touch as seamless as possible.


Filing a Bug
------------

If you find an issue,
1. Make sure your graphics drivers are up to date. I know, it's annoying, and 
   every game says this. But the OpenGL drivers (especially on windows) make  
   a big difference. OpenGL 3.1 has rendering issues. OpenGL 3.2 and higher
   should work.
2. Open a "new issue" at http://code.google.com/p/alteraorbis/issues/list
3. Write down the steps to reproduce
4. IMPORTANT: include the release_log.txt file. It is written anew every time 
   you run the game, so be sure to include the log that show the problem you 
   are submitting.

Extra info for bug:
The rendering features can be toggled with the number keys. If you a rendering 
bug, knowing if it can be toggled on/off is very valuable information. For 
example, if you spot the "blue wall" rendering bug, you can tap (1) to turn off
glow, then (1) to turn glow back on. If the "blue wall" goes away then returns,
it's part of the glow rendering.

1: glow
2: particles
3: voxel
4: shadow
5: bolt


Known Issues
------------

The current bug lists that I'm personally tracking:
https://code.google.com/p/alteraorbis/source/browse/bugs.txt

And reported issues:
https://code.google.com/p/alteraorbis/issues/list
