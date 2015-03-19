# Introduction #

These are notes on editing existing animations. Creating new ones is more involved.

Altera uses a skeletal (but not skinned) animation system. Animations are created typically in Blender, although anything that supports BVH export should work.

Animations are stored in sub-directories of 'resin'.

## Notes ##

  * Scaling is bad and confuses the animation system. Animation can only be position & rotation. (That's how the armature is set up.)
  * The model in the blender file doesn't do anything except provide reference. Only the armature is exported.
  * The "base" armature goes from the origin (0,0,0) to the pelvis of the skeleton. This is required by the importer.
  * The bone sizes are determined by the model, so you can't change them. Basically, "pose mode" is fully supported, but you shouldn't edit anything in "edit mode".

## Animations ##

Altera uses the following animations:
  * **stand** Required. The standing pose.
  * **walk** Required. About 1000-1500ms.
  * **gunstand** Present if the model can hold a weapon. Standing while carrying an item.
  * **gunwalk** Present if the model can hold a weapon. The walking animation while holding (and possibly shooting) a blaster.
  * **melee** Present if the model has a melee strike. About 800ms. A strike.
  * **impact** Required. A single frame, about 600ms, used when a character is hit by a powerful weapon.

## default.xml ##

### animation tag ###

The animation in the XML is straightforward.
  * **assetName** is arbitrary, but referred to later in the file
  * **meta** defines an event.
    * 'animation="melee" name="impact"' is the point in the melee animation where the strike occurs. This is used for event generation in the engine. This is required if melee is present, and only the time attribute can change.

```
   <animation filename="human2_male/human2_male_.bvh" assetName="humanMale2Animation">
    <meta animation="melee" name="impact" time="709" />
  </animation>
```

### model tag ###

A model is attached to an animation via the animation attribute:
```
  <model filename="human.ac" modelName="humanFemale" animation="humanFemaleAnimation" origin="1 0 0" shading="flat" polyRemoval="pre">
```

# Viewing an Animation #

Compile the assets with builder and run the game. Then click on the "animation" button to enter animation preview mode. You can then select a model and animation. Full interpolation is supported.

You may also toggle on a weapon or tracking point.