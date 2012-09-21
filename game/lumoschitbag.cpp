#include "lumoschitbag.h"
#include "../engine/model.h"
#include "gameitem.h"

void LumosChitBag::HandleBolt( const Bolt& bolt, Model* m, const grinliz::Vector3F& at )
{
	Chit* chit = m->userData;
	GLASSERT( chit );
	GLASSERT( GetChit( chit->ID() ) == chit );
//	GLOUTPUT(( "Chit id=%d hit\n", chit->ID() ));
 
	DamageDesc dd;
	dd.components = bolt.damage;
	chit->SendMessage( ChitMsg( ChitMsg::CHIT_DAMAGE, 0, &dd ), 0 );
}
