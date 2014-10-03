#include "procedural.h"
#include "../grinliz/glvector.h"
#include "../game/gameitem.h"
#include "../xegame/istringconst.h"
#include "../game/team.h"

using namespace grinliz;



HumanGen::HumanGen( bool female, U32 seed, int team, bool electric )
{
	this->female = female;
	this->seed   = seed;
	this->team   = team;
	this->electric = electric;
}


void HumanGen::GetSkinColor( int index0, int index1, float fade, Color4F* color )
{
	static const int NUM_SKIN_COLORS = 4;
	static const Vector2I c[NUM_SKIN_COLORS] = {
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		{ PAL_TANGERINE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_TANGERINE }
	};

	index0 = abs(index0) % NUM_SKIN_COLORS;
	index1 = abs(index1) % NUM_SKIN_COLORS;

	const Game::Palette* palette = Game::GetMainPalette();
		
	*color = Lerp( palette->Get4F( c[index0].x, c[index0].y ),
				   palette->Get4F( c[index1].x, c[index1].y ),
				   fade );
}


void HumanGen::GetGlassesColor( int index0, int index1, float fade, Color4F* color )
{
	static const int NUM_GLASSES_COLORS = 6;
	static const Vector2I c[NUM_GLASSES_COLORS] = {
		{ PAL_BLUE*2, PAL_ZERO },
		{ PAL_BLUE*2, PAL_RED },
		{ PAL_BLUE*2, PAL_GREEN },
		{ PAL_ZERO*2+1, PAL_BLUE },
		{ PAL_RED*2+1, PAL_BLUE },
		{ PAL_RED*2+1, PAL_PURPLE }
	};

	index0 = abs(index0) % NUM_GLASSES_COLORS;
	index1 = abs(index1) % NUM_GLASSES_COLORS; 

	const Game::Palette* palette = Game::GetMainPalette();

	*color = Lerp( palette->Get4F( c[index0].x, c[index0].y ),
				   palette->Get4F( c[index1].x, c[index1].y ),
				   fade );
}


void HumanGen::GetHairColor( int index0, Color4F* color )
{
	static const int N_FEMALE = 9;
	static const int N_MALE   = 7;
	
	static const Vector2I cFemale[N_FEMALE] = {
		// natural
		{ PAL_GREEN*2,		PAL_RED },
		{ PAL_RED*2+0,		PAL_TANGERINE },
		{ PAL_PURPLE*2,		PAL_TANGERINE },
		{ PAL_RED*2+1,		PAL_GREEN },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		// intense
		{ PAL_RED*2+1,		PAL_RED },
		{ PAL_ZERO*2+1,		PAL_TANGERINE },
		{ PAL_ZERO*2+1,		PAL_BLUE },
		{ PAL_GRAY*2,		PAL_GRAY }
	};

	static const Vector2I cMale[N_MALE] = {
		// natural
		{ PAL_GREEN*2, PAL_RED },
		{ PAL_RED*2+0, PAL_TANGERINE },
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_RED*2+1, PAL_GREEN },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		// intense
		{ PAL_ZERO*2+1, PAL_TANGERINE },
		{ PAL_GRAY*2,   PAL_GRAY }
	};

	const Game::Palette* palette = Game::GetMainPalette();
	if ( female ) {
		index0 = abs(index0) % N_FEMALE;
		*color = palette->Get4F( cFemale[index0].x, cFemale[index0].y );
	}
	else {
		index0 = abs(index0) % N_MALE;
		*color = palette->Get4F( cMale[index0].x, cMale[index0].y );
	}
}


void HumanGen::GetSuitColor( grinliz::Vector4F* c )
{
	Random random( seed );
	int y = random.Rand( PAL_COUNT );
	int x0 = y + random.Rand( PAL_COUNT - y );
	int x1 = y + random.Rand( PAL_COUNT - y );
	float fraction = random.Uniform();

	const Game::Palette* palette = Game::GetMainPalette();
	Vector4F c0 = palette->Get4F( x0*2, y );
	Vector4F c1 = palette->Get4F( x1*2, y );

	*c = Lerp( c0, c1, fraction );
	c->w = 0.7f;
}


void HumanGen::GetColors( grinliz::Vector4F* c )
{
	Random random( seed );
	random.Rand();

	GetSkinColor(	random.Rand(), 
					random.Rand(),
					random.Uniform(), 
					c+SKIN );
	GetHairColor( random.Rand(), c+HAIR );
	GetGlassesColor( random.Rand(),
					 random.Rand(),
					 random.Uniform(),
					 c+GLASSES );

	if ( electric ) {
		c[SKIN] = c[SKIN] + c[HAIR]*0.5f + c[GLASSES]*0.5f;
		c[HAIR] = c[HAIR] + c[GLASSES]*0.5f;
	}
}


