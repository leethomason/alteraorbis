#ifndef MODEL_VOXEL_INCLUDED
#define MODEL_VOXEL_INCLUDED

#include "../grinliz/glvector.h"

class Model;

struct ModelVoxel {
	ModelVoxel() { model=0; voxel.Set(-1,-1,-1); at.Zero(); }
	bool ModelHit() const { return model != 0; };
	bool VoxelHit() const { return voxel.x >= 0; }
	bool Hit() const	  { return ModelHit() || VoxelHit(); }
	grinliz::Vector2I Voxel2() const { grinliz::Vector2I v = { voxel.x, voxel.z }; return v; }

	Model*				model;
	grinliz::Vector3I	voxel;
	grinliz::Vector3F	at;
};


#endif // MODEL_VOXEL_INCLUDED
