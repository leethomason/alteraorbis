/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "../libs/SDL2/include/SDL.h"

#include <string>
#include <vector>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/glutil.h"
#include "../grinliz/glstringutil.h"

#include "../engine/enginelimits.h"
#include "../engine/model.h"
#include "../engine/serialize.h"

#include "../tinyxml2/tinyxml2.h"
#include "../shared/dbhelper.h"
#include "../importers/import.h"

#include "../version.h"

#include "modelbuilder.h"
#include "btexture.h"
#include "dither.h"
#include "animationbuilder.h"

#include "../markov/markov.h"
#include "../shared/lodepng.h"

using namespace std;
using namespace grinliz;
using namespace tinyxml2;

string* inputDirectory;
string* inputFullPath;
string* outputPath;

int totalModelMem = 0;
int totalTextureMem = 0;
int totalDataMem = 0;

gamedb::Writer* writer;

void ExitError( const char* tag, 
				const char* pathName,
				const char* assetName,
				const char* message )
{
	printf( "\nERROR: tag='%s' path='%s' asset='%s'. %s\n",
		   tag ? tag : "<no tag>", 
		   pathName ? pathName : "<no path>",
		   assetName ? assetName : "<no asset>", 
		   message );
	exit( 1 );
}


static const char* gMetaName[EL_NUM_METADATA] = {
	"target",
	"trigger",
	"althand",
	"head",
	"shield"
};

const char* IDToMetaData( int id )
{

	GLASSERT( id >= 0 && id < EL_NUM_METADATA );
	return gMetaName[id];
}


int MetaDataToID( const char* n ) 
{
	for( int i=0; i<EL_NUM_METADATA; ++i ) {
		if ( StrEqual( gMetaName[i], n )) {
			return i;
		}
	}
	GLASSERT( 0 );
	return 0;
}


