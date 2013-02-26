This is a very early alpha tech test for the game Altera.

More at: grinninglizard.com/altera

Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe
It may require the microsoft redistributables. (Although if you installed Xenowar, these
are already installed.)
http://www.microsoft.com/en-us/download/details.aspx?id=5555


NOTE, at this time, there is NO save game compatibility. Be sure to delete old games.

Filing a Bug
------------
If you find an issue,
1. Make sure your graphics drivers are up to date. I know, it's annoying, and every
   game says this. But the OpenGL drivers (especially on windows) make a big difference.
2. Open an issue here, by pushing the "new issue" in the upper left;
   http://code.google.com/p/alteraorbis/issues/list
3. Write down the steps to reproduce
4. IMPORTANT: include the release_log.txt file. It is written anew every time you
   run the game, so be sure to include the log that show the problem you are submitting.


Game Tech
---------
A basic map can be created, you can walk around, save, and load.

'Generate' or the title screen will generate a new map.
'Continue' will load the last saved game or start a newly generated map.
[Bug: you can 'continue' without generating the world, which will crash the game]

In the main game screen:
[Control feedback appreciated! These work on my laptop.]

Left-click the world or mini-map to walk there.
PageUp / PageDown zoom.
Home / End rotate.

- 'Save' saves the game (there is only one save slot)
- 'Load' loads the game

- 'Track' walks places, 'Teleport' teleports there.
- shift-click to create a new player
- Space-Bar will flip it to fast mode.

The buttons on the right side of the game scene are interesting world features. Tap
to walk or teleport there.

Features
--------

The are a variety of test scenes, in different states. The important one for this release;

* Live Preview *
This scene allows you to edit face art and ring (melee weapon) art and see the result as
soon as you save the .png file.

http://code.google.com/p/alteraorbis/wiki/ProceduralRendering

* Asset Preview *
Shows the face and ring art as they will appear in the game. The texture is 
pulled from the database file.

Other scenes:
* render: rendering tests, glow, atlasing
* particle: particle editing and playback
* battle: battle test scene. the main work-in-progress is to get this fulling implemented
* animation: animation preview utiliti

http://grinninglizard.com/altera/

