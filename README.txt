This is a very early alpha tech test for the game Altera.

More at: grinninglizard.com/altera

Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe
It may require the microsoft redistributables. (Although if you installed Xenowar, these
are already installed.)
http://www.microsoft.com/en-us/download/details.aspx?id=5555

Important Notes
---------------

* There is NO save game compatibility. Be sure to delete old games.
* If your avatar gets killed, ctrl-click will create a new one. (You can create a 
  bunch if you wish).
* "Fast Mode" doesn't work very well. This will probably get removed.

Getting Started
---------------
Tap "Load Established" to bring in a pre-generated world, well populated. Or,
if you prefer, tap "Generate" to create a world. You can re-try if you don't like the 
result. Back on the title screen, tap "Continue" to start in the world or continue
with the last saved game.

You start near a Domain Core. (Placeholder art: it's a monolith.) Mantis will
spawn from the core, your avatar will automatically respond. You can click to move,
or click to focus attention on a particular target.

You can tap on the ground or mini-map to move. If you move between sectors on the
Grid, you will use automatically use the grid travel system.

Red Mantis and Green Mantis will fight one another, as well as you. Green mantis
will travel around the world.


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


Known Issues:
------------
The current bug lists that I'm personally tracking:
https://code.google.com/p/alteraorbis/source/browse/bugs.txt

And reported issues:
https://code.google.com/p/alteraorbis/issues/list

Test Scenes
-----------

The are a variety of test scenes, in different states. The important one for this release;

* Asset Preview *
Shows the face and ring art as they will appear in the game.

* Particle *
The particle scene is both preview and a particle editor. Check out res/particles.xml
if you are interested in editing.

Other scenes:
* render: rendering tests, glow, atlasing
* battle: battle test scene. the main work-in-progress is to get this fulling implemented
* animation: animation preview utiliti

http://grinninglizard.com/altera/