void ModelHeader::Set(	const IString& name, int nAtoms, int nTotalVertices, int nTotalIndices,
						const grinliz::Rectangle3F& bounds )
{
	GLASSERT( nAtoms > 0 && nAtoms < EL_MAX_MODEL_GROUPS );
	GLASSERT( nTotalVertices > 0 && nTotalVertices < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( EL_MAX_VERTEX_IN_MODEL <= 0xffff );
	GLASSERT( nTotalIndices > 0 && nTotalIndices < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( EL_MAX_INDEX_IN_MODEL <= 0xffff );
	GLASSERT( nTotalVertices <= nTotalIndices );

	this->name = name;
	this->flags = 0;
	this->nAtoms = nAtoms;
	this->nTotalVertices = nTotalVertices;
	this->nTotalIndices = nTotalIndices;
	this->bounds = bounds;
	this->animationSpeed = 1.0f;
	memset( metaData, 0, sizeof(ModelMetaData)*EL_NUM_METADATA );
	memset( effectData, 0, sizeof(ModelParticleEffect)*EL_MAX_MODEL_EFFECTS );
}


void ModelHeader::Save( gamedb::WItem* parent )
{
	gamedb::WItem* node = parent->FetchChild( "header" );
	node->SetInt( "nTotalVertices", nTotalVertices );
	node->SetInt( "nTotalIndices", nTotalIndices );
	node->SetInt( "flags", flags );
	node->SetInt( "nGroups", nAtoms );
	if ( !animation.empty() ) {
		node->SetString( "animation", animation.c_str() );
	}
	node->SetFloat("animationSpeed", this->animationSpeed);

	DBSet( node, "bounds", bounds ); 
	gamedb::WItem* metaNode = node->FetchChild( "metaData" );
	for( int i=0; i<EL_NUM_METADATA; ++i ) {
		if ( metaData[i].InUse() ) {
			gamedb::WItem* data = metaNode->FetchChild( i );
			data->SetFloat( "x", metaData[i].pos.x );
			data->SetFloat( "y", metaData[i].pos.y );
			data->SetFloat( "z", metaData[i].pos.z );

			data->SetFloat( "axis.x", metaData[i].axis.x );
			data->SetFloat( "axis.y", metaData[i].axis.y );
			data->SetFloat( "axis.z", metaData[i].axis.z );
			data->SetFloat( "rotation", metaData[i].rotation );

			if( !metaData[i].boneName.empty() ) {
				data->SetString( "boneName", metaData[i].boneName.c_str() );
			}
		}
	}

	gamedb::WItem* effectNode = node->FetchChild( "effectData" );
	for( int i=0; i<EL_MAX_MODEL_EFFECTS; ++i ) {
		if ( !effectData[i].name.empty() ) {
			gamedb::WItem* data = effectNode->FetchChild(i);
			data->SetInt( "metaData", effectData[i].metaData );
			data->SetString( "name", effectData[i].name.c_str() );
		}
	}

	gamedb::WItem* boneNode = node->FetchChild( "bones" );
	for( int i=0; i<EL_MAX_BONES && !modelBoneName[i].empty(); ++i ) {
		gamedb::WItem* data = boneNode->FetchChild( modelBoneName[i].c_str() );
		data->SetInt( "id", i );
	}
}


void ModelGroup::Set( const char* textureName, int nVertex, int nIndex )
{
	GLASSERT( nVertex > 0 && nVertex < EL_MAX_VERTEX_IN_MODEL );
	GLASSERT( nIndex > 0 && nIndex < EL_MAX_INDEX_IN_MODEL );
	GLASSERT( nVertex <= nIndex );

	this->textureName.ClearBuf();
	this->textureName = textureName;
	this->nVertex = nVertex;
	this->nIndex = nIndex;
}


Color4U8 GetPixel( const SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    U8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	U32 c = 0;

    switch(bpp) {
    case 1:
        c = *p;
		break;

    case 2:
        c = *(Uint16 *)p;
		break;

    case 3:
        c =  (p[0] << 16) | (p[1] << 8) | p[2];
		break;

    case 4:
        c = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		break;
    }
	Color4U8 color;

	SDL_GetRGBA( c, surface->format, &color.x, &color.y, &color.z, &color.w );
	return color;
}


void PutPixel(SDL_Surface *surface, int x, int y, const Color4U8& c )
{
	U32 pixel = SDL_MapRGBA( surface->format, c.r(), c.g(), c.b(), c.a() );
    int bpp = surface->format->BytesPerPixel;
    U8 *p = (U8*)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(U16*)p = pixel;
        break;

    case 3:
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
		break;

    case 4:
        p[0] = (pixel >> 24) & 0xff;
        p[1] = (pixel >> 16) & 0xff;
        p[2] = (pixel >> 8) & 0xff;
        p[3] = pixel & 0xff;
		break;
    }
}


template <class STRING>
void AssignIf( STRING& a, const XMLElement* element, const char* attribute ) {
	const char* attrib = element->Attribute( attribute );
	if ( attrib ) {
		a = element->Attribute( attribute );
	}
}


void ParseNames( const XMLElement* element, GLString* _assetName, GLString* _fullPath, GLString* _extension, GLString* _fullPath2=0 )
{
	string filename, filename2;
	AssignIf( filename, element, "filename" );
	AssignIf( filename2, element, "filename2" );

	string assetName;
	AssignIf( assetName, element, "modelName" );
	AssignIf( assetName, element, "assetName" );

	GLString fullIn = inputDirectory->c_str();
	fullIn += filename.c_str();	

	GLString fullIn2;
	if ( filename2.size() > 0 ) {
		fullIn2 = inputDirectory->c_str();
		fullIn2 += filename2.c_str();
	}

	GLString base, name, extension;
	grinliz::StrSplitFilename( fullIn, &base, &name, &extension );
	if ( assetName.empty() ) {
		assetName = name.c_str();
	}

	if ( _assetName )
		*_assetName = assetName.c_str();
	if ( _fullPath )
		*_fullPath = fullIn;
	if ( _extension )
		*_extension = extension;
	if ( _fullPath2 ) {
		*_fullPath2 = fullIn2;
	}
}


void ProcessTreeRec( gamedb::WItem* parent, XMLElement* ele )
{
	string name = ele->Value();
	gamedb::WItem* witem = parent->FetchChild( name.c_str() );

	for( const XMLAttribute* attrib=ele->FirstAttribute(); attrib; attrib=attrib->Next() ) {		
		int i;

		if ( XML_SUCCESS == attrib->QueryIntValue( &i ) ) {
			witem->SetInt( attrib->Name(), i );
		}
		else {
			witem->SetString( attrib->Name(), attrib->Value() );
		}
	}

	if ( ele->GetText() ) {
		witem->SetData( "text", ele->GetText(), strlen( ele->GetText() ) );
	}

	for( XMLElement* child=ele->FirstChildElement(); child; child=child->NextSiblingElement() ) {
		ProcessTreeRec( witem, child );
	}
}


void ProcessTree( XMLElement* data )
{
	// Create the root tree "research"
	gamedb::WItem* witem = writer->Root()->FetchChild( "tree" );
	if ( data->FirstChildElement() )
		ProcessTreeRec( witem, data->FirstChildElement() );
}


void ProcessMarkov(XMLElement* data)
{
	GLString assetName, pathName;
	ParseNames(data, &assetName, &pathName, 0);

	// copy the entire file.
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
#endif

	FILE* read = fopen(pathName.c_str(), "r");

#if defined( _MSC_VER )
#pragma warning (pop)
#endif

	if (!read) {
		ExitError("Markov", pathName.c_str(), assetName.c_str(), "Could not load file.");
		return;	// for static analysis
	}

	MarkovBuilder builder;

	char buffer[256];
	while (fgets(buffer, 256, read)) {
		// Filter out comments.
		const char* p = buffer;
		while (*p && isspace(*p)) {
			++p;
		}
		if (*p == '#')
			continue;

		GLString str = buffer;
		CDynArray< StrToken > tokens;
		StrTokenize(str, &tokens);

		for (int i = 0; i<tokens.Size(); ++i) {
			builder.Push(tokens[i].str);
		}
	}
	builder.Process();
	gamedb::WItem* witem = writer->Root()->FetchChild("markovName")->FetchChild(assetName.c_str());

	HashTable<U32, GLString> nameTable;
	MarkovGenerator generator(builder.Data(), builder.NumBytes(), 1);
	for (int i = 0; i < 4000 && nameTable.Size() < 1000; ++i) {
		GLString str;
		generator.Name(&str, 7);
		if (str.size() >= 4) {
			U32 hash = Random::Hash(str.c_str(), str.size());
			nameTable.Add(hash, str);
		}
	}

	gamedb::WItem* child = witem->FetchChild("names");

	GLString key;
	for (int i = 0; i < nameTable.Size(); ++i) {
		gamedb::WItem* nameChild = child->FetchChild(i);
		nameChild->SetString("name", nameTable.GetValue(i).c_str());
	}
	printf("markovName '%s' numNames=%d\n", assetName.c_str(), nameTable.Size());
	fclose(read);
}


void ProcessMarkovWord( XMLElement* data ) 
{
	GLString assetName, pathName;
	ParseNames( data, &assetName, &pathName, 0 );
	bool insertSpace = true;
	data->QueryAttribute("space", &insertSpace);

	// copy the entire file.
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
#endif

	FILE* read = fopen( pathName.c_str(), "r" );

#if defined( _MSC_VER )
#pragma warning (pop)
#endif

	if ( !read ) {
		ExitError( "Markov", pathName.c_str(), assetName.c_str(), "Could not load file." );
		return;	// for static analysis
	}

	char buffer[256];
	int section = 0;
	static const int MAX_SECTIONS = 3;
	CDynArray<GLString> sectionArr[MAX_SECTIONS];

	while( fgets( buffer, 256, read )) {
		// Filter out comments.
		const char* p = buffer;
		while ( *p && isspace( *p )) {
			++p;
		}
		if ( *p == '#' )
			continue;
		if (!*p)
			continue;

		if (isdigit(*p)) {
			section = *p - '0';
			if (section < 0 || section > MAX_SECTIONS) {
				ExitError("MarkovWord", pathName.c_str(), assetName.c_str(), "Invalid section #.");
			}
		}
		else {
			char* q = (char*)strchr(p, '\n');
			if (q) *q = 0;
			GLString str(p);
			sectionArr[section].Push(str);
		}
	}
	fclose( read );

	CDynArray<GLString> nameList;
	GLASSERT(section == 1);	// just not a general algorithm.
	for (int i = 0; i < sectionArr[0].Size(); ++i) {
		for (int j = 0; j < sectionArr[1].Size(); ++j) {
			GLString str = sectionArr[0][i];
			if (insertSpace) str += " ";
			str.append(sectionArr[1][j]);
			nameList.Push(str);
		}
	}
	Random random;
	random.ShuffleArray(nameList.Mem(), nameList.Size());

	gamedb::WItem* witem = writer->Root()->FetchChild("markovName")->FetchChild(assetName.c_str());
	gamedb::WItem* child = witem->FetchChild("names");

	int SIZE = nameList.Size();
	if (SIZE > 1000) SIZE = 1000;

	for (int i = 0; i < SIZE; ++i) {
		gamedb::WItem* nameChild = child->FetchChild(i);
		nameChild->SetString("name", nameList[i].safe_str());
	}
	printf("markovName-words '%s' numNames=%d\n", assetName.c_str(), SIZE );
}


void ProcessData( XMLElement* data )
{
	bool compression = true;
	const char* compress = data->Attribute( "compress" );
	if ( compress && StrEqual( compress, "false" ) ) {
		compression = false;
	}

	string filename;
	AssignIf( filename, data, "filename" );
	string fullIn = *inputDirectory + filename;

	GLString assetName, pathName;
	ParseNames( data, &assetName, &pathName, 0 );

	// copy the entire file.
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
#endif
	FILE* read = fopen( pathName.c_str(), "rb" );

#if defined( _MSC_VER )
#pragma warning (pop)
#endif

	if ( !read ) {
		ExitError( "Data", pathName.c_str(), assetName.c_str(), "Could not load asset file." );
	}

	// length of file.
	fseek( read, 0, SEEK_END );
	int len = ftell( read );
	fseek( read, 0, SEEK_SET );

	char* mem = new char[len];
	fread( mem, len, 1, read );

	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )->FetchChild( assetName.c_str() );
	witem->SetData( "binary", mem, len, compression );

	delete [] mem;

	printf( "Data '%s' memory=%dk compression=%s\n", filename.c_str(), len/1024, compression ? "true" : "false" );
	totalDataMem += len;

	fclose( read );
}


