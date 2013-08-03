#ifndef X_ANIMATION_PARSER
#define X_ANIMATION_PARSER

#include "../grinliz/gltypes.h"
#include "../grinliz/gldebug.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glstringutil.h"

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

private:
	bool GetLine( FILE* fp, char* buf, int size );
	const char* SkipWhiteSpace( const char* p ) { while( p && *p && isspace(*p)) ++p; return p; }
	const char* ParseDataObject( const char* p, int depth ); 
	bool IsIdent( char p ) {
		return isalnum(p) || p == '_';
	}

	grinliz::CDynArray< grinliz::GLString > lines;
	grinliz::GLString str;
};


#endif // X_ANIMATION_PARSER
