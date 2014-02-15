This is a very early alpha test for the game Altera.

More at: grinninglizard.com/altera

The "Denizen" release adds basic city building.


How to Play
-----------
Is now on the wiki: http://alteraorbis.wikispaces.com/


Changes (Denizen-2)
-------------------

- Updated art. Power plants are now temples. (Thanks GT / ShroomArts)
- Tombstones. Getting de-rezzed leaves a mark for a while.
- Shield fixes. The shields didn't actually do anything in prior releases. Hopefully fixed now.
- Accomplishment, news, & history tracking systems.
- The "Census" button takes you to a work-in-progress screen of major accomplishments.
- UI on the MOBs to notify when they take actions (talk, notice enemies, loot, etc.)
- General bug fixes
- Guard posts. You can now specify areas for your denizens to congregate.

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