class TextBuilder : public XMLVisitor
{
public:
	string str;
	string filter;

	bool textOn;
	int depth;

	TextBuilder() : textOn( false ), depth( 0 ) {}

	virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* firstAttribute )
	{
		++depth;

		// depth = 1 element
		// depth = 2 <p>
		if ( depth == 2 ) {
			string platform;
			AssignIf( platform, &element, "system" );

			if ( platform.empty() || platform == filter )
				textOn = true;
		}
		return true;
	}
	virtual bool VisitExit( const XMLElement& element )
	{
		if ( depth == 2 ) {
			if ( textOn ) {
				str += "\n\n";
				textOn = false;
			}
		}
		--depth;
		return true;
	}
	virtual bool Visit( const XMLText& text) {
		if ( textOn )
			str += text.Value();
		return true;
	}
};


void ProcessText( XMLElement* textEle )
{
	string name;
	AssignIf( name, textEle, "name" );
	gamedb::WItem* textItem = writer->Root()->FetchChild( "text" )->FetchChild( name.c_str() );
	int index = 0;

	for( XMLElement* pageEle = textEle->FirstChildElement( "page" ); pageEle; pageEle = pageEle->NextSiblingElement( "page" ) ) {
		string pcText, androidText;

		TextBuilder textBuilder;
		textBuilder.filter = "pc";
		pageEle->Accept( &textBuilder );
		pcText = textBuilder.str;

		textBuilder.str.clear();
		textBuilder.filter = "android";
		pageEle->Accept( &textBuilder );
		androidText = textBuilder.str;

		gamedb::WItem* pageItem = textItem->FetchChild( index );

		if ( pcText == androidText ) {
			pageItem->SetData( "text", pcText.c_str(), pcText.size() );
			totalDataMem += pcText.size();
		}
		else {
			pageItem->SetData( "text_pc", pcText.c_str(), pcText.size() );
			pageItem->SetData( "text_android", androidText.c_str(), androidText.size() );
			totalDataMem += pcText.size();
			totalDataMem += androidText.size();
		}

		if ( pageEle->Attribute( "image" ) ) {
			pageItem->SetString( "image", pageEle->Attribute( "image" ) );
		}
		printf( "Text '%s' page %d\n", name.c_str(), index );
		index++;
	}
}

