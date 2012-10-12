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
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glmatrix.h"
#include "../engine/enginelimits.h"

using namespace grinliz;
using namespace tinyxml2;


void SCMLParser::ReadPartNames( const tinyxml2::XMLDocument* doc )
{
	XMLConstHandle h( doc );
	const XMLElement* folderEle = h.FirstChildElement( "spriter_data" ).FirstChildElement( "folder" ).ToElement();
	GLASSERT( folderEle );

	for( const XMLElement* fileEle=folderEle->FirstChildElement( "file" ); fileEle; fileEle=fileEle->NextSiblingElement( "file" ) )
	{
		int id=0;
		fileEle->QueryIntAttribute( "id", &id );
		const char* fullName = fileEle->Attribute( "name" );
		GLASSERT( fullName );
		const char* name = strstr( fullName, "/" );
		GLASSERT( name );
		name++;
		char* end = (char*)strrchr( name, '.' );
		GLASSERT( end );
		*end = 0;

		GLString str( name );
		//GLOUTPUT(( "adding [%d]=%s\n", id, str.c_str() ));
		partIDNameMap.Add( id, str );
	}
}


void SCMLParser::ReadAnimations( const tinyxml2::XMLDocument* doc )
{
	XMLConstHandle h( doc );
	const XMLElement* entityEle = h.FirstChildElement( "spriter_data" ).FirstChildElement( "entity" ).ToElement();
	GLASSERT( entityEle );

	for( const XMLElement* animationEle=entityEle->FirstChildElement( "animation" ); animationEle; animationEle=animationEle->NextSiblingElement( "animation" ) )
	{
		Animation a;
		a.name = animationEle->Attribute( "name" );

		animationEle->QueryUnsignedAttribute( "length", &a.length );
		animationEle->QueryIntAttribute( "id", &a.id );
		//GLOUTPUT(( "Animation: %s len=%d id=%d\n", a.name.c_str(), a.length, a.id ));
		a.nFrames = 0;
		const XMLElement* mainlineEle = animationEle->FirstChildElement( "mainline" );
		GLASSERT( mainlineEle );
		for( const XMLElement* keyEle=mainlineEle->FirstChildElement( "key" ); keyEle; keyEle=keyEle->NextSiblingElement( "key" ))
		{
			a.nFrames++;
		}
		animationArr.Push( a );
	}
}


const XMLElement* SCMLParser::GetAnimationEle( const XMLDocument* doc, const GLString& name )
{
	XMLConstHandle h( doc );
	const XMLElement* entityEle = h.FirstChildElement( "spriter_data" ).FirstChildElement( "entity" ).ToElement();
	GLASSERT( entityEle );

	for( const XMLElement* animEle=entityEle->FirstChildElement( "animation" ); animEle; animEle=animEle->NextSiblingElement( "animation" ))
	{
		if ( name == animEle->Attribute( "name" ))
			return animEle;
	}
	return 0;
}


const tinyxml2::XMLElement* SCMLParser::GetObjectEle(	const tinyxml2::XMLElement* animationEle,
														int timelineID,
														int keyID )
{
	for( const XMLElement* timelineEle = animationEle->FirstChildElement( "timeline" );
		 timelineEle;
		 timelineEle = timelineEle->NextSiblingElement( "timeline" ) )
	{
		int id=-1;
		timelineEle->QueryIntAttribute( "id", &id );
		if ( id == timelineID ) {
			for( const XMLElement* keyEle = timelineEle->FirstChildElement( "key" );
				 keyEle;
				 keyEle = keyEle->NextSiblingElement( "key" ) )
			{
				int k=-1;
				keyEle->QueryIntAttribute( "id", &k );
				if ( keyID == k ) {
					const XMLElement* objectEle = keyEle->FirstChildElement( "object" );
					return objectEle;
				}
			}
		}
	}
	return 0;
}


