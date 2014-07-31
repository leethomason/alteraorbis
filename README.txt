Altera Orbis 
Beta 2

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

Altera Orbis started as Syndicate reimagined to be a a 
Dungeon-Keeperish / Dwarf Fortressy sort of game set in a 
Star Wars-ian & Tron-esqe world. It keeps changing and 
developing.

Altera Orbis is in Beta, and will be until it's ready;
thanks for playing and checking the game out. Feedback
on the forums is appreciated!


Changes in Beta 2
-----------------

- Fluid physics. Floating objects. Water, magma, and lava affect plants
  and mobs. 
- Circuits to create traps, machine, control water and magma, and defend your 
  domain.
- Tab key now toggles between isometric and top-down view.
- Deeper integration of the effects of fire, shock, and water. (Although more 
  needs to be done.)
- Menu system gives indication of build order.
- Improved world generation.
- Vastly improved memory usage. Smaller and faster save files.
- Simplified code architecture.
- New, simplified, much more stable grid travel code.
- Many bug fixes. Many bugs created. :)
- Art improvements.
- The Arachnoid is now called a Trilobyte
- Drones no longer carry fruit and care heal at domain cores.


FAQ
---
Why do denizens just stand there or get stuck?

	Make sure you don't have a portch that isn't accessible. They may be 
	trying to get to a facility and can't.

Why is everyone carrying fruit?

	You don't have a distillery, or the distillery can't keep up
	with production. Build another distillery.


Controls
--------

Tab key toggles between top down and isometric views.

Left-click moves the avatar, selects things, builds.
Left-click & drag builds lots of pavement or ice at once, or clears an area.

Right-click & drag moves the camera.
Ctrl-Right-click & drag rotates and zooms the camera.

The arrow keys or WASD moves the camera.
Ctrl-Arrow keys or ctrl-WASD rotates and zooms.


Install
-------

Run the installer; it will install the game into your Program Files (x86) and 
save files into the Documents directory. The redistributables will be installed
as well. (If in doubt, re-install of the redistributables is harmless.) You can 
safely install over old game installations.


Important Notes
---------------

1. Save game compatibility in only with the release #. Beta1a to Beta1d can use
   the same save files, but Beta1 and Beta2 do not. Old games will be 
   automatically ignored.
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


Known Issues:
------------

The current bug lists that I'm personally tracking:
https://code.google.com/p/alteraorbis/source/browse/bugs.txt

And reported issues:
https://code.google.com/p/alteraorbis/issues/list


