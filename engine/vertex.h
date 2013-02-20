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


struct InstVertex
{
	enum {
		POS_OFFSET = 0,
		NORMAL_OFFSET = 12,
		TEXTURE_OFFSET = 24,
		BONE_ID_OFFSET  = 32,
		INSTANCE_OFFSET = 34
	};

	void From( const Vertex& rhs ) {
		GLASSERT( ((const U8*)&boneID) - ((const U8*)this) == BONE_ID_OFFSET );
		GLASSERT( ((const U8*)&instanceID) - ((const U8*)this) == INSTANCE_OFFSET );

		this->pos = rhs.pos;
		this->normal = rhs.normal;
		this->tex = rhs.tex;
		this->boneID = rhs.boneID;
		instanceID = 0;
	}

	grinliz::Vector3F	pos;
	grinliz::Vector3F	normal;
	grinliz::Vector2F	tex;
	U16					boneID;
	U16					instanceID;	// 8 bits is fine, but this structure should be 32 bit-aligned
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
	// maps to Vector3F
	struct Bone {
		grinliz::IString name;	
		float angleRadians;
		float dy;
		float dz;

		void ToMatrix( grinliz::Matrix4* mat ) const;
	};

	const Bone* GetBone( const grinliz::IString& internedName ) const {
		for( int i=0; i<EL_MAX_BONES; ++i ) {
			if ( bone[i].name == internedName ) {
				return bone+i;
			}
		}
		GLASSERT( 0 );
		return 0;
	}

	Bone bone[EL_MAX_BONES];

	void Clear() { memset( bone, 0, sizeof(Bone)*EL_MAX_BONES ); }
	void Load( const tinyxml2::XMLElement* element );
	void Save( tinyxml2::XMLPrinter* printer );
	void Serialize( XStream* xs );
};


#endif //  UFOATTACK_VERTEX_INCLUDED
