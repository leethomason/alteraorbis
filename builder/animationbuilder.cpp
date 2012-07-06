#include "animationbuilder.h"
#include "builder.h"
#include "../tinyxml2/tinyxml2.h"
#include "../grinliz/glmatrix.h"

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

			for( const XMLElement* spriteEle = frameEle->FirstChildElement( "sprite" );
				 spriteEle;
				 spriteEle = spriteEle->NextSiblingElement( "sprite" ) )
			{
				GLString boneName = GetBoneName( spriteEle );
				gamedb::WItem* bone = frame->CreateChild( boneName.c_str() );
				
				float x=0, y=0, angle=0, anglePrime=0;
				float dy=0, dz=0;

				spriteEle->FirstChildElement( "angle" )->QueryFloatText( &angle );
				spriteEle->FirstChildElement( "x"     )->QueryFloatText( &x );
				spriteEle->FirstChildElement( "y"     )->QueryFloatText( &y );

				x /= pur;
				y /= pur;

				// Spriter transformation is written as the position of the bitmap, relative
				// to the model origin, followed by a rotation. (Origin of the bitmap is 
				// the upper left in this version.)
				//
				// Lumos needs the transformation (delta) from the origin.
				// To translate, we need the reference (untransformed) frame.

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

					float rx=0, ry=0;
					refSpriteEle->FirstChildElement( "x" )->QueryFloatText( &rx );
					refSpriteEle->FirstChildElement( "y" )->QueryFloatText( &ry );
					rx /= pur;
					ry /= pur;

					// Deltas, convert to Xenoengine coordinates.
					dz =  (rx - x) / pur;
					dy = -(ry - y) / pur;

					// Part to origin. Rotate. Back to pos. Apply delta.
					Matrix4 toOrigin, toPos, rot, delta;
					toOrigin.SetTranslation( 0, ry, -rx );
					rot.SetXRotation( -angle );
					toPos.SetTranslation( 0, -ry + dy, rx + dz );

					Matrix4 m = delta * toPos * rot * toOrigin;
					m.Dump( "Bone" );
					anglePrime = m.CalcRotationAroundAxis( 0 );
					//GLOUTPUT(( "anglePrime=%f\n", anglePrime ));
				}

				bone->SetFloat( "angle", anglePrime );
				//bone->SetFloat( "x", x );
				//bone->SetFloat( "y", y );
				bone->SetFloat( "dz", dz );
				bone->SetFloat( "dy", dy );
			}
			return frameEle;
		}
	}
	return 0;
}


//	animations
//    "humanFemale"
//		"walk"
//			0	[time=0]								Milliseconds
//				arm.left.upper [y=0.01 z=0.02 r=73]		Translation then rotation. Unit coordinates. Degrees.
//														YZ plane. Origin lower left.
//				arm.left.lower [...]
//				...
//			1	[time=400]

void ProcessAnimation( const tinyxml2::XMLElement* element, gamedb::WItem* witem )
{
	GLString pathName, assetName, extension;
	ParseNames( element, &assetName, &pathName, &extension );

	printf( "Animation path='%s' name='%s'\n", pathName.c_str(), assetName.c_str() );

	FILE* fp = fopen( pathName.c_str(), "r" );
	if ( !fp ) {
		ExitError( "Animation", pathName.c_str(), assetName.c_str(), "could not load file" );
	}
	XMLDocument doc;
	doc.LoadFile( fp );
	printf( "ErrorID=%d\n", doc.ErrorID() );

	gamedb::WItem* root = witem->CreateChild( assetName.c_str() );	// "humanFemale" 

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
				//printf( "  animation: %s\n", name );
		
				gamedb::WItem* anim = root->CreateChild( animName );	// "walk"
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

	fclose( fp );
}