void HumanGen::AssignFace( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Vector4F vcol[3];
	GetColors( vcol );

	// Get the texture to get the the metadata
	Texture* texture = 0;
	if ( female ) {
		texture = TextureManager::Instance()->GetTexture( "humanFemaleFaceAtlas" );
	}
	else {
		texture = TextureManager::Instance()->GetTexture( "humanMaleFaceAtlas" );
	}
	GLASSERT( texture );

	info->texture = texture;
	texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );

	info->color.SetCol( 0, vcol[0] );
	info->color.SetCol( 1, vcol[1] );
	info->color.SetCol( 2, vcol[2] );

	// Alpha information solely from the alpha channel.
	info->color.m41 = 0;
	info->color.m42 = 0;
	info->color.m43 = 0;
	info->color.m44 = 1;
}


void HumanGen::AssignSuit( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Vector4F vcol[3];
	GetColors( vcol );
	GetSuitColor( &vcol[2] );

	// Get the texture to get the the metadata
	Texture* texture = 0;
	if ( female ) {
		texture = TextureManager::Instance()->GetTexture( "femsuit0" );
	}
	else {
		texture = TextureManager::Instance()->GetTexture( "suit0" );
	}
	GLASSERT( texture );

	info->texture = texture;
	//texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );

	vcol[2].w = 0.7f;	// suit glow
	info->color.SetCol( 0, vcol[0] );
	info->color.SetCol( 1, vcol[1] );
	info->color.SetCol( 2, vcol[2] );

	// alpha interpreted as emissive; comes from color2
	info->color.m41 = 0;
	info->color.m42 = 0;
	info->color.m43 = 0;
	info->color.m44 = 1;
}


WeaponGen::WeaponGen(U32 _seed, int _team, int _effectFlags, int _features) : seed(_seed), team(_team), effectFlags(_effectFlags), features(_features)
{
}


Color4F WeaponGen::GetEffectColor(int flags)
{
	Vector2I effect = { 0, 0 };
	float alpha = 0.0;

	switch ( flags & (GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK)) {
	case 0:													effect.Set( 1, PAL_GREEN ); 					break;
	case GameItem::EFFECT_FIRE:								effect.Set( 1, PAL_RED );		alpha=0.7f;		break;
	case GameItem::EFFECT_SHOCK:							effect.Set( 1, PAL_BLUE );		alpha=1.0f;		break;
	case GameItem::EFFECT_FIRE | GameItem::EFFECT_SHOCK:	effect.Set( PAL_PURPLE*2+1, PAL_GRAY );	alpha=0.7f;		break;
	default:
		GLASSERT( 0 );
		break;
	}

	const Game::Palette* palette = Game::GetMainPalette();
	Color4F effectColor = palette->Get4F( effect.x, effect.y );
	return effectColor;
}


void WeaponGen::GetColors( int flags, grinliz::Color4F* array )
{
	// red for fire
	// green for shock/corruption? 
	static const Vector2I c[NUM_COLORS*3] = {
		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2, PAL_ZERO },		{ PAL_GREEN*2, PAL_RED },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_BLUE },	{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_GREEN*2+1, PAL_PURPLE },	{ PAL_GREEN*2, PAL_GRAY },
		{ PAL_BLUE*2, PAL_RED },		{ PAL_BLUE*2, PAL_RED },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_PURPLE*2, PAL_RED },		{ PAL_BLUE*2, PAL_RED },		{ PAL_PURPLE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_PURPLE },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_BLUE },		{ PAL_BLUE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_GRAY },		{ PAL_GRAY*2, PAL_TANGERINE },	{ PAL_BLUE*2+1, PAL_GRAY },
	};

	int i = seed % NUM_COLORS;

	Color4F effectColor = GetEffectColor(flags);
	float alpha = effectColor.a();
	const Game::Palette* palette = Game::GetMainPalette();

	// Add in explosive as a modifying color to the main 2.
	// At the time of this code being written, I don't
	// expect exlposive to be very common.
	if ( flags & GameItem::EFFECT_EXPLOSIVE ) {
		Color4F ec = palette->Get4F( PAL_TANGERINE*2, 0 );
		for( int i=0; i<3; ++i )
			effectColor.X(i) = Mean( effectColor.X(i), ec.X(i) );
		alpha = Max( 0.7f, alpha );
	}
	effectColor.a() = alpha;

	Color4F base		= palette->Get4F( c[i*3+0].x, c[i*3+0].y );
	array[BASE]			= base;
	Color4F contrast	= palette->Get4F( c[i*3+1].x, c[i*3+1].y );
	static const float LERP = 0.65f;
	array[CONTRAST].Set(	Lerp( base.r(), contrast.r(), LERP ),
							Lerp( base.g(), contrast.g(), LERP ),
							Lerp( base.b(), contrast.b(), LERP ),
							Lerp( base.a(), contrast.a(), LERP ) );
	
	// Handle special cases.
	if (Team::Group(team) == TEAM_TROLL) {
		static const Vector2I TROLL_CONTRAST[] = {
			{ 2, 3 }, { 2, 2 }, { 4, 2 }, { 10, 0 }, { 10, 2 }, { 10, 5 }
		};
		static const int NUM_TROLL_CONTRAST = GL_C_ARRAY_SIZE(TROLL_CONTRAST);
		int index = seed % NUM_TROLL_CONTRAST;

		array[BASE] = palette->Get4F(PAL_GRAY * 2, PAL_GREEN);
		array[CONTRAST] = palette->Get4F(TROLL_CONTRAST[index]);
	}
	else if (Team::Group(team) == TEAM_GOB) {
		base = palette->Get4F(PAL_GRAY * 2, PAL_PURPLE);
		contrast = palette->Get4F(PAL_RED * 2, PAL_PURPLE);
	}

	array[EFFECT]		= effectColor;
	array[BASE].a()		= 0;
	array[CONTRAST].a()	= 0;
	array[EFFECT].a()	= alpha;
}


