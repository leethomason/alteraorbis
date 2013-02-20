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

#include "vertex.h"
#include "../grinliz/glstringutil.h"
#include "../xarchive/glstreamer.h"

using namespace grinliz;

void BoneData::Bone::ToMatrix( grinliz::Matrix4* mat ) const
{
	mat->SetIdentity();
	mat->SetXRotation( grinliz::ToDegree( angleRadians ));
	mat->SetTranslation( 0, dy, dz );
}


void BoneData::Load( const tinyxml2::XMLElement* element )
{
	Clear();
	const tinyxml2::XMLElement* boneEle = element->FirstChildElement( "BoneData" );
	if ( boneEle ) {
		int i=0;
		for( const tinyxml2::XMLElement* ele = boneEle->FirstChildElement( "Bone" );
			 ele && i < EL_MAX_BONES;
			 ++i, ele = ele->NextSiblingElement( "Bone" ))
		{
			if ( ele->FirstAttribute() ) {
				const char* name = ele->Attribute( "name" );
				GLASSERT( name );
				bone[i].name = StringPool::Intern( name );
				ele->QueryAttribute( "angleRadians", &bone[i].angleRadians );
				ele->QueryAttribute( "dy", &bone[i].dy );
				ele->QueryAttribute( "dz", &bone[i].dz );
			}
		}
	}
}


void BoneData::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		if ( !bone[i].name.empty() ) {
			printer->OpenElement( "Bone" );
			printer->PushAttribute( "name", bone[i].name.c_str() );
			printer->PushAttribute( "angleRadians", bone[i].angleRadians );
			printer->PushAttribute( "dy", bone[i].dy );
			printer->PushAttribute( "dz", bone[i].dz );
			printer->CloseElement();
		}
	}
	printer->CloseElement();
}


void BoneData::Serialize( XStream* xs )
{
	XarcOpen( xs, "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		XarcOpen( xs, "bone" );
		XARC_SER_KEY( xs, "name", bone[i].name );
		XARC_SER_KEY( xs, "angle", bone[i].angleRadians );
		XARC_SER_KEY( xs, "dy", bone[i].dy );
		XARC_SER_KEY( xs, "dz", bone[i].dz );
		XarcClose( xs );
	}
	XarcClose( xs );
}



