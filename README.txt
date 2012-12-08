This is a very early alpha tech test for the game Altera.

More at: grinninglizard.com/altera

Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe
It may require the microsoft redistributables. (Although if you installed Xenowar, these
are already installed.)
http://www.microsoft.com/en-us/download/details.aspx?id=5555

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
* dialog: simple drag & drop test
* render: rendering tests, glow, atlasing
* particle: particle editing and playback
* nav: nav mesh and pathfinding tests
* blue smoke: test of the full system on a game-sized world
* noise: initial perlin noise / world gen test
* battle: battle test scene. the main work-in-progress is to get this fulling implemented
* animation: animation preview utiliti
* weapon stat: in development, dumps weapon statistics to disk

http://grinninglizard.com/altera/

