# Introduction #

Altera can assign colors at run time to a texture atlas, to provide many variations of assets.

For example, faces:

![http://i48.tinypic.com/1zyww2w.png](http://i48.tinypic.com/1zyww2w.png)

Are created from roughly 100 different false color textures:

![http://i39.tinypic.com/23jvww.png](http://i39.tinypic.com/23jvww.png)

Each false color texture is 256x256 in size. Each face (or ring, or gun, etc.) texture is edited separately, and correct colors are assigned at run time.

You can see the final results in the "Asset Preview" scene in the game.

# False Colors #

## Faces ##

  * red -> skin
  * green -> hair
  * blue -> glasses, tattoo

## Rings ##

  * red -> primary color
  * blue -> effect / glow color
  * green -> secondary color