void StringToVector( const char* str, Vector3F* vec )
{
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )
#endif
		sscanf( str, "%f %f %f", &vec->x, &vec->y, &vec->z );

#if defined( _MSC_VER )
#pragma warning ( pop )
#endif
}


void StringToVector( const char* str, Vector4F* vec )
{
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )
#endif
		sscanf( str, "%f %f %f %f", &vec->x, &vec->y, &vec->z, &vec->w );
#if defined( _MSC_VER )
#pragma warning ( pop )
#endif
}


void StringToAxisAngle( const char* str, Vector3F* axis, float* angle )
{
#if defined( _MSC_VER )
#pragma warning ( push )
#pragma warning ( disable : 4996 )
#endif
		sscanf( str, "%f %f %f %f", &axis->x, &axis->y, &axis->z, angle );
#if defined( _MSC_VER )
#pragma warning ( pop )
#endif
}


void ProcessModel( XMLElement* model )
{
	int nTotalIndex = 0;
	int nTotalVertex = 0;
	int startIndex = 0;
	int startVertex = 0;
	Vector3F origin = { 0, 0, 0 };

	GLString pathName, assetName, extension;
	ParseNames( model, &assetName, &pathName, &extension );

	if ( model->Attribute( "origin" ) ) {
		StringToVector( model->Attribute( "origin" ), &origin );
	}
	bool usingSubModels = false;
	if ( model->Attribute( "modelName" ) ) {
		usingSubModels = true;
	}
	
	printf( "Model '%s'", assetName.c_str() );

	ModelBuilder* builder = new ModelBuilder();
//	builder->SetAtlasPool( atlasArr, MAX_ATLAS );
	builder->SetShading( ModelBuilder::FLAT );

	GLString animation = "";
	if ( model->Attribute( "animation" ) ) {
		builder->EnableBones( true );
		animation = model->Attribute( "animation" );
	}
	if ( model->Attribute( "bones" )) {
		bool b;
		model->QueryBoolAttribute( "bones", &b );
		builder->EnableBones( b );
	}

	if ( grinliz::StrEqual( model->Attribute( "shading" ), "smooth" ) ) {
		builder->SetShading( ModelBuilder::SMOOTH );
	}
	else if ( grinliz::StrEqual( model->Attribute( "shading" ), "crease" ) ) {
		builder->SetShading( ModelBuilder::CREASE );
	}
	else if ( grinliz::StrEqual(  model->Attribute( "shading" ), "smooth" ) ) {
		builder->SetShading( ModelBuilder::SMOOTH );
	}

	if ( grinliz::StrEqual( model->Attribute( "polyRemoval" ), "pre" ) ) {
		builder->SetPolygonRemoval( ModelBuilder::POLY_PRE );
	}

	if ( extension == ".ac" ) {
		ImportAC3D(	std::string( pathName.c_str() ), 
			        builder, 
					origin, 
					usingSubModels ? string( assetName.c_str()) : "" );
	}
	else if ( extension == ".off" ) {
		ImportOFF( std::string( pathName.c_str() ), builder );
	}
	else {
		ExitError( "Model", pathName.c_str(), assetName.c_str(), "Could not load asset file." ); 
	}

	const VertexGroup* vertexGroup = builder->Groups();
	
	for( int i=0; i<builder->NumGroups(); ++i ) {
		nTotalIndex += vertexGroup[i].nIndex;
		nTotalVertex += vertexGroup[i].nVertex;
	}
	if ( builder->PolyCulled() ) {
		printf( " culled=%d", builder->PolyCulled() );
	}	
	printf( " groups=%d nVertex=%d nTri=%d\n", builder->NumGroups(), nTotalVertex, nTotalIndex/3 );

	ModelHeader header;	
	header.Set( StringPool::Intern( assetName.c_str() ), builder->NumGroups(), nTotalVertex, nTotalIndex, builder->Bounds() );
	header.animation = StringPool::Intern( animation.c_str() );

	for( int i=0; i<builder->boneNames.Size(); ++i ) {
		header.modelBoneName[i] = builder->boneNames[i];
	}

	if ( grinliz::StrEqual( model->Attribute( "shadowCaster" ), "false" ) ) {
		header.flags |= ModelHeader::RESOURCE_NO_SHADOW;
	}
	model->QueryFloatAttribute("animationSpeed", &header.animationSpeed);

	for(	const XMLElement* metaEle = model->FirstChildElement( "meta" );
			metaEle;
			metaEle = metaEle->NextSiblingElement( "meta" ) )
	{
		int id = MetaDataToID( metaEle->Attribute( "name" ));
		GLASSERT( metaEle->Attribute( "pos" ));
		if ( metaEle->Attribute( "pos" )) {
			StringToVector( metaEle->Attribute( "pos" ), &header.metaData[id].pos );
			header.metaData[id].pos = header.metaData[id].pos - origin;
		}
		if ( metaEle->Attribute( "rot" )) {
			StringToAxisAngle( metaEle->Attribute( "rot" ), &header.metaData[id].axis, &header.metaData[id].rotation );
		}
		header.metaData[id].boneName = StringPool::Intern( metaEle->Attribute( "boneName" ) );
	}

	int nEffect = 0;
	for(	const XMLElement* effectEle = model->FirstChildElement( "particle" );
			effectEle && nEffect < EL_MAX_MODEL_EFFECTS;
			effectEle = effectEle->NextSiblingElement( "particle" ))
	{
		header.effectData[nEffect].metaData = MetaDataToID(effectEle->Attribute( "meta" ));
		header.effectData[nEffect].name     = StringPool::Intern( effectEle->Attribute( "name" ) );
		++nEffect;
	}

	gamedb::WItem* witem = writer->Root()->FetchChild( "models" )->FetchChild( assetName.c_str() );

	int totalMemory = 0;

	for( int i=0; i<builder->NumGroups(); ++i ) {
		int mem = vertexGroup[i].nVertex*sizeof(Vertex) + vertexGroup[i].nIndex*2;
		totalMemory += mem;
		printf( "    %d: '%s' nVertex=%d nTri=%d memory=%.1fk\n",
				i,
				vertexGroup[i].textureName.c_str(),
				vertexGroup[i].nVertex,
				vertexGroup[i].nIndex / 3,
				(float)mem/1024.0f );

		ModelGroup group;
		group.Set( vertexGroup[i].textureName.c_str(), vertexGroup[i].nVertex, vertexGroup[i].nIndex );

		gamedb::WItem* witemGroup = witem->FetchChild( i );
		if ( group.textureName.empty() ) {
			ExitError( "Model", pathName.c_str(), assetName.c_str(), "no texture specified" );
		}
		witemGroup->SetString( "textureName", group.textureName.c_str() );
		witemGroup->SetInt( "nVertex", group.nVertex );
		witemGroup->SetInt( "nIndex", group.nIndex );

		startVertex += vertexGroup[i].nVertex;
		startIndex += vertexGroup[i].nIndex;
	}

	Vertex* vertexBuf = new Vertex[nTotalVertex];
	U16* indexBuf = new U16[nTotalIndex];

	Vertex* pVertex = vertexBuf;
	U16* pIndex = indexBuf;
	
	const Vertex* pVertexEnd = vertexBuf + nTotalVertex;
	const U16* pIndexEnd = indexBuf + nTotalIndex;

	// Write the vertices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		GLASSERT( pVertex + vertexGroup[i].nVertex <= pVertexEnd );
		memcpy( pVertex, vertexGroup[i].vertex, sizeof(Vertex)*vertexGroup[i].nVertex );
		pVertex += vertexGroup[i].nVertex;
	}
	// Write the indices in each group:
	for( int i=0; i<builder->NumGroups(); ++i ) {
		GLASSERT( pIndex + vertexGroup[i].nIndex <= pIndexEnd );
		memcpy( pIndex, vertexGroup[i].index, sizeof(U16)*vertexGroup[i].nIndex );
		pIndex += vertexGroup[i].nIndex;
	}
	GLASSERT( pVertex == pVertexEnd );
	GLASSERT( pIndex == pIndexEnd );

	//witem->SetData( "header", &header, sizeof(header ) );
	header.Save( witem );
	witem->SetData( "vertex", vertexBuf, nTotalVertex*sizeof(Vertex) );
	witem->SetData( "index", indexBuf, nTotalIndex*sizeof(U16) );

	printf( "  total memory=%.1fk\n", (float)totalMemory / 1024.f );
	totalModelMem += totalMemory;
	
	delete [] vertexBuf;
	delete [] indexBuf;
	delete builder;
}


