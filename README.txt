This is an alpha-of-the-beta for the game Altera Orbis.

More at: grinninglizard.com/altera

This implements the basics of the domain vs. the world gameplay.

How to Play
-----------
Is now on the wiki: http://alteraorbis.wikispaces.com/

New Controls
------------

Left-click moves the avatar, selects things, builds.
Left-click & drag builds lots of pavement or ice at once, or clears an area.

Right-click & drag moves the camera.
Ctrl-Right-click & drag rotates and zooms the camera.

The arrow keys move the camera.
Ctrl-Arrow keys rotates and zooms.

Changes (Alpha-Beta)
-------------------

- Buildings and your core are now attacked my MOBs
- Sounds (although the actual sounds still need work)
- Domain start, end, and score UI
- Domains are remembered for the Census
- New controls
- Some animation improvement
- settings.xml file for controlling debug and game settings
- Buildings no longer function if they can't path to the Core
- Performance improvement

Install
-------

Run the installer; it will install the game into your Program Files (x86) and save 
files into the Documents directory. The redistributables will be installed as 
well. (If in doubt, re-install of the redistributables is harmless.)


Important Notes
---------------

1. There is NO save game compatibility. Old games will be automatically ignored.
2. The game is intended to run on a tablet (Windows tablets only at this point) but touch controls aren't implemented yet. Although
   the new control scheme is intended to make touch as seamless as possible.

Filing a Bug
------------

If you find an issue,
1. Make sure your graphics drivers are up to date. I know, it's annoying, and every game says this. But the OpenGL drivers (especially on windows) make a big difference.
2. Open an issue here, by pushing the "new issue" in the upper left; http://code.google.com/p/alteraorbis/issues/list
3. Write down the steps to reproduce
4. IMPORTANT: include the release_log.txt file. It is written anew every time you run the game, so be sure to include the log that show the problem you are submitting.

Extra info for bug:
The rendering features can be toggled with the number keys. If you a rendering bug, knowing if it can be toggled on/off is very valuable information. For example, if you spot the "blue wall" rendering bug, you can tap (1) to turn off glow, then (1) to turn glow back on. If the "blue wall" goes away then returns, it's part of the glow rendering.

1: glow
2: particles
3: voxel
4: shadow
5: bolt


Known Issues:
------------

"BLUE WALL" rendering bug: a giant block of color blocks half the screen. If you see it, please toggle the rendering features (above) to see if anything fixes it.

The current bug lists that I'm personally tracking:
https://code.google.com/p/alteraorbis/source/browse/bugs.txt

And reported issues:
https://code.google.com/p/alteraorbis/issues/list


Test Scenes
-----------

The are a variety of test scenes, in different states. The important one for this release;

* Asset Preview *
Shows the face, ring, and gun art as they will appear in the game.

* Particle *
The particle scene is both preview and a particle editor. Check out res/particles.xml
if you are interested in editing.

Other scenes:
* render: rendering tests, glow, atlasing
* battle: battle test scene. the main work-in-progress is to get this fulling implemented
* animation: animation preview utility
* dialog: screen to test the game menus

http://grinninglizard.com/altera/

