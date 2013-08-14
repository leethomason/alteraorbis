#ifndef X_ANIMATION_PARSER
#define X_ANIMATION_PARSER

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"

#include "../tinyxml2/tinyxml2.h"
#include "../shared/gamedbwriter.h"
#include "../engine/enginelimits.h"

/*	Parses the ultra-simple and readable BVH file.
	There is an issue with position vs. reference position handling. 
		See AnimationResource::GetTransform
	
	This parses files in XYZ order. Other orders should be fine,
	but some code is needed.

	Input is expected in Blender coordinates. (Z-up) Swizzle
	changes to GL coordinats.
*/
class XAnimationParser
{
public:
	XAnimationParser();
	~XAnimationParser();

	void ParseBVH( const char* filename, gamedb::WItem* witem );

private:
	// Tree Structure
	struct BNode {
		BNode() : parent(0)	{
			refPos.Zero();
			memset( channel, 0, sizeof(float)*EL_MAX_ANIM_FRAMES*6 );
		}

		BNode*				parent;
		grinliz::GLString	name;	// torso

		grinliz::CDynArray< BNode*, grinliz::OwnedPtrSem > 
							childArr;	// use semantics to call delete on child nodes

		grinliz::Vector3F	refPos;		// the position of the bone
		float				channel[EL_MAX_ANIM_FRAMES][6];	// 0-2 pos xyz, 3-5 rot xyz
	};


	struct Channel {
		BNode*	node;
		int		select;
	};

	bool GetLine( FILE* fp, grinliz::GLString* str );
	const char* SkipWhiteSpace( const char* p ) { while( p && *p && isspace(*p)) ++p; return p; }
	const char* SkipNumDelimiter( const char* p ) 
	{ 
		while(    p 
			   && *p 
			   && (isspace(*p) || (*p==',') || (*p==';') ))
		{
			++p;
		}
		return p; 
	}

	const char* ParseJoint( const char* p, BNode* node );
	const char* ParseMotion( const char* p );

	bool IsIdent( char p ) {
		return isalnum(p) || p == '_' || p == '.';
	}
	bool IsNum( char p ) {
		return isdigit(p) || p == '-' || p == '.';
	}

	grinliz::GLString GetAnimationType( const char* filename );
	void ReadFile( const char* filename );	// reads in the text and normalizes the lines

	const char* ScanFloat( float* v, const char* p );
	
	void DumpBNode( BNode* node, int depth );

	void WriteBVH( const grinliz::GLString& type, gamedb::WItem* witem );
	void WriteBVHRec( gamedb::WItem* witem, int n, BNode* node );

	int   nFrames;
	float frameTime;
	int   frameSkip;
	BNode* bNodeRoot;

	grinliz::CDynArray< Channel > channelArr;
	grinliz::CDynArray< grinliz::GLString > lines;
	grinliz::GLString str;
};


#endif // X_ANIMATION_PARSER
