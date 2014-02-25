This is an alpha test for the game Altera.

More at: grinninglizard.com/altera

The "Food" release adds fruit growth, eating, and elixir distillation.


How to Play
-----------
Is now on the wiki: http://alteraorbis.wikispaces.com/


Changes (Food)
-------------------

- Food cycle: plants create food (when they have farms) and process it in distilleries
- Morale: unhappy denizens will come to bad ends
- Tombstones: tombstones affect morale (to good or ill)
- Improved performance 
- In view mode, you can cycle through your denizens.
- Re-order the UI to be (somewhat) more consistent
- Losing a denizen will now destroy a sleep tube. Sleep tubes are limited by the tech level.
- Denizens now heal when they eat

Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe. It may require the microsoft redistributables. (Although if you installed Xenowar, these are already installed.) http://www.microsoft.com/en-us/download/details.aspx?id=5555


Important Notes
---------------

1. There is NO save game compatibility. Be sure to delete old games.
2. If your avatar gets killed, ctrl-click will create a new one. (You can create a bunch if you wish).
3. The game is intended to run on a tablet (Windows tablets only at this point) and generally will. Good touch support for building / removing isn't in place yet.

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