void WeaponGen::AssignRing( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Color4F c[3];
	GetColors(effectFlags, c ); 

	for( int i=0; i<3; ++i ) {
		info->color.SetCol( i, c[i] );
	}
	info->color.m44 = 0;

	Texture* texture = TextureManager::Instance()->GetTexture( "ringAtlas" );
	GLASSERT( texture );

	info->texture = texture;
	texture->GetTableEntry( random.Rand( texture->NumTableEntries() ), &info->te );
	
	info->filter[0]  = true;
	info->filter[1] = (features & RING_GUARD) != 0;
	info->filter[2] = (features & RING_TRIAD) != 0;
	info->filter[3] = (features & RING_BLADE) != 0;

	static const char* procNames[4] = {
		"main",
		"guard",
		"triad",
		"blade"
	};

	for( int i=0; i<4; ++i ) {
		info->filterName[i] = StringPool::Intern( procNames[i] );
	}
}


void WeaponGen::AssignShield( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Color4F c[3];
	Vector4F v[3] = { V4F_ZERO, V4F_ZERO, V4F_ZERO };
	GetColors( effectFlags, c ); 

	for( int i=0; i<3; ++i ) {
		v[i] = c[i];
	}

	v[CONTRAST].w = Max( v[CONTRAST].w, 0.7f );

	// Red: outer
	info->color.SetCol( 0, v[BASE] );
	// Blue: ring
	info->color.SetCol( 2, v[CONTRAST] );
	// Effect: inner
	info->color.SetCol( 1, v[EFFECT] );
	info->color.m44 = 0;

	Texture* texture = TextureManager::Instance()->GetTexture( "structure" );
	GLASSERT( texture );

	info->texture = texture;	
}


void WeaponGen::AssignGun( ProcRenderInfo* info )
{
	Random random( seed );
	random.Rand();

	Color4F c[3];
	GetColors(	effectFlags, c ); 

	for( int i=0; i<3; ++i ) {
		info->color.SetCol( i, c[i] );
	}
	info->color.m44 = 0;

	Texture* texture = TextureManager::Instance()->GetTexture( "structure" );
	GLASSERT( texture );

	info->texture = texture;	
	info->filter[0] = true;
	info->filter[1] = (features & GUN_CELL)   != 0;
	info->filter[2] = (features & GUN_DRIVER) != 0;
	info->filter[3] = (features & GUN_SCOPE)  != 0;

	static const char* GUN_PARTS[4] = { "body", "cell", "driver", "scope" };

	for( int i=0; i<4; ++i ) {
		info->filterName[i] = StringPool::Intern( GUN_PARTS[i] );
	}
}


