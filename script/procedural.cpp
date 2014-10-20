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


void HumanGen::GetSkinColor( int index, Color4F* color )
{
	static const int NUM_SKIN_COLORS = 4;
	static const Vector2I c[NUM_SKIN_COLORS] = {
		{ PAL_PURPLE*2, PAL_TANGERINE },
		{ PAL_TANGERINE*2+1, PAL_PURPLE },
		{ PAL_TANGERINE*2+1, PAL_GRAY },
		{ PAL_GRAY*2, PAL_TANGERINE }
	};
	int i = abs(index) % NUM_SKIN_COLORS;
	const Game::Palette* palette = Game::GetMainPalette();
	*color = palette->Get4F(c[i].x, c[i].y);
}


void HumanGen::GetGlassesColor( int index, Color4F* color )
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

	int i = abs(index) % NUM_GLASSES_COLORS;
	const Game::Palette* palette = Game::GetMainPalette();
	*color = palette->Get4F(c[i].x, c[i].y);
}


void HumanGen::GetHairColor( int index, Color4F* color )
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
		int index0 = abs(index) % N_FEMALE;
		*color = palette->Get4F( cFemale[index0].x, cFemale[index0].y );
	}
	else {
		int index0 = abs(index) % N_MALE;
		*color = palette->Get4F( cMale[index0].x, cMale[index0].y );
	}
}


void HumanGen::GetSuitColor( grinliz::Vector4F* c )
{
	// Suits are at the right column and bottom row
	// of the color palette. Just to be a little fancy,
	// ony use colors that are in the team palette 
	// somewhere.
	Vector2I cvec[3];
	TeamGen::TeamBuildColors(team, cvec + 0, cvec + 1, cvec + 2);

	CArray<Vector2I, 6> arr;
	Vector2I v;
	for (int i = 0; i < 2; ++i) {
		v.Set(PAL_GRAY * 2, cvec[0].y);
		arr.Push(v);
		v.Set(cvec[0].x, PAL_GRAY);
		arr.Push(v);
		v.Set(cvec[0].x + 1, PAL_GRAY);
		arr.Push(v);
	}

	int teamID, teamGroup;
	Team::SplitID(team, &teamGroup, &teamID);
	int index = abs(teamID) % arr.Size();

	const Game::Palette* palette = Game::GetMainPalette();
	*c = palette->Get4F(arr[index]);
	c->w = 0.7f;
}


