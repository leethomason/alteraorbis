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

#include "animationbuilder.h"
#include "builder.h"
#include "xfileparser.h"
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glmatrix.h"
#include "../engine/enginelimits.h"

using namespace grinliz;
using namespace tinyxml2;


void ProcessAnimation( const tinyxml2::XMLElement* element, gamedb::WItem* witem )
{
	GLString pathName, assetName, extension;
	ParseNames( element, &assetName, &pathName, &extension );

	printf( "Animation path='%s' name='%s'\n", pathName.c_str(), assetName.c_str() );
	gamedb::WItem* root = witem->FetchChild( assetName.c_str() );	// "humanFemaleAnimation" 

	if ( extension == ".scml" ) {
		// -- Process the SCML file --- //
		ExitError( "Animation", pathName.c_str(), assetName.c_str(), "SCML is no longer supported." );
	}
	else if ( extension == ".bvh" ) {
		static const char* postfix[] = { "stand", "walk", "gunstand", "gunwalk", "melee", "impact", "" };
		for( int i=0; *postfix[i]; ++i ) {

			// stip off ".bvh"
			GLString fn = pathName.substr( 0, pathName.size()-4 );
			fn.append( postfix[i] );
			fn.append( ".bvh" );

			FILE* fp = fopen(  fn.c_str(), "rb" );
			if ( fp ) {
				fclose( fp );

				XAnimationParser parser;
				parser.ParseBVH( fn.c_str(), root );
			}
		}
	}
	else {
		ExitError( "Animation", pathName.c_str(), assetName.c_str(), "file extension not recognized" );
	}

	
	// -- Process the meta data -- //
	for(	const XMLElement* metaEle = element->FirstChildElement( "meta" );
			metaEle;
			metaEle = metaEle->NextSiblingElement( "meta" ) )
	{
		const char* animationName = metaEle->Attribute( "animation" );
		const char* metaName = metaEle->Attribute( "name" );
		int time = 0;
		metaEle->QueryIntAttribute( "time", &time );

		gamedb::WItem* animationItem = root->FetchChild( animationName );
		animationItem->SetString( "metaData", metaName );
		animationItem->SetInt(    "metaDataTime", time );
	}
}