void TeamGen::Assign(U32 seed, int team, ProcRenderInfo* info)
{
	static const Vector2I NEUTRAL_COLORS[] = {
		{ 0, PAL_GRAY }, { 0, PAL_GRAY }, { 0, PAL_GRAY }
	};

	static const Vector2I TROLL_COLORS[] = {
		{ PAL_GRAY * 2, PAL_GREEN }, { PAL_RED * 2, PAL_GREEN }, { PAL_TANGERINE * 2, PAL_GREEN }
	};

	static const Vector2I GOB_COLORS[] = {
		{ PAL_GRAY * 2, PAL_PURPLE }, { PAL_RED * 2, PAL_PURPLE }, { PAL_GRAY * 2, PAL_PURPLE }
	};

	static const Vector2I HUMAN_COLORS[] = {
		{ 8, 3 }, { 6, 3 }, { 8, 3 },
		{8, 3}, {12, 5}, {8, 3},
		{10, 3}, {7, 4}, {8, 3},
		{10, 3}, {6, 1}, {10, 3},
		{10, 3}, {4, 2}, {3, 2},
		{6, 1}, {8, 3}, {6, 1},
		{8, 1}, {6, 0}, {7, 4},
		{8, 1}, {12, 5}, {8, 2},
		{10, 1}, {6, 4}, {10, 1},
		{6, 2}, {8, 2}, {8, 2},
		{6, 2}, {10, 2}, {6, 2},
		{8, 2}, {10, 3}, {10, 3},
		{10, 2}, {10, 3}, {10, 3},
		{6, 3}, {6, 1}, {6, 1},
		{10, 3}, {3, 4}, {3, 4},
		{10, 3}, {1, 3}, {10, 3}
	};

	const Vector2I* colors = 0;
	int nColors = 0;
	int teamGroup = 0;
	int teamID = 0;

	Team::SplitID(team, &teamGroup, &teamID);

	switch (teamGroup) {
		case TEAM_HOUSE:	
		GLASSERT(teamID);	// how can a neutral ID be building?
		colors = HUMAN_COLORS;
		nColors = GL_C_ARRAY_SIZE(HUMAN_COLORS) / 3;
		break;

		case TEAM_TROLL:
		colors = TROLL_COLORS;
		nColors = GL_C_ARRAY_SIZE(TROLL_COLORS) / 3;
		break;

		case TEAM_GOB:
		GLASSERT(teamID);	// how can a neutral ID be building?
		colors = GOB_COLORS;
		nColors = GL_C_ARRAY_SIZE(GOB_COLORS) / 3;
		break;

		case TEAM_NEUTRAL:
		colors = NEUTRAL_COLORS;
		nColors = GL_C_ARRAY_SIZE(NEUTRAL_COLORS) / 3;
		break;

		default:
		GLASSERT(0);	// team not implemented?
	}

	info->texture = TextureManager::Instance()->GetTexture( "structure" );

	int index = teamID % nColors;
	const Game::Palette* palette = Game::GetMainPalette();

	Vector4F base		= palette->Get4F(colors[index * 3 + 0].x, colors[index * 3 + 0].y);
	Vector4F contrast	= palette->Get4F(colors[index * 3 + 1].x, colors[index * 3 + 1].y);
	Vector4F glow		= palette->Get4F(colors[index * 3 + 2].x, colors[index * 3 + 2].y);

	base.w		= 0;
	contrast.w	= 0;
	glow.w		= 0.7f;

	info->color.SetCol( 0, base );
	info->color.SetCol( 1, contrast );
	info->color.SetCol( 2, glow );
	info->color.m44 = 0;
}


void AssignProcedural(const GameItem* item, ProcRenderInfo* info)
{
	if (!item) return;
	IString proc = item->keyValues.GetIString("procedural");
	if (proc.empty()) return;

	const char* name = item->Name();
	bool female = (item->IName() == ISC::humanFemale);
	U32 seed = item->ID();
	int team = item->team;
	bool electric = false;	// not using; FIXME revisit 'electric'
	int effects = item->Effects();
	int features = 0;
	item->keyValues.Get(ISC::features, &features);

	AssignProcedural(name, female, seed, team, electric, effects, features, info);
}


void AssignProcedural( const char* name,
					   bool female, U32 seed, int team, bool electric, int effectFlags, int features,
					   ProcRenderInfo* info )
{
	if ( !name || !*name )
		return;

	if ( StrEqual( name, "team" )) {
		TeamGen gen;
		gen.Assign( seed, team, info );
	}
	else if ( StrEqual( name, "suit" )) {
		HumanGen gen( female, seed, team, electric );
		gen.AssignSuit( info );
	}
	else if ( StrEqual( name, "ring" )) {
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignRing( info );
	}
	else if ( StrEqual( name, "gun" )
	          || StrEqual( name, "pistol" )
			  || StrEqual( name, "blaster" )
			  || StrEqual( name, "pulse" )
			  || StrEqual( name, "beamgun" ))
	{
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignGun( info );
	}
	else if ( StrEqual( name, "shield" )) {
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignShield( info );
	}
	else {
		GLASSERT( 0 );
	}
}

