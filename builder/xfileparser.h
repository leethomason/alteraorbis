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

/* Parses both .x and .bvh files.
   Similar parser, and only the armiture is read from the .x, so
   they produce exactly the same output.
*/
class XAnimationParser
{
public:
	XAnimationParser();
	~XAnimationParser();

	void Parse(    const char* filename, gamedb::WItem* witem );
	void ParseBVH( const char* filename, gamedb::WItem* witem );

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
		grinliz::GLString	reference;
		grinliz::Quaternion rotation[EL_MAX_ANIM_FRAMES];
		grinliz::Vector3F	scale[EL_MAX_ANIM_FRAMES];
		grinliz::Vector3F	pos[EL_MAX_ANIM_FRAMES];
	};

	// Tree Structure
	struct BNode {
		BNode() : parent(0)	{}

		BNode*				parent;
		grinliz::GLString	name;	// torso

		grinliz::CDynArray< BNode*, grinliz::OwnedPtrSem > 
							childArr;	// use semantics to call delete on child nodes

		grinliz::Vector3F	pos;		// the position of the bone
		grinliz::Vector3F	xformTrans;	// translation	
		grinliz::Vector3F	xformRot;	// rotation (euler, 3 component)
	};


	struct Channel {
		BNode*	node;
		int		select;	// 0-2: pos, 3-5: rot
	};

	FrameNode* frameNodeRoot;
	grinliz::CDynArray< AnimationNode*, grinliz::OwnedPtrSem > animationArr;
	BNode* bNodeRoot;
	grinliz::CDynArray< Channel > channelArr;

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

	// Parsers. Note that the X version takes the parent, the BVH version parses the parent.
	const char* ParseDataObject( const char* p, Node* parent ); 
	const char* ParseJoint( const char* p, BNode* node );

	bool IsIdent( char p ) {
		return isalnum(p) || p == '_' || p == '.';
	}
	bool IsNum( char p ) {
		return isdigit(p) || p == '-' || p == '.';
	}

	grinliz::GLString GetAnimationType( const char* filename );
	void ReadFile( const char* filename );	// reads in the text and normalizes the lines

	const char* ScanFloat( float* v, const char* p );
	void WalkFrameNodes( FrameNode* fn, Node* n );
	void ReadAnimation( Node* node, AnimationNode* anim );
	
	void DumpNode( Node* node, int depth );
	void DumpBNode( BNode* node, int depth );
	void DumpFrameNode( FrameNode* node, int depth );
	void DumpAnimation( AnimationNode* );

	void Write( const grinliz::GLString& type, gamedb::WItem* witem );

	Node* root;
	int frameSkip;
	float ticksPerSecond;

	grinliz::CDynArray< grinliz::GLString > lines;
	grinliz::GLString str;
};


#endif // X_ANIMATION_PARSER
