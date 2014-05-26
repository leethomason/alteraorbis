/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_VERTEX_INCLUDED
#define UFOATTACK_VERTEX_INCLUDED

#include "../grinliz/glvector.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glgeometry.h"
#include "../tinyxml2/tinyxml2.h"
#include "enginelimits.h"

class Texture;
class XStream;

struct Vertex
{
	enum {
		POS_OFFSET = 0,
		NORMAL_OFFSET = 12,
		TEXTURE_OFFSET = 24,
		BONE_ID_OFFSET = 32
	};

	grinliz::Vector3F	pos;
	grinliz::Vector3F	normal;
	grinliz::Vector2F	tex;
	U16					boneID;		// 8 bits is fine; needs to be 32 bit aligned structure.
	U16					_pad;

	bool Equal( const Vertex& v ) const {
		if (    pos == v.pos 
			 && normal == v.normal
			 && tex == v.tex )
		{
			return true;
		}
		return false;
	}

	bool Equal( const Vertex& v, float EPS ) const
	{
		if (	pos.Equal( v.pos, EPS ) 
			 && normal.Equal( v.normal, EPS ) 
			 && tex.Equal( v.tex, EPS ) )
		{
			return true;
		}
		return false;
	}
};
	

struct PTVertex
{
	enum {
		POS_OFFSET = 0,
		TEXTURE_OFFSET = 12
	};

	grinliz::Vector3F	pos;
	grinliz::Vector2F	tex;

};


struct PTCVertex
{
	enum {
		POS_OFFSET = 0,
		TEXTURE_OFFSET = 12,
		COLOR_OFFSET = 20
	};

	grinliz::Vector3F	pos;
	grinliz::Vector2F	tex;
	grinliz::Vector4F	color;
};


struct PTVertex2
{
	enum {
		POS_OFFSET		= 0,
		TEXTURE_OFFSET	= 8
	};

	grinliz::Vector2F	pos;
	grinliz::Vector2F	tex;
};


struct BoneData
{
	struct Bone {
		grinliz::IString	name;	
		int					parent;		// -1 if no parent, else index
		// Reference values:
		// These are duplicated per-bone, only needed for the reference
		// skeleton. If it matters, this can be cleaned up.
		grinliz::Vector3F	refPos;
		grinliz::Vector3F	refConcat;

		// Animation values:
		grinliz::Quaternion rotation[EL_MAX_ANIM_FRAMES];
		grinliz::Vector3F	position[EL_MAX_ANIM_FRAMES];

		void Serialize( XStream* xs );
	};

	// nBones is in the Sequence.
	Bone bone[EL_MAX_BONES];

	void Clear() { memset( bone, 0, sizeof(Bone)*EL_MAX_BONES ); }
	void Serialize( XStream* xs );

	const Bone* GetBone( const grinliz::IString& internedName ) const;
	int GetBoneIndex( const grinliz::IString& internedName ) const;
};


#endif //  UFOATTACK_VERTEX_INCLUDED