void ProcessTexture( XMLElement* texture )
{
	BTexture btexture;
	btexture.ParseTag( texture );

	GLString pathName, assetName, pathName2;
	ParseNames( texture, &assetName, &pathName, 0, &pathName2 );
	btexture.SetNames( assetName, pathName, pathName2 );

	btexture.Load();
	btexture.Scale();

	btexture.ToBuffer();
	gamedb::WItem* witem = btexture.InsertTextureToDB( writer->Root()->FetchChild( "textures" ) );

	// If present, write the texture packer data about the image.
	const char* tableName = texture->Attribute( "table" );
	if ( tableName ) {
		gamedb::WItem* table = witem->FetchChild( "table" );

		XMLDocument doc;
		std::string tpath = *inputDirectory + tableName;
		doc.LoadFile( tpath.c_str() );
		if ( doc.Error() ) {
			ExitError( "ProcessTexture", tableName, 0, "Failed to load texture table." );
		}

		const XMLElement* atlasEle = doc.FirstChildElement( "TextureAtlas" );
		if ( !atlasEle ) {
			ExitError( "ProcessTexture", tableName, 0, "Failed to load texture table." );
			return;	// make static analysis happy
		}
		float width = 1;
		float height = 1;
		atlasEle->QueryAttribute( "width", &width );
		atlasEle->QueryAttribute( "height", &height );
		for( const XMLElement* spriteEle = atlasEle->FirstChildElement( "sprite" );
			 spriteEle;
			 spriteEle = spriteEle->NextSiblingElement( "sprite" ))
		{
			const char* name = spriteEle->Attribute( "n" );
			gamedb::WItem* entry = table->FetchChild( name );
			
			float x=0, y=0, w=0, h=0;
			spriteEle->QueryAttribute( "x",  &x );
			spriteEle->QueryAttribute( "y",  &y );
			spriteEle->QueryAttribute( "w",  &w );
			spriteEle->QueryAttribute( "h",  &h );
			float oX=0, oY=0, oW=w, oH=h;
			spriteEle->QueryAttribute( "oX", &oX );
			spriteEle->QueryAttribute( "oY", &oY );
			spriteEle->QueryAttribute( "oW", &oW );
			spriteEle->QueryAttribute( "oH", &oH );

			entry->SetFloat( "x",  x / width );
			entry->SetFloat( "y",  y / height );
			entry->SetFloat( "w",  w / width );
			entry->SetFloat( "h",  h / height );
			entry->SetFloat( "oX", oX / width );
			entry->SetFloat( "oY", oY / height );
			entry->SetFloat( "oW", oW / width );
			entry->SetFloat( "oH", oH / height );
		}
	}
}

