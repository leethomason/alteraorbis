This is a very early alpha tech test for the game Altera.


More at: grinninglizard.com/altera


The “Silver” release is cleaning up issues with the prior (giant) “Gold” release.


Game Fiction (in brief)
-----------------------


Altera ("other") is the world inside the computer. Mother Core wakes at date 0.00 and creates the world. The world is split into Domains, each controlled by a central core. The domains are connected by the grid, a fast travel system that can be accessed with enough permission from Mother Core.


Note: the date and domain name are shown near the character portrait.


The first age (Age 0) is the Age of Fire. The world is covered by volcanoes that shape the landscape. After that volcanoes become less common (but are always present) and vegetation begins to rule the landscape.


Note: the advantage of starting with the pre-generated world is that it is filled out with rock, creatures, and plants.


Au (gold), Crystals, and Energy are the currencies of Altera. Au is used for purchases of both item and buildings. Many items, and some buildings, also requires a crystal. From most to least common, crystals are green, red, blue, and violet.


Note: you can collect gold and crystals, but the economy isn’t connected. Nothing costs anything to build or maintain.


Energy is used to power buildings and keep a domain operating. Visitors bring energy; they enter the world looking for domains that have interesting News, Commerce, Media, or Social, any of which they get at kiosks. If the domain has what the Visitor is looking for, energy is transferred to the domain. Visitors keep track of their favorite domains, and share the links with their friends.


Changes (Silver-3)
-------------------
1. Buildings, rocks, plants, workers, and the ground are now production art. There will be tweaks, but the basic look of the game is in place.
2. Animated things is production art. The animations (the actual movement) is not.
3. Visitors have specific interest: News, Media, Commerce, and Social and will seek out Kiosks to fulfill that interest.
4. Fun tip: to cycle through the colors used to render buildings (only a few is this inital release) press the ‘c’ key.
5. To change between the Ring and Blaster, use the spacebar.


Install
-------


Unzip (to desktop, or downloads directory) and run the game .exe. It may require the microsoft redistributables. (Although if you installed Xenowar, these
are already installed.) http://www.microsoft.com/en-us/download/details.aspx?id=5555


Important Notes
---------------


1. There is NO save game compatibility. Be sure to delete old games.
2. If your avatar gets killed, ctrl-click will create a new one. (You can create a bunch if you wish).


Getting Started
---------------
Tap "Load Established" to bring in a pre-generated world, well populated. That’s the best way to get started. (Later, try "Generate" to create a world from scratch.)


Tap "Continue" to start in the world or continue with the last saved game.


You start near a Domain Core. (Placeholder art.) Mantis will
spawn from the core, your avatar will automatically respond. You can click to move,
or click to focus attention on a particular target.


You can tap on the ground or mini-map to move. If you move between sectors on the
Grid, you will use automatically use the grid travel system.


Red Mantis and Green Mantis will fight one another, as well as you. Green mantis
will travel around the world.


You can collect gold and crystal from enemies you defeat. (And remember, control-click will respawn if you are defeated.)


If you step onto a domain core, you will immediately take control of it. 2 worker bots appear and wander about, waiting for you bidding. You can clear rock or vegetation. Build Ice, which is manufactured rock. And you can build a kiosk.


Once you build a kiosk, Visitors will start to show up and use them.


Filing a Bug
------------
If you find an issue,
1. Make sure your graphics drivers are up to date. I know, it's annoying, and every game says this. But the OpenGL drivers (especially on windows) make a big difference.
2. Open an issue here, by pushing the "new issue" in the upper left; http://code.google.com/p/alteraorbis/issues/list
3. Write down the steps to reproduce
4. IMPORTANT: include the release_log.txt file. It is written anew every time you run the game, so be sure to include the log that show the problem you are submitting.


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
Shows the face, ring, and gun art as they will appear in the game.


* Particle *
The particle scene is both preview and a particle editor. Check out res/particles.xml
if you are interested in editing.


Other scenes:
* render: rendering tests, glow, atlasing
* battle: battle test scene. the main work-in-progress is to get this fulling implemented
* animation: animation preview utiliti


http://grinninglizard.com/altera/