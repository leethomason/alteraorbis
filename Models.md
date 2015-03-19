# Introduction #

The game uses 3 kinds of models:

  * A simple file. A model in a file (ac). The "vault", for example, is just all the objects in the vault.ac file.
  * Multiple models in a file. The "pyramid" variants are 3 models in one file - pyramid.ac. Each pyramid is a group, and the group has a name. (pyramid0 for example.) The name is how the game engine loads the asset.
  * For animated models and models with sub-components (weapons), there is a 2 level hierarchy. The top level group is the name of the asset (beamgun) and sub-groups define the parts (body, driver, cell, and scope.)

# Groups #

In AC3D, use the F8 keep to bring up the object hierarchy. You can move things between groups and fix the hierarchy in this view.

You can select a set of objects, switch to group mode (upper left), and then tap "group" to create a new group.

# Notes #

**Models must be 1-sided**. Select everything (ctrl-a) and select 1S in the lower left of AC3D.