#ifndef X_ANIMATION_PARSER
#define X_ANIMATION_PARSER

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"
#include "../grinliz/glmatrix.h"

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

	struct FrameNode {
		FrameNode() : parent(0)	{}
		~FrameNode();

		FrameNode*			parent;
		grinliz::GLString	ident;	// Frame
		grinliz::GLString	name;	// Armature_torso

		grinliz::CDynArray< FrameNode*, grinliz::OwnedPtrSem > 
							childArr;	// use semantics to call delete on child nodes

		grinliz::Matrix4	xform;
	};

private:
	bool GetLine( FILE* fp, char* buf, int size );
	const char* SkipWhiteSpace( const char* p ) { while( p && *p && isspace(*p)) ++p; return p; }
	const char* ParseDataObject( const char* p, int depth ); 
	bool IsIdent( char p ) {
		return isalnum(p) || p == '_';
	}
	bool IsNum( char p ) {
		return isdigit(p) || p == '-';
	}

	void Push( const grinliz::GLString& ident, const grinliz::GLString& name );
	void Pop(  const grinliz::GLString& ident, const grinliz::GLString& name );
	void Scan( const grinliz::GLString& ident, const char* );
	void ScanFloats( float* arr, int count, const char* p );

	FrameNode* rootFrameNode;
	FrameNode* currentFrameNode;
	int scanDepth;

	grinliz::CDynArray< grinliz::GLString > lines;
	grinliz::GLString str;
};


#endif // X_ANIMATION_PARSER
