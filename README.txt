This is a very early alpha test for the game Altera.

More at: grinninglizard.com/altera

The "Econ" release adds the basic game mechanics - exploring and building - in a rudimentary form.

Game Fiction (in brief)
-----------------------


Altera ("other") is the world inside the computer. Mother Core wakes at date 0.00 and creates the world. The world is split into Domains, each controlled by a central core. The domains are connected by the grid, a fast travel system that can be accessed with enough permission from Mother Core.

The first age (Age 0) is the Age of Fire. The world is covered by volcanoes that shape the landscape. After that volcanoes become less common (but are always present) and vegetation begins to rule the landscape.

Note: the advantage of starting with the pre-generated world is that it is filled out with rock, creatures, and plants.

Au (gold) and Crystals are the currencies of Altera. Au is used for purchases of both item and buildings. Many items, and some buildings, also requires a crystal. From most to least common, crystals are green, red, blue, and violet.

"Tech" measures the technology of a domain, and more generally refers to its civilization achievement. A domain starts at Tech0 and can rise to Tech3. Building in a domain work better at higher tech levels, and many buildings can only be contstructed once that tech level is achieved.

Visitors bring Tech, which degrades over time. You need a continual stream of visitors, which is done be constructing Kiosks to attract them. Visitors enter the world looking for domains that have interesting News, Commerce, Media, or Social, any of which they get at kiosks. If the domain has what the Visitor is looking for, tech is transferred to the domain. Visitors keep track of their favorite domains, and share the links with their friends.


Changes (Econ)
-----------------


Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe. It may require the microsoft redistributables. (Although if you installed Xenowar, these are already installed.) http://www.microsoft.com/en-us/download/details.aspx?id=5555


Important Notes
---------------

1. There is NO save game compatibility. Be sure to delete old games.
2. If your avatar gets killed, ctrl-click will create a new one. (You can create a bunch if you wish).


Getting Started
---------------

Tap "Load Established" to bring in a pre-generated world, well populated. That’s the best way to get started. (Later, try "Generate" to create a world from scratch.)

Tap "Continue" to start in the world or continue with the last saved game.

The world is more dangerous at the edges, and safer in the center. You want to collect enough gold and crystals as you make your way toward a good spot near the center to build a domain.




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