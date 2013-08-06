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

class XAnimationParser
{
public:
	XAnimationParser();
	~XAnimationParser();

	void Parse( const char* filename,
				gamedb::WItem* witem );

	struct Node {
		Node* parent;
		grinliz::GLString	ident;
		grinliz::GLString	name;
		grinliz::CDynArray< Node*, grinliz::OwnedPtrSem > childArr;
		grinliz::CDynArray< float >	floatArr;
	};

	// Tree structure.
	struct FrameNode {
		FrameNode() : parent(0)	{}

		FrameNode*			parent;
		grinliz::GLString	ident;	// Frame
		grinliz::GLString	name;	// Armature_torso

		grinliz::CDynArray< FrameNode*, grinliz::OwnedPtrSem > 
							childArr;	// use semantics to call delete on child nodes

		grinliz::Matrix4	xform;
	};

	// Flat
	struct AnimationNode {
		AnimationNode() : nFrames(0) {}

		int nFrames;
		grinliz::GLString	name;
		grinliz::Quaternion rotation[EL_MAX_ANIM_FRAMES];
		grinliz::Vector3F	scale[EL_MAX_ANIM_FRAMES];
		grinliz::Vector3F	pos[EL_MAX_ANIM_FRAMES];
	};

	FrameNode* frameNodeRoot;

private:
	bool GetLine( FILE* fp, char* buf, int size );
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
	const char* ParseDataObject( const char* p, Node* parent ); 

	bool IsIdent( char p ) {
		return isalnum(p) || p == '_';
	}
	bool IsNum( char p ) {
		return isdigit(p) || p == '-' || p == '.';
	}

	const char* ScanFloat( float* v, const char* p );
	void WalkFrameNodes( FrameNode* fn, Node* n );
	
	void DumpNode( Node* node, int depth );
	void DumpFrameNode( FrameNode* node, int depth );

	Node* root;

	grinliz::CDynArray< grinliz::GLString > lines;
	grinliz::GLString str;
};


#endif // X_ANIMATION_PARSER
