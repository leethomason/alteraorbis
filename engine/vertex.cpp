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

void BoneData::Bone::ToMatrix( grinliz::Matrix4* mat ) const
{
	mat->SetIdentity();
	mat->SetXRotation( grinliz::ToDegree( angleRadians ));
	mat->SetTranslation( 0, dy, dz );
}


void BoneData::Load( const tinyxml2::XMLElement* element )
{
	GLASSERT( 0 );	
}


void BoneData::Save( tinyxml2::XMLPrinter* printer )
{
	printer->OpenElement( "BoneData" );
	for( int i=0; i<EL_MAX_BONES; ++i ) {
		printer->OpenElement( "Bone" );
		printer->PushAttribute( "name", bone[i].name.c_str() );
		printer->PushAttribute( "angleRadians", bone[i].angleRadians );
		printer->PushAttribute( "dy", bone[i].dy );
		printer->PushAttribute( "dz", bone[i].dz );
		printer->CloseElement();
	}
	printer->CloseElement();
}




