This is a very early alpha tech test for the game Altera.

More at: grinninglizard.com/altera

Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe
It may require the microsoft redistributables. (Although if you installed Xenowar, these
are already installed.)
http://www.microsoft.com/en-us/download/details.aspx?id=5555


NOTE, at this time, there is NO save game compatibility. Be sure to delete old games.

Getting Started
---------------
Start by tapping "Generate" to create a world. You can re-try if you don't like the 
result. (And if you are a programmer interested in improving the world gen code, it
runs as a stand alone utility.) Back on the title screen, tap "Continue" to start 
in the world.

It is very empty.

Tap the spacebar to toggle on 'fast mode'. Volcanoes, plants, water, and (rarely) 
waterfalls will form in the world. The date is in the upper right. Usually the
world is an interesting place around the end of the 2nd age.

All there is to do at this point is take a look, explore, share some screenshots,
and share what you think.


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
[Control feedback appreciated!]

Left-click the world or mini-map to walk (or teleport) there.
PageUp / PageDown zoom.
Home / End rotate.

- 'Save' saves the game (there is only one save slot)
- 'Load' loads the game

- 'Track' walks places, 'Teleport' teleports there.
- ctrl-click to create a new player.
- Space-Bar toggles between fast sim mode and normal mode. (Fast mode is intended
  for world creation, not a "play the game at 2x" mode. It's very fast.)

The buttons on the right side of the game scene are interesting world features. Tap
to walk or teleport there. (You'll generally need to teleport, unless the feature
is unusually close.)

Test Scenes
-----------

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

