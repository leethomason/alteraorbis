#include "worldscript.h"

#include "../xegame/spatialcomponent.h"
#include "../xegame/rendercomponent.h"

#include "../engine/engine.h"
#include "../engine/loosequadtree.h"

using namespace grinliz;


// The complexity is because multiple models are
// used by one render component. The "main" model
// and its hardpooints.
Chit* WorldScript::IsTopLevel( Model* model )
{
	if ( model && model->userData ) {
		Chit* chit = model->userData;
		RenderComponent* rc = chit->GetRenderComponent();
		if ( rc ) {
			if ( rc->MainModel() == model ) {
				return chit;
			}
		}
	}
	return 0;
}


void WorldScript::QueryChits( const Rectangle2F& rect, Engine* engine, grinliz::CDynArray<Chit*> *chitArr )
{
	chitArr->Clear();
	Model* root = engine->GetSpaceTree()->QueryRect( rect, 0, 0 );
	for( ; root; root=root->next ) {
		Chit* chit = IsTopLevel( root );
		if ( chit ) {
			chitArr->Push( chit );
		}
	}
}