void HumanGen::GetColors(grinliz::Vector4F* c)
{
	Random random(seed);
	random.Rand();

	GetSkinColor(random.Rand(), c + SKIN);
	GetHairColor(random.Rand(), c + HAIR);
	GetGlassesColor(random.Rand(), c + GLASSES);

	if (electric) {
		c[SKIN] = c[SKIN] + c[HAIR] * 0.5f + c[GLASSES] * 0.5f;
		c[HAIR] = c[HAIR] + c[GLASSES] * 0.5f;
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


void TeamGen::TeamBuildColors(int team, grinliz::Vector2I* base, grinliz::Vector2I* contrast, grinliz::Vector2I* glow)
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

	static const Vector2I KAMAKIRI_COLORS[] = {
		{ 4, 1 }, { 6, 0 }, {4, 1},
		{ 4, 1 }, { 6, 0 }, {10, 3},
		{ 6, 0 }, { 4, 2 }, { 7, 5 },
		{ 6, 0 }, { 10, 3 }, {4,0}
	};

	static const Vector2I HUMAN_COLORS[] = {
		{ 8, 3 }, { 6, 3 }, { 8, 3 },
		{8, 3}, {12, 5}, {8, 3},
		{10, 3}, {7, 4}, {8, 3},
		{10, 3}, {6, 1}, {10, 3},
		{10, 3}, {4, 2}, {3, 2},
		{6, 1}, {8, 3}, {6, 1},
		//{8, 1}, {6, 0}, {7, 4},
		//{8, 1}, {12, 5}, {8, 2},
		{10, 1}, {6, 4}, {10, 1},
		{6, 2}, {8, 2}, {8, 2},
		{6, 2}, {10, 2}, {6, 2},
		{8, 2}, {10, 3}, {10, 3},
		{10, 2}, {10, 3}, {10, 3},
		{6, 3}, {6, 1}, {6, 1},
		//{10, 3}, {3, 4}, {3, 4},
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

		case TEAM_KAMAKIRI:
		GLASSERT(teamID);
		colors = KAMAKIRI_COLORS;
		nColors = GL_C_ARRAY_SIZE(KAMAKIRI_COLORS) / 3;
		break;

		case TEAM_NEUTRAL:
		colors = NEUTRAL_COLORS;
		nColors = GL_C_ARRAY_SIZE(NEUTRAL_COLORS) / 3;
		break;

		default:
		GLASSERT(0);	// team not implemented?
	}

	int index = abs(teamID) % nColors;
	*base = colors[index * 3 + 0];
	*contrast = colors[index * 3 + 1];
	*glow = colors[index * 3 + 2];
}


void TeamGen::TeamWeaponColors(int team, grinliz::Vector2I* base, grinliz::Vector2I* contrast)
{
	static const Vector2I TROLL_COLORS[] = {
		{ PAL_GRAY * 2, PAL_GREEN }, { PAL_RED * 2, PAL_GREEN }, { PAL_TANGERINE * 2, PAL_GREEN }
	};

	static const Vector2I KAMAKIRI_COLORS[] = {
		{ 4, 2 }, { 6, 0 },
		{ 4, 2 }, { 10, 3 },
		{ 4, 2 }, { 7, 5 },
	};

	static const Vector2I GENERAL_COLORS[] = {
		{4,0}, {6,0},		{4,1}, {8,1},		{6,1}, {6,0},		{8,1}, {10,0},
		{3,2}, {10,0},		{8,2}, {8,3},		{8,2}, {6,4},		{8,2}, {7,4},
		{10,2}, {3,2},		{6,3}, {8,2},		{8,3}, {4,1},		{8,3}, {4,2},
		{8,3}, {8,2},		{8,3}, {7,4},		{8,3}, {7,5},		{8,3}, {10,5},
		{3,4}, {8,3},		{3,4}, {3,2},		{3,4}, {1,3},		{4,4}, {4,2},
		{4,4}, {6,2},		{4,4}, {8,2},		{4,4}, {10,2},		{4,4}, {7,4},
		{5,4}, {8,3},		{5,4}, {8,2},		{6,4}, {4,2},		{6,4}, {7,4},
		{6,4}, {5,4}
	};

	const Vector2I* colors = 0;
	int nColors = 0;
	int teamGroup = 0;
	int teamID = 0;

	Team::SplitID(team, &teamGroup, &teamID);

	switch (teamGroup) {
		case TEAM_HOUSE:	
		case TEAM_GOB:
		GLASSERT(teamID);	// how can a neutral ID be building?
		colors = GENERAL_COLORS;
		nColors = GL_C_ARRAY_SIZE(GENERAL_COLORS) / 2;
		break;

		case TEAM_TROLL:
		colors = TROLL_COLORS;
		nColors = GL_C_ARRAY_SIZE(TROLL_COLORS) / 2;
		break;

		case TEAM_KAMAKIRI:
		colors = KAMAKIRI_COLORS;
		nColors = GL_C_ARRAY_SIZE(KAMAKIRI_COLORS) / 2;
		break;

		case TEAM_NEUTRAL:
		default:
		GLASSERT(0);	// team not implemented?
	}

	int index = abs(teamID) % nColors;
	*base		= colors[index * 2 + 0];
	*contrast	= colors[index * 2 + 1];
}


void TeamGen::Assign(U32 seed, int team, ProcRenderInfo* info)
{

	info->texture = TextureManager::Instance()->GetTexture( "structure" );

	Vector2I vbase, vcontrast, vglow;
	TeamGen::TeamBuildColors(team, &vbase, &vcontrast, &vglow);

	const Game::Palette* palette = Game::GetMainPalette();

	Vector4F base		= palette->Get4F(vbase);
	Vector4F contrast	= palette->Get4F(vcontrast);
	Vector4F glow		= palette->Get4F(vglow);

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

	bool female = (item->IName() == ISC::humanFemale);
	U32 seed = item->ID();
	int team = item->team;
	bool electric = false;	// not using; FIXME revisit 'electric'
	int effects = item->Effects();
	int features = 0;
	item->keyValues.Get(ISC::features, &features);

	AssignProcedural(proc, female, seed, team, electric, effects, features, info);
}


void AssignProcedural( const IString& proc,
					   bool female, U32 seed, int team, bool electric, int effectFlags, int features,
					   ProcRenderInfo* info )
{
	if ( proc == ISC::team) {
		TeamGen::Assign( seed, team, info );
	}
	else if ( proc == ISC::suit) {
		HumanGen gen( female, seed, team, electric );
		gen.AssignSuit( info );
	}
	else if (proc == ISC::ring) {
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignRing( info );
	}
	else if (proc == ISC::gun )
	{
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignGun( info );
	}
	else if ( proc == ISC::shield) {
		WeaponGen gen( seed, team, effectFlags, features );
		gen.AssignShield( info );
	}
	else {
		GLASSERT( 0 );
	}
}

