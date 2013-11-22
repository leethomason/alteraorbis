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

1. New fancy and fixed rendering system. Better management of GPU memory. (Hopefully) gets Altera running on many more systems, including Intel and nVidia.
2. Upgrade underlying SDL framework the game uses.
3. Forges / Factory: ability to create a factory and make weapons.
4. Vault: ability to use vaults to store things. Worker Bots will take world items to vaults.
5. Improvements to character screen.
6. Map scene, information about the nearby domains.
7. Improve the widget system used by the game.
8. Trolls! Trolls wander the world creating trouble. They can use items that they find.


Install
-------

Unzip (to desktop, or downloads directory) and run the game .exe. It may require the microsoft redistributables. (Although if you installed Xenowar, these are already installed.) http://www.microsoft.com/en-us/download/details.aspx?id=5555


Important Notes
---------------

1. There is NO save game compatibility. Be sure to delete old games.
2. If your avatar gets killed, ctrl-click will create a new one. (You can create a bunch if you wish).
3. The game is intended to run on a tablet (Windows tablet, at this point) and generally will. Good touch support for building / removing isn't in place yet.


Getting Started
---------------

** Adventuring **

Tap "Load Established" to bring in a pre-generated world, well populated. That’s the best way to get started. (Later, try "Generate" to create a world from scratch.)

Tap "Continue" to start in the world or continue with the last saved game.

The world is more dangerous at the edges, and safer in the center. You want to collect enough gold and crystals as you make your way toward a good spot near the center to build a domain.

Tap (left click) where you want to move.

In an uncontrolled domain, you can tap rock or plants to remove them.

You can tap a monster to attack; if not doing something else, your avatar will automatically attack monsters as they approach. The space bar swaps between your first 2 weapons (generally a ring and a gun). You can order weapons and choose shields on the character screen.

If items are dropped by the monsters, a list (nearest at top) of items you can pick up appears on the left side of the screen. Tap what you wish to pick up. You pick up gold and crystal automatically when you walk over it.

Tap your face portrait to bring up the character screen. You can order you weapons here and drop extra items.

Tap the mini-map to see the local area and world overview. The map shows your grid travel destination, the location of your domain (once you build it), and a scan of nearby domains. The nearby domain view is important to navigating the world. It shows monster counts with lower/equivalent/higher threat rankings than your avatar. Keep in mind that enough numbers of lower level monsters will still overwhelm you. Additionally, it shows a count of Greater MoBs (G1 for example) which represent very deadly threats.

You can either exit the mini-map at your currect location, or initiate grid travel.

You will need about 300 gold to build a domain. (But you can get started with significantly less and add you go.)

** Building a Domain **

If you step onto a domain core, you will immediately take control of it. 2 worker bots appear and wander about, waiting for you bidding. You can have them clear rock or vegetation. You will need clear paths from your core to the ports so that visitors can get to your domain.

You can use Pave to create walkways that won't get overrun by vegitation.

Buildings:

* ICE is artificial stone. You can use it for walls and to organize your domain.
* Kiosks attract Visitors, which bring Tech. There are different kinds of kiosks; at this stage, there isn't a strategic advantage to specializing, so should probably build one (at least) of each.
* A Vault is a place to store items you create. Worker Bots will bring any gold and crystal they collect to your vault, where it is deposited in your account.
* A Factory (or Forge - not sure of the name yet) allows you to build Guns, Rings, and Shields. Before you tap "Build" the weapon with typical stats will be shown. When you press build, your avatar's level and random factors are applied, and the resulting stats are shown.
* Power buildings hold tech. (Working on a better name for the next build of the game.) You need one power level for each level of tech.

You can build additional Worker Bots (up to a limit) and at any time eject from your core. You can't build while not controlling your core, but the AI will carry on without you. In this alpha, you are the sole defender of your domain and will need to eject to fend of attackers.

You can use a Factory or Vault my moving your avatar to the grid square at the front. The Factory or Vault screen will automatically open.


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
* animation: animation preview utility
* dialog: screen to test the game menus


http://grinninglizard.com/altera/
