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
	grinliz::CDynArray< grinliz::GLString > lines;
};


#endif // X_ANIMATION_PARSER