/*
void ProcessAtlas( XMLElement* atlasElement )
{
	static const int MAX = 40;	// increase as needed
	BTexture btextureArr[MAX];
	int index=0;

	for( const XMLElement* texture = atlasElement->FirstChildElement( "texture" );
		 texture;
		 texture = texture->NextSiblingElement( "texture" ))
	{
		GLASSERT( index < MAX );
		if ( index >= MAX ) {
			ExitError( "Atlas", 0, 0, "Atlas limit exceeded." );
		}
		btextureArr[index].ParseTag( texture );

		GLString pathName, assetName;
		ParseNames( texture, &assetName, &pathName, 0 );
		btextureArr[index].SetNames( assetName, pathName, "" );

		btextureArr[index].Load();
		btextureArr[index].Scale();
		++index;
	}
	int maxWidth = 1024;
	atlasElement->QueryIntAttribute( "width", &maxWidth );

	int it = 0;
	for( it=0; it<MAX_ATLAS; ++it ) {
		if ( !atlasArr[it].btexture.surface ) {
			break;
		}
	}
	if ( it == MAX_ATLAS ) {
		ExitError( "Atlas", 0, 0, "Out of atlases." );
	}

	GLString name = "atlas";
	name += (char)('0' + it);
	atlasArr[it].btexture.SetNames( name, "", "" );

	atlasArr[it].Generate( btextureArr, index, maxWidth );
	atlasArr[it].btexture.dither = true;
	atlasArr[it].btexture.ToBuffer();
	atlasArr[it].btexture.InsertTextureToDB(  writer->Root()->FetchChild( "textures" ) );
}
*/

SDL_Surface* LoadImage( const char* pathname )
{
	FILE* fp = fopen( pathname, "rb" );
	if ( !fp ) return 0;

	fseek( fp, 0, SEEK_END );
	int size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	if ( size == 0 ) return 0;

	U8* data = new U8[size];
	fread( data, 1, size, fp );
	fclose( fp );

	unsigned w, h, error=0;
	LodePNGState state;
	memset( &state, 0, sizeof(state) );
	lodepng_inspect( &w, &h, &state, data, size );
	SDL_Surface* surface = 0;

	if ( state.info_png.color.colortype == LCT_RGB ) {
		U8* out = 0;
		error = lodepng_decode24( &out, &w, &h, data, size );
		GLASSERT( !error );
		surface = SDL_CreateRGBSurface( 0, w, h, 24, 0xff0000, 0x00ff00, 0x0000ff, 0 );
		GLASSERT( surface->pitch == int(w * 3));
		memcpy( surface->pixels, out, w*h*3 );
		free( out );
	}
	else if ( state.info_png.color.colortype == LCT_RGBA ) {
		U8* out = 0;
		error = lodepng_decode32( &out, &w, &h, data, size );
		GLASSERT( !error );
		surface = SDL_CreateRGBSurface( 0, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
		GLASSERT( surface->pitch == int(w*4) );
		memcpy( surface->pixels, out, w*h*4 );
		free( out );
	}
	else if ( state.info_png.color.colortype == LCT_GREY ) {
		U8* out = 0;
		error = lodepng_decode_memory( &out, &w, &h, data, size, LCT_GREY, 8  );
		GLASSERT( !error );
		surface = SDL_CreateRGBSurface( 0, w, h, 8, 0, 0, 0, 0xff );
		GLASSERT( surface->pitch == int(w) );
		memcpy( surface->pixels, out, w*h );
		free( out );
	}
	else {
		GLASSERT( 0 );
	}

	delete [] data;
	return surface;
}


void ProcessPalette( XMLElement* pal )
{
	int dx=0;
	int dy=0;
	pal->QueryIntAttribute( "dx", &dx );
	pal->QueryIntAttribute( "dy", &dy );
	float sx=0, sy=0;
	pal->QueryFloatAttribute( "sx", &sx );
	pal->QueryFloatAttribute( "sy", &sy );
	
	GLString pathName, assetName;
	ParseNames( pal, &assetName, &pathName, 0 );

	SDL_Surface* surface = LoadImage( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Palette", pathName.c_str(), assetName.c_str(), "Could not load asset." );
		return;	// for static analysis
	}
	printf( "Palette[data] asset=%s cx=%d cy=%d sx=%f sy=%f\n",
		    assetName.c_str(), dx, dy, sx, sy );

	std::vector<Color4U8> colors;
	int offsetX = (int)(sx * (float)(surface->w / dx));
	int offsetY = (int)(sy * (float)(surface->h / dy));

	for( int y=0; y<dy; ++y ) {
		for( int x=0; x<dx; ++x ) {
			int px = surface->w * x / dx + offsetX;
			int py = surface->h * y / dy + offsetY;
			Color4U8 rgba = GetPixel( surface, px, py );
			colors.push_back( rgba );
		}
	}

	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )
										 ->FetchChild( "palettes" )
										 ->FetchChild( assetName.c_str() );
	witem->SetInt( "dx", dx );
	witem->SetInt( "dy", dy );
	witem->SetData( "colors", &colors[0], dx*dy*sizeof(Color4U8), true );

	if ( surface ) { 
		SDL_FreeSurface( surface );
	}
}