void SCMLParser::ReadAnimation( const XMLDocument* doc, Animation* a )
{
	XMLConstHandle h( doc );

	const XMLElement* animationEle = GetAnimationEle( doc, a->name );
	GLASSERT( animationEle );
	const XMLElement* mainlineEle = animationEle->FirstChildElement( "mainline" );
	GLASSERT( mainlineEle );

	/*
		This is a little weird.

            <timeline id="5">
                <key id="0">
                    <object folder="0" file="1" x="-172" y="162" a="0.5"/>
                </key>
		
		timeline.id -> ??
		timeline::key.id -> frame
		timeline::key::object.file -> partID

		At some point I'll get docs and this will make sense.
	*/

	for( const XMLElement* timelineEle = animationEle->FirstChildElement( "timeline" );
		 timelineEle;
		 timelineEle = timelineEle->NextSiblingElement( "timeline" ))
	{
		for( const XMLElement* keyEle=timelineEle->FirstChildElement( "key" );
			 keyEle;
			 keyEle = keyEle->NextSiblingElement( "key" ) )
		{
			// Start time and frame number, makes sense. (What's with the 'id' thing??)
			int frameNumber = 0;
			int startTime=0;
			keyEle->QueryIntAttribute( "id", &frameNumber );
			keyEle->QueryIntAttribute( "time", &startTime );

			a->frames[frameNumber].time = startTime;

			const XMLElement* objectEle = keyEle->FirstChildElement( "object" );
			GLASSERT( objectEle );	// can there be more than one? Less? No docs. Guessing.

			int partID = 0;
			float x=0;
			float y=0;
			float angle=0;

			objectEle->QueryIntAttribute( "file", &partID );
			objectEle->QueryFloatAttribute( "x", &x );
			objectEle->QueryFloatAttribute( "y", &y );
			objectEle->QueryFloatAttribute( "angle", &angle );

			a->frames[frameNumber].xforms[partID].id    = partID;
			a->frames[frameNumber].xforms[partID].angle = angle;
			a->frames[frameNumber].xforms[partID].x	 = x;
			a->frames[frameNumber].xforms[partID].y	 = y;
		}
	}

}


void SCMLParser::WriteAnimation( gamedb::WItem* witem, const Animation& a, const Animation* reference, float pur )
{
	gamedb::WItem* animationItem = witem->CreateChild( a.name.c_str() );
	animationItem->SetInt( "totalDuration", a.length );

	for( int i=0; i<a.nFrames; ++i ) {
		const Frame& f = a.frames[i];
		gamedb::WItem* frameItem = animationItem->CreateChild( i );
		frameItem->SetInt( "time", f.time );

		for( int k=0; k<EL_MAX_METADATA; ++k ) {
			if ( f.meta[k].size() > 0 ) {
				GLString str;
				str.Format( "event%d", k );
				frameItem->SetString( str.c_str(), f.meta[k].c_str() );
			}
		}

		for( int k=0; k<EL_MAX_BONES; ++k ) {
			GLString name;

			if ( partIDNameMap.Query( k, &name ) ) {
				GLASSERT( k == f.xforms[k].id );	
				const PartXForm& xform = f.xforms[k];
				gamedb::WItem* bone = frameItem->CreateChild( name.c_str() );

				float anglePrime = 0;
				float dy = 0;
				float dz = 0;
				
				float angle = NormalizeAngleDegrees( xform.angle );
				float x = xform.x / pur;
				float y = xform.y / pur;

				// Spriter transformation is written as the position of the bitmap, relative
				// to the model origin, followed by a rotation. (Origin of the bitmap is 
				// the upper left in this version.)
				//
				// Lumos needs the transformation (delta) from the origin.
				// To translate, we need the reference (untransformed) frame.
				Matrix4 m;
				float rx=0, ry=0;

				if ( reference ) {
					rx = reference->frames[0].xforms[k].x / pur;
					ry = reference->frames[0].xforms[k].y / pur;

					// Part to origin. Rotate. Back to pos. Apply delta.
					Matrix4 toOrigin, toPos, rot;

					toOrigin.SetTranslation( 0, ry, -rx );	// negative direction, remembering: 1) y is flipped, 2) x maps to z
					rot.SetXRotation( -angle );				// rotate, convert to YZ right hand rule
					toPos.SetTranslation( 0, -y, x );		// positive direction

					//m = toPos * rot * toOrigin;
					m = toOrigin * rot * toPos;
					anglePrime = m.CalcRotationAroundAxis( 0 );		// not very meaningful, just used to construct a transformation matrix in the shader
				}

				bone->SetFloat( "anglePrime", anglePrime );
				bone->SetFloat( "dy", m.m24 );
				bone->SetFloat( "dz", m.m34 );
			}
		}
	}
}


