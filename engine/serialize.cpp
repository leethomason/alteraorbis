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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serialize.h"

#include "../grinliz/glstringutil.h"
#include "model.h"

using namespace grinliz;
using namespace tinyxml2;

void ModelHeader::Load( const gamedb::Item* item )
{
	name = item->Name();

	const gamedb::Item* header = item->Child( "header" );
	nTotalVertices = header->GetInt( "nTotalVertices" );
	nTotalIndices = header->GetInt( "nTotalIndices" );
	flags = header->GetInt( "flags" );
	nAtoms = header->GetInt( "nGroups" );

	bounds.Zero();
	const gamedb::Item* boundsItem = header->Child( "bounds" );
	if ( boundsItem ) {
		bounds.min.x = boundsItem->GetFloat( "min.x" );
		bounds.min.y = boundsItem->GetFloat( "min.y" );
		bounds.min.z = boundsItem->GetFloat( "min.z" );
		bounds.max.x = boundsItem->GetFloat( "max.x" );
		bounds.max.y = boundsItem->GetFloat( "max.y" );
		bounds.max.z = boundsItem->GetFloat( "max.z" );
	}

	memset( metaData, 0, sizeof(MetaData)*EL_MAX_METADATA );
	const gamedb::Item* metaItem = header->Child( "metaData" );
	if ( metaItem ) {
		for( int i=0; i<metaItem->NumChildren(); ++i ) {
			const gamedb::Item* dataItem = metaItem->Child( i );
			metaData[i].name = dataItem->Name();
			metaData[i].value.x = dataItem->GetFloat( "x" );
			metaData[i].value.y = dataItem->GetFloat( "y" );
			metaData[i].value.z = dataItem->GetFloat( "z" );
		}
	}
}


void ModelGroup::Load( const gamedb::Item* item )
{
	textureName = item->GetString( "textureName" );
	nVertex = item->GetInt( "nVertex" );
	nIndex = item->GetInt( "nIndex" );
}


void LoadParticles( grinliz::CDynArray< ParticleDef >* particleDefArr, const char* path )
{
	XMLDocument doc;
	doc.LoadFile( path );

	if ( doc.Error() ) {
		GLASSERT( 0 );
		return;
	}
	const XMLElement* particlesEle = doc.FirstChildElement( "particles" );
	if ( !particlesEle ) {
		GLASSERT( 0 );
		return;
	}

	for( const XMLElement* partEle = particlesEle->FirstChildElement( "particle" );
		 partEle;
		 partEle = partEle->NextSiblingElement( "particle" ) )
	{
		ParticleDef pd;
		pd.Load( partEle );
		particleDefArr->Push( pd );
	}
}


void LoadColorBase( const tinyxml2::XMLElement* element, float* r, float* g, float* b, float* a )
{
	if ( element->Attribute( "gray" ) ) {
		float gray = 1.f;
		element->QueryFloatAttribute( "gray", &gray );
		if ( r ) *r = gray;
		if ( g ) *g = gray;
		if ( b ) *b = gray;
		if ( a ) *a = 1.0f;
	}
	else {
		if ( r ) element->QueryFloatAttribute( "red", r );
		if ( g ) element->QueryFloatAttribute( "green", g );
		if ( b ) element->QueryFloatAttribute( "blue", b );
	}
	if ( a ) element->QueryFloatAttribute( "alpha", a );
}


void LoadColor( const tinyxml2::XMLElement* element, grinliz::Color4F* color )
{
	LoadColorBase( element, &color->r, &color->g, &color->b, &color->a );
}


void LoadColor( const tinyxml2::XMLElement* element, grinliz::Color3F* color )
{
	LoadColorBase( element, &color->r, &color->g, &color->b, 0 );
}


void LoadColor( const tinyxml2::XMLElement* element, grinliz::Vector4F* color )
{
	LoadColorBase( element, &color->x, &color->y, &color->z, &color->w );
}


void LoadVector( const tinyxml2::XMLElement* element, grinliz::Vector3F* vec )
{
	element->QueryFloatAttribute( "x", &vec->x );
	element->QueryFloatAttribute( "y", &vec->y );
	element->QueryFloatAttribute( "z", &vec->z );
}
