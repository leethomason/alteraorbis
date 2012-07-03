#include "animationbuilder.h"
#include "builder.h"
#include "../tinyxml2/tinyxml2.h"

using namespace grinliz;
using namespace tinyxml2;

void InsertFrame(	gamedb::WItem* frame, 
					const tinyxml2::XMLElement* reference, 
					const tinyxml2::XMLElement* frameEle, 
					const char* frameName )
{
	// Find the frame.
	for ( ; frameEle; frameEle=frameEle->NextSiblingElement( "frame" ) ) {
		const char* name = frameEle->FirstChildElement( "name" )->GetText();
		if ( StrEqual( name, frameName ) ) {
			for( const XMLElement* spriteEle = frameEle->FirstChildElement( "sprite" );
				 spriteEle;
				 spriteEle = spriteEle->NextSiblingElement( "sprite" ) )
			{
				GLString boneName = spriteEle->FirstChildElement( "image" )->GetText();
				boneName = boneName.substr( 7, 1000 );					// remove "asset/"
				boneName = boneName.substr( 0, boneName.size()-4 );	// remove ".png"

				gamedb::WItem* bone = frame->CreateChild( boneName.c_str() );
				
				float x=0, y=0, angle=0;
				float dx=0, dy=0;

				spriteEle->FirstChildElement( "angle" )->QueryFloatText( &angle );
				spriteEle->FirstChildElement( "x"     )->QueryFloatText( &x );
				spriteEle->FirstChildElement( "y"     )->QueryFloatText( &y );

				if ( reference ) {
					const XMLElement* refSpriteEle = 0;
					for( refSpriteEle = reference->FirstChildElement( "sprite" );
						 refSpriteEle;
						 refSpriteEle = refSpriteEle->NextSiblingElement( "sprite" ))
					{
						if ( StrEqual( refSpriteEle->FirstChildElement( "image" )->GetText(), boneName.c_str() ) ) {
							break;
						}
					}
					GLASSERT( refSpriteEle );

					float rx=0, ry=0;
					refSpriteEle->FirstChildElement( "x" )->QueryFloatText( &rx );
					refSpriteEle->FirstChildElement( "y" )->QueryFloatText( &ry );

					dx = rx - x;
					dy = ry - y;
				}
				// Spriter transformation is written as the position of the bitmap, relative
				// to the model origin, followed by a rotation. (Origin of the bitmap is 
				// the upper left in this version.)
				//
				// Lumos needs the transformation (delta) from the origin.
				// To translate, we need the reference (untransformed) frame.

				bone->SetFloat( "angle", angle );

				// FIXME: x and y need to be deltas from the reference.
				// FIXME: x and y need to be normalized to the Pixel-Unit ratio
				bone->SetFloat( "x", x );
				bone->SetFloat( "y", y );
				bone->SetFloat( "dx", dx );
				bone->SetFloat( "dy", dy );
			}
		}
	}
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

				for( const XMLElement* frameEle = animEle->FirstChildElement( "frame" );
					 frameEle;
					 frameEle = frameEle->NextSiblingElement( "frame" ) )
				{
					gamedb::WItem* frame = anim->CreateChild( frameCount++ );
					float duration = 100.0f;
					XMLUtil::ToFloat( frameEle->FirstChildElement( "duration" )->GetText(), &duration );

					duration *= 10.0f;	// Totally mystery. Seems to be in 1/100th seconds. Likely Spriter bug.
					frame->SetFloat( "duration", duration );

					const char* frameName = frameEle->FirstChildElement( "name" )->GetText();

					InsertFrame( frame, 
								 reference,
								 docH.FirstChildElement( "spriterdata" ).FirstChildElement( "frame" ).ToElement(), 
								 frameName );

					if ( StrEqual( animName, "reference" ) ) {
						reference = frameEle;
					}
				}
			}
		}
	}

	fclose( fp );
}

