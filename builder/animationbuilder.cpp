#include "animationbuilder.h"
#include "builder.h"
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glmatrix.h"
#include "../engine/enginelimits.h"

using namespace grinliz;
using namespace tinyxml2;

GLString GetBoneName( const XMLElement* spriteEle )
{
	GLString boneName = spriteEle->FirstChildElement( "image" )->GetText();
	boneName = boneName.substr( 7, 1000 );					// remove "asset/"
	boneName = boneName.substr( 0, boneName.size()-4 );		// remove ".png"
	return boneName;
}


const XMLElement* InsertFrame(	gamedb::WItem* frame, 
								const tinyxml2::XMLElement* reference, 
								const tinyxml2::XMLElement* frameEle, 
								const char* frameName,
								float pur )
{
	// Find the frame, in this case the frame description,
	// not the character frame.
	
	for ( ; frameEle; frameEle=frameEle->NextSiblingElement( "frame" ) ) {
		const char* name = frameEle->FirstChildElement( "name" )->GetText();
		if ( StrEqual( name, frameName ) ) {

			int i=0;
			for( const XMLElement* spriteEle = frameEle->FirstChildElement( "sprite" );
				 spriteEle;
				 spriteEle = spriteEle->NextSiblingElement( "sprite" ), ++i )
			{
				GLString boneName = GetBoneName( spriteEle );
				gamedb::WItem* bone = frame->CreateChild( boneName.c_str() );
				
				float x=0, y=0, angle=0, anglePrime=0;

				spriteEle->FirstChildElement( "angle" )->QueryFloatText( &angle );
				spriteEle->FirstChildElement( "x"     )->QueryFloatText( &x );
				spriteEle->FirstChildElement( "y"     )->QueryFloatText( &y );

				angle = NormalizeAngleDegrees( angle );
				x /= pur;
				y /= pur;

				// Spriter transformation is written as the position of the bitmap, relative
				// to the model origin, followed by a rotation. (Origin of the bitmap is 
				// the upper left in this version.)
				//
				// Lumos needs the transformation (delta) from the origin.
				// To translate, we need the reference (untransformed) frame.
				Matrix4 m;
				float rx=0, ry=0;

				if ( reference ) {
					const XMLElement* refSpriteEle = 0;
					for( refSpriteEle = reference->FirstChildElement( "sprite" );
						 refSpriteEle;
						 refSpriteEle = refSpriteEle->NextSiblingElement( "sprite" ))
					{
						GLString refBoneName = GetBoneName( refSpriteEle );
						if ( boneName == refBoneName ) {
							break;
						}
					}
					GLASSERT( refSpriteEle );

					refSpriteEle->FirstChildElement( "x" )->QueryFloatText( &rx );
					refSpriteEle->FirstChildElement( "y" )->QueryFloatText( &ry );
					rx /= pur;
					ry /= pur;

					// Part to origin. Rotate. Back to pos. Apply delta.
					Matrix4 toOrigin, toPos, rot;

					toOrigin.SetTranslation( 0, ry, -rx );			// negative direction, remembering: 1) y is flipped, 2) x maps to z
					rot.SetXRotation( -angle );						// rotate, convert to YZ right hand rule
					toPos.SetTranslation( 0, -y, x );				// positive direction

					m = toPos * rot * toOrigin;
					//m.Dump( "Bone" );
					anglePrime = m.CalcRotationAroundAxis( 0 );		// not very meaningful, just used to construct a transformation matrix in the shader
					//GLOUTPUT(( "anglePrime=%f\n", anglePrime ));

#ifdef DEBUG
					// Check that the matrix can be reconstructed.
					Matrix4 shader;
					float sinTheta = sinf( ToRadian( anglePrime ));
					float cosTheta = cosf( ToRadian( anglePrime ));
					shader.m22 = cosTheta;
					shader.m32 = sinTheta;
					shader.m23 = -sinTheta;
					shader.m33 = cosTheta;
					shader.m24 = m.m24;	// see below
					shader.m34 = m.m34;	// see below

					GLASSERT( Equal( shader, m ));
#endif
				}

				bone->SetFloat( "anglePrime", anglePrime );
				bone->SetFloat( "dy", m.m24 );
				bone->SetFloat( "dz", m.m34 );

				/*
				bone->SetFloat( "angle", NormalizeAngleDegrees( -angle ));
				bone->SetFloat( "z", x );
				bone->SetFloat( "y", -y );
				bone->SetFloat( "rz", rx );
				bone->SetFloat( "ry", -ry );
				*/
			}
			return frameEle;
		}
	}
	return 0;
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
	//FILE* fp = fopen( pathName.c_str(), "r" );
	//if ( !fp ) {
	//	ExitError( "Animation", pathName.c_str(), assetName.c_str(), "could not load file" );
	//}
	XMLDocument doc;
	doc.LoadFile( pathName.c_str() );
	if ( doc.Error() ) {
		ExitError( "Animation", pathName.c_str(), assetName.c_str(), "tinyxml-2 could not parse scml" );
	}

	gamedb::WItem* root = witem->CreateChild( assetName.c_str() );	// "humanFemaleAnimation" 

	const XMLConstHandle docH( doc );
	const XMLElement* reference = 0;
	float pur = 1;
	element->QueryFloatAttribute( "pur", &pur );

	// First pass is to get the reference animation; the 2nd pass parses the various flavors.
	for( int pass=0; pass<2; ++pass ) {

		const XMLElement* animEle = docH.FirstChildElement( "spriterdata" ).FirstChildElement( "char" ).FirstChildElement( "anim" ).ToElement();
		for( ; animEle; animEle=animEle->NextSiblingElement( "anim" ) ) {

			const char* animName = animEle->FirstChildElement( "name" )->GetText();
			if (    ( pass == 0 && StrEqual( animName, "reference" ) )
				 || ( pass == 1 && !StrEqual( animName, "reference" )))
			{
				gamedb::WItem* anim = root->CreateChild( animName );	// "walk"
				anim->FetchChild( "metaData" );		// always write so that NumFrames = NumChildren - 1
				int frameCount = 0;
				float totalDuration = 0;

				for( const XMLElement* frameEle = animEle->FirstChildElement( "frame" );
					 frameEle;
					 frameEle = frameEle->NextSiblingElement( "frame" ) )
				{
					gamedb::WItem* frame = anim->CreateChild( frameCount++ );
					float duration = 100.0f;
					XMLUtil::ToFloat( frameEle->FirstChildElement( "duration" )->GetText(), &duration );

					duration *= 10.0f;	// Totally mystery. Seems to be in 1/100th seconds. Likely Spriter bug.
					frame->SetFloat( "duration", duration );
					totalDuration += duration;

					const char* frameName = frameEle->FirstChildElement( "name" )->GetText();

					const XMLElement* f = InsertFrame(	frame, 
														reference,
														docH.FirstChildElement( "spriterdata" ).FirstChildElement( "frame" ).ToElement(), 
														frameName, 
														pur );
					if ( pass == 0 ) {
						reference = f;
					}
				}
				anim->SetFloat( "totalDuration", totalDuration );
			}
		}
	}

	// -- Process the meta data -- //
	for(	const XMLElement* metaEle = element->FirstChildElement( "meta" );
			metaEle;
			metaEle = metaEle->NextSiblingElement( "meta" ) )
	{
		const char* animationName = metaEle->Attribute( "animation" );
		const char* metaName = metaEle->Attribute( "name" );
		float time = 0;
		metaEle->QueryFloatAttribute( "time", &time );

		gamedb::WItem* animationItem = root->FetchChild( animationName );
		gamedb::WItem* metaDataItem  = animationItem->FetchChild( "metaData" );
		gamedb::WItem* eventItem     = metaDataItem->FetchChild( metaName );
		eventItem->SetFloat( "time", time );
	}

//	fclose( fp );
}