void SCMLParser::Parse( const XMLElement* element, const XMLDocument* doc, gamedb::WItem* witem, float pur )
{
	ReadPartNames( doc );
	ReadAnimations( doc );

	for( int i=0; i<animationArr.Size(); ++i ) {
		ReadAnimation( doc, &animationArr[i] );
	}

	GLOUTPUT(( "Animation\n" ));
	for( int i=0; i<animationArr.Size(); ++i ) {
		if ( animationArr[i].name == "walk" ) {
			GLOUTPUT(( "  Animation name=%s nFrames=%d duration=%d\n",
					   animationArr[i].name.c_str(),
					   animationArr[i].nFrames,
					   animationArr[i].length ));
			for( int j=0; j<animationArr[i].nFrames; ++j ) {
				GLOUTPUT(( "    Frame time=%d\n", animationArr[i].frames[j].time ));
				for( int k=0; k<EL_MAX_BONES; ++k ) {
					GLString name;
					if ( partIDNameMap.Query( k, &name ) ) {
						GLOUTPUT(( "      Bone %d %s angle=%.1f x=%.1f y=%.1f\n",
								  k, name.c_str(), 
								  animationArr[i].frames[j].xforms[k].angle,
								  animationArr[i].frames[j].xforms[k].x,
								  animationArr[i].frames[j].xforms[k].y ));
					}
				}
			}
		}
	}

	const Animation* reference = 0;
	for( int i=0; i<animationArr.Size(); ++i ) {
		if ( animationArr[i].name == "reference" ) {
			reference = &animationArr[i];
			break;
		}
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

		for( int i=0; i<animationArr.Size(); ++i ) {
			if ( animationArr[i].name == animationName ) {
				for( int j=animationArr[i].nFrames-1; j>=0; --j ) {
					if ( time >= animationArr[i].frames[j].time ) {
						for( int k=0; k<EL_MAX_METADATA; ++k ) {
							if ( animationArr[i].frames[j].meta[k].size() == 0 ) {
								animationArr[i].frames[j].meta[k] = metaName;
								goto done;
							}
						}
					}
				}
				goto done;
			}
		}
done:
		continue;
	}

	for( int i=0; i<animationArr.Size(); ++i ) {
		WriteAnimation( witem, animationArr[i], reference == &animationArr[i] ? 0 : reference, pur );
	}
}



/*
  animations []
    humanFemale []
      gunrun [ totalDuration=1200.000000]
        0 [ duration=200.000000]
          arm.lower.left [ angle=27.083555 dy=-0.000076 dz=0.000340]
          arm.lower.right [ angle=268.449707 dy=0.000445 dz=-0.000228]
          arm.upper.left [ angle=28.810850 dy=-0.000000 dz=0.000000]
		  ...
*/

void ProcessAnimation( const tinyxml2::XMLElement* element, gamedb::WItem* witem )
{
	GLString pathName, assetName, extension;
	ParseNames( element, &assetName, &pathName, &extension );

	printf( "Animation path='%s' name='%s'\n", pathName.c_str(), assetName.c_str() );

	// -- Process the SCML file --- //
	XMLDocument doc;
	doc.LoadFile( pathName.c_str() );
	if ( doc.Error() ) {
		ExitError( "Animation", pathName.c_str(), assetName.c_str(), "tinyxml-2 could not parse scml" );
	}
	float pur = 1;
	element->QueryFloatAttribute( "pur", &pur );

	gamedb::WItem* root = witem->CreateChild( assetName.c_str() );	// "humanFemaleAnimation" 

	SCMLParser parser;
	parser.Parse( element, &doc, root, pur );

//	fclose( fp );
}