void ProcessFont( XMLElement* font )
{
	GLString pathName, assetName;
	ParseNames( font, &assetName, &pathName, 0 );

	SDL_Surface* surface = LoadImage( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Font", pathName.c_str(), assetName.c_str(), "Could not load asset." );
	}
	ProcessTexture( font );

	printf( "font asset='%s'\n",
		    assetName.c_str() );
	gamedb::WItem* witem = writer->Root()->FetchChild( "data" )
										 ->FetchChild( "fonts" )
										 ->FetchChild( assetName.c_str() );
	
	// Read the data tags. These describe the font.
	static const char* tags[] = { "info", "common", 0 };
	for( int i=0; tags[i]; ++i ) {
		const char* tag = tags[i];
		
		const XMLElement* element = font->FirstChildElement( tag );
		if ( !element ) {
			char buf[90];
			SNPrintf( buf, 90, "XML Element %s not found.", tag );
			ExitError( "Font", pathName.c_str(), assetName.c_str(), buf );
			return;
		}
		
		gamedb::WItem* tagItem = witem->FetchChild( tag );
		for( const XMLAttribute* attrib=element->FirstAttribute();
			 attrib;
			 attrib = attrib->Next() )
		{
			tagItem->SetInt( attrib->Name(), attrib->IntValue() );
		}
	}

	// Character data.
	const XMLElement* charsElement = font->FirstChildElement( "chars" );
	if ( !charsElement ) {
		ExitError( "Font", pathName.c_str(), assetName.c_str(), "Font contains no 'chars' XML Element." );
		return;
	}

	gamedb::WItem* charsItem = witem->FetchChild( "chars" );
	for( const XMLElement* charElement = charsElement->FirstChildElement( "char" );
		 charElement;
		 charElement = charElement->NextSiblingElement( "char" ) )
	{
		// Gamedb items must be unique. Munge name and id.
		char buffer[6] = "char0";
		int id = 1;
		charElement->QueryIntAttribute( "id", &id );
		buffer[4] = id;
		gamedb::WItem* charItem = charsItem->FetchChild( buffer );

		for( const XMLAttribute* attrib=charElement->FirstAttribute();
			 attrib;
			 attrib = attrib->Next() )
		{
			if ( !StrEqual( attrib->Name(), "id" ) ) {
				charItem->SetInt( attrib->Name(), attrib->IntValue() );
			}
		}
	}

	const XMLElement* kerningsElement = font->FirstChildElement( "kernings" );
	if ( !kerningsElement ) {
		return;
	}
	gamedb::WItem* kerningsItem = witem->FetchChild( "kernings" );
	for( const XMLElement* kerningElement = kerningsElement->FirstChildElement( "kerning" );
		 kerningElement;
		 kerningElement = kerningElement->NextSiblingElement( "kerning" ) )
	{
		// Gamedb items must be unique. Munge name and id.
		char buffer[10] = "kerning00";
		int first = 1, second = 1;
		kerningElement->QueryIntAttribute( "first", &first );
		kerningElement->QueryIntAttribute( "second", &second );
		buffer[7] = first;
		buffer[8] = second;
		gamedb::WItem* kerningItem = kerningsItem->FetchChild( buffer );
		int amount = 0;
		kerningElement->QueryIntAttribute( "amount", &amount );
		kerningItem->SetInt( "amount",	amount );
	}
}


int main( int argc, char* argv[] )
{
	printf( "UFO Builder. version='%s' argc=%d argv[1]=%s\n", VERSION, argc, argv[1] );
	if ( argc < 3 ) {
		printf( "Usage: ufobuilder ./path/xmlFile.xml ./outPath/filename.db <options>\n" );
		printf( "options:\n" );
		printf( "    -d    print database\n" );
		printf( "    -b    print output bitmaps\n" );
		exit( 0 );
	}

    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER );

	inputDirectory = new string();
	inputFullPath = new string();
	outputPath = new string();

	*inputFullPath = argv[1];
	*outputPath = argv[2];
	bool printDatabase = false;
	for( int i=3; i<argc; ++i ) {
		if ( StrEqual( argv[i], "-d" ) ) {
			printDatabase = true;
		}
		if ( StrEqual( argv[i], "-b" )) {
			BTexture::logToPNG = true;
		}
	}

	GLString _inputFullPath( inputFullPath->c_str() ), _inputDirectory, name, extension;
	grinliz::StrSplitFilename( _inputFullPath, &_inputDirectory, &name, &extension );
	*inputDirectory = _inputDirectory.c_str();

