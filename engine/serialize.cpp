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
#include "../shared/dbhelper.h"
#include "../xegame/game.h"
#include "../engine/particle.h"

using namespace grinliz;
using namespace tinyxml2;

void ModelHeader::Load( const gamedb::Item* item )
{
	name = StringPool::Intern( item->Name(), true );

	const gamedb::Item* header = item->Child( "header" );
	nTotalVertices = header->GetInt( "nTotalVertices" );
	nTotalIndices = header->GetInt( "nTotalIndices" );
	flags = header->GetInt( "flags" );
	nAtoms = header->GetInt( "nGroups" );
	if ( header->HasAttribute( "animation" ) ) {
		animation = StringPool::Intern( header->GetString( "animation" ), true );
	}
	animationSpeed = header->GetFloat("animationSpeed");

	bounds.Zero();
	DBRead( header, "bounds", bounds );

	memset( metaData, 0, sizeof(ModelMetaData)*EL_NUM_METADATA );
	const gamedb::Item* metaItem = header->Child( "metaData" );
	if ( metaItem ) {
		for( int i=0; i<metaItem->NumChildren(); ++i ) {
			const gamedb::Item* dataItem = metaItem->ChildAt( i );
			int id = (int) atof( dataItem->Name() );

			//metaData[i].name = StringPool::Intern( dataItem->Name(), true );
			metaData[id].pos.x = dataItem->GetFloat( "x" );
			metaData[id].pos.y = dataItem->GetFloat( "y" );
			metaData[id].pos.z = dataItem->GetFloat( "z" );

			metaData[id].axis.x = dataItem->GetFloat( "axis.x" );
			metaData[id].axis.y = dataItem->GetFloat( "axis.y" );
			metaData[id].axis.z = dataItem->GetFloat( "axis.z" );
			metaData[id].rotation = dataItem->GetFloat( "rotation" );

			metaData[id].boneName = IString();
			if ( dataItem->HasAttribute( "boneName" )) {
				metaData[id].boneName = StringPool::Intern( dataItem->GetString( "boneName" ), true );
			}
		}
	}

	memset( effectData, 0, sizeof(ModelParticleEffect)*EL_MAX_MODEL_EFFECTS );
	const gamedb::Item* effectItem = header->Child( "effectData" );
	if ( effectData ) {
		for( int i=0; i<effectItem->NumChildren(); ++i ) {
			const gamedb::Item* item = effectItem->Child(i);
			effectData[i].metaData	=item->GetInt( "metaData" );
			effectData[i].name		= StringPool::Intern( item->GetString( "name" ), true );
		}
	}

	memset( modelBoneName, 0, sizeof(const char*)*EL_MAX_BONES );
	const gamedb::Item* boneItem = header->Child( "bones" );
	if ( boneItem && boneItem->NumChildren() > 0 ) {
		for( int i=0; i<boneItem->NumChildren(); ++i ) {

			const gamedb::Item* dataItem = boneItem->ChildAt( i );
			IString name = StringPool::Intern( dataItem->Name(), true );
			int id       = dataItem->GetInt( "id" );

			GLASSERT( modelBoneName[id] == 0 );	// else duplicated bone?
			modelBoneName[id] = name;
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
	else if ( element->Attribute( "palx" ) || element->Attribute( "paly" )) {
		int palx = 0;
		int paly = 0;
		element->QueryAttribute( "palx", &palx );
		element->QueryAttribute( "paly", &paly );
		const Game::Palette* palette = Game::GetMainPalette();
		Color4F c = palette->Get4F( palx, paly );
		*r = c.r;
		*g = c.g;
		*b = c.b;
		*a = 1;
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


void PushType( tinyxml2::XMLPrinter* printer, const char* name, const Vector3F& vec )
{
	printer->OpenElement( name );
	printer->PushAttribute( "type", "Vector3F" );
	printer->PushAttribute( "x", vec.x );
	printer->PushAttribute( "y", vec.y );
	printer->PushAttribute( "z", vec.z );
	printer->CloseElement();
}


void PushType( tinyxml2::XMLPrinter* printer, const char* name, const Vector4F& vec )
{
	printer->OpenElement( name );
	printer->PushAttribute( "type", "Vector4F" );
	printer->PushAttribute( "x", vec.x );
	printer->PushAttribute( "y", vec.y );
	printer->PushAttribute( "z", vec.z );
	printer->PushAttribute( "w", vec.w );
	printer->CloseElement();
}


void PushType( tinyxml2::XMLPrinter* printer, const char* name, const Matrix4& mat )
{
	printer->OpenElement( name );
	printer->PushAttribute( "type", "Matrix4" );
	for( int i=0; i<16; ++i ) {
		CStr<10> str;
		str.Format( "x%0d", i );
		printer->PushAttribute( str.c_str(), mat.x[i] );
	}
	printer->CloseElement();
}


void PushType( tinyxml2::XMLPrinter* printer, const char* name, const Quaternion& q )
{
	printer->OpenElement( name );
	printer->PushAttribute( "type", "Quaternion" );
	printer->PushAttribute( "x", q.x );
	printer->PushAttribute( "y", q.y );
	printer->PushAttribute( "z", q.z );
	printer->PushAttribute( "w", q.w );
	printer->CloseElement();
}


void XEArchive( XMLPrinter* printer, const XMLElement* element, const char* name, Vector2I& vec )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	if ( printer ) {
		printer->OpenElement( name );
		printer->PushAttribute( "type", "Vector2I" );
		printer->PushAttribute( "x", vec.x );
		printer->PushAttribute( "y", vec.y );
		printer->CloseElement();
	}
	if ( element ) {
		vec.Zero();
		const XMLElement* ele = element->FirstChildElement( name );
		GLASSERT( ele );
		if ( ele ) {
			const char* type = ele->Attribute( "type" );
			GLASSERT( type && StrEqual( type,  "Vector2I" ));
			ele->QueryAttribute( "x", &vec.x );
			ele->QueryAttribute( "y", &vec.y );
		}
	}
}

void XEArchive( XMLPrinter* printer, const XMLElement* element, const char* name, Vector3F& vec )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	if ( printer ) 
		PushType( printer, name, vec );
	if ( element ) {
		vec.Zero();
		const XMLElement* ele = element->FirstChildElement( name );
		GLASSERT( ele );
		if ( ele ) {
			const char* type = ele->Attribute( "type" );
			GLASSERT( type && StrEqual( type,  "Vector3F" ));
			ele->QueryFloatAttribute( "x", &vec.x );
			ele->QueryFloatAttribute( "y", &vec.y );
			ele->QueryFloatAttribute( "z", &vec.z );
		}
	}
}


void XEArchive( XMLPrinter* printer, const XMLElement* element, const char* name, Vector4F& vec, const Vector4F* def )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	if ( printer ) {
		if ( !def || (*def != vec) ) {	
			PushType( printer, name, vec );
		}
	}
	if ( element ) {
		if ( def ) 
			vec = *def;
		else
			vec.Zero();

		const XMLElement* ele = element->FirstChildElement( name );
		if ( ele ) {
			const char* type = ele->Attribute( "type" );
			GLASSERT( type && StrEqual( type,  "Vector4F" ));
			ele->QueryFloatAttribute( "x", &vec.x );
			ele->QueryFloatAttribute( "y", &vec.y );
			ele->QueryFloatAttribute( "z", &vec.z );
			ele->QueryFloatAttribute( "w", &vec.w );
		}
	}
}


void XEArchive( XMLPrinter* printer, const XMLElement* element, const char* name, Quaternion& q )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	static const Quaternion DEFAULT;

	if ( printer ) {
		if ( q != DEFAULT ) {
			PushType( printer, name, q );
		}
	}
	if ( element ) {
		q = DEFAULT;
		const XMLElement* ele = element->FirstChildElement( name );
		if ( ele ) {
			const char* type = ele->Attribute( "type" );
			GLASSERT( type && StrEqual( type,  "Quaternion" ));
			ele->QueryFloatAttribute( "x", &q.x );
			ele->QueryFloatAttribute( "y", &q.y );
			ele->QueryFloatAttribute( "z", &q.z );
			ele->QueryFloatAttribute( "w", &q.w );
		}
	}
}


void XEArchive( XMLPrinter* printer, const XMLElement* element, const char* name, Matrix4& m )
{
	GLASSERT( !(printer && element));
	GLASSERT( printer || element );
	GLASSERT( name && *name );

	if ( printer ) 
		PushType( printer, name, m );
	if ( element ) {
		m.SetIdentity();
		const XMLElement* ele = element->FirstChildElement( name );
		GLASSERT( ele );
		if ( ele ) {
			const char* type = ele->Attribute( "type" );
			GLASSERT( type && StrEqual( type, "Matrix4" ));
			for( int i=0; i<16; ++i ) {
				CStr<10> str;
				str.Format( "x%0d", i );
				element->QueryFloatAttribute( str.c_str(), &m.x[i] );
			}
		}
	}
}