//	const char* xmlfile = argv[2];
	printf( "Opening, path: '%s' filename: '%s'\n", inputDirectory->c_str(), inputFullPath->c_str() );
	
	// Test:
	
	{
		string testInput = *inputDirectory + "Lenna.png";
		SDL_Surface* surface = LoadImage( testInput.c_str() );
		BTexture btexture;
		btexture.Create( surface->w, surface->h, TEX_RGB16 );
		btexture.ToBuffer();

		if ( surface ) {
			// 444

			OrderedDitherTo16( surface, TEX_RGBA16, false, (U16*)btexture.pixelBuffer );
			SDL_Surface* newSurf = SDL_CreateRGBSurface( 0, surface->w, surface->h, 16, 0xf000, 0x0f00, 0x00f0, 0x000f );
			GLASSERT( newSurf->pitch == surface->w*2 );
			memcpy( newSurf->pixels, btexture.pixelBuffer, surface->w*surface->h*2 );
			string out = *inputDirectory + "Lenna4440.bmp";
			SDL_SaveBMP( newSurf, out.c_str() );
			
			SDL_FreeSurface( newSurf );

			// 565
			OrderedDitherTo16( surface, TEX_RGB16, false, (U16*)btexture.pixelBuffer );
			newSurf = SDL_CreateRGBSurface(	0, surface->w, surface->h, 16, 0xf800, 0x07e0, 0x001f, 0 );
			GLASSERT( newSurf->pitch == surface->w*2 );
			memcpy( newSurf->pixels, btexture.pixelBuffer, surface->w*surface->h*2 );
			string out1 = *inputDirectory + "Lenna565.bmp";
			SDL_SaveBMP( newSurf, out1.c_str() );

			SDL_FreeSurface( newSurf );

#if 0
			// 565 Diffusion
			DiffusionDitherTo16( surface, RGB16, false, btexture.pixelBuffer16 );
			newSurf = SDL_CreateRGBSurface(	06, surface->w, surface->h, 16,	0xf800, 0x07e0, 0x001f, 0 );
			GLASSERT( newSurf->pitch == surface->w*2 );
			memcpy( newSurf->pixels, btexture.pixelBuffer16, surface->w*surface->h*2 );

			string out2 = inputDirectory + "Lenna565Diffuse.bmp";
			SDL_SaveBMP( newSurf, out2.c_str() );
			SDL_FreeSurface( newSurf );
#endif
			
			SDL_FreeSurface( surface );
		}
	}
	

	XMLDocument xmlDoc;
	xmlDoc.LoadFile( inputFullPath->c_str() );
	if ( xmlDoc.Error() || !xmlDoc.FirstChildElement() ) {
		xmlDoc.PrintError();
		exit( 2 );
	}

	printf( "Processing tags:\n" );

	// Remove the old table.
	writer = new gamedb::Writer();

	for( XMLElement* child = xmlDoc.FirstChildElement()->FirstChildElement();
		 child;
		 child = child->NextSiblingElement() )
	{
		if (    StrEqual( child->Value(), "texture" )
			 || StrEqual( child->Value(), "image" ))
		{
			ProcessTexture( child );
		}
		else if ( StrEqual( child->Value(),  "model" )) {
			ProcessModel( child );
		}
		else if ( StrEqual( child->Value(), "animation" )) {
			ProcessAnimation( child, writer->Root()->FetchChild( "animations" ) );
		}
		else if ( StrEqual( child->Value(), "data" )) {
			ProcessData( child );
		}
		else if ( StrEqual( child->Value(), "text" )) {
			ProcessText( child );
		}
		else if ( StrEqual( child->Value(), "tree" )) {
			ProcessTree( child );
		}
		else if ( StrEqual( child->Value(), "palette" )) {
			ProcessPalette( child );
		}
		else if ( StrEqual( child->Value(), "font" )) {
			ProcessFont( child );
		}
//		else if ( StrEqual( child->Value(), "atlas" )) {
//			ProcessAtlas( child );
//		}
		else if ( StrEqual( child->Value(), "markov" )) {
			ProcessMarkov( child );
		}
		else if (StrEqual(child->Value(), "markov-word")) {
			ProcessMarkovWord(child);
		}
		else {
			printf( "Unrecognized element: %s\n", child->Value() );
			ExitError( 0, 0, 0, "Unrecognized Element." );
		}
	}

	int total = totalTextureMem + totalModelMem + totalDataMem;

	printf( "Total memory=%dk Texture=%dk Model=%dk Data=%dk\n", 
			total/1024, totalTextureMem/1024, totalModelMem/1024, totalDataMem/1024 );
	printf( "All done.\n" );
	SDL_Quit();

	writer->Save( outputPath->c_str() );
	delete writer;

	if ( printDatabase ) {
		gamedb::Reader reader;
		reader.Init( 0, outputPath->c_str(), 0 );
		reader.RecWalk( reader.Root(), 0 );
		reader.Manifest(2);
	}

	delete inputDirectory;
	delete inputFullPath;
	delete outputPath;
	delete StringPool::Instance();
	return 0;
}