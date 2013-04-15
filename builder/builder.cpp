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

#include "SDL.h"
#if defined(__APPLE__)	// OSX, not iphone
#include "SDL_image.h"
#endif
#include "SDL_loadso.h"

#include <string>
#include <vector>

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glcolor.h"
#include "../grinliz/gldynamic.h"
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
#include "atlas.h"
#include "animationbuilder.h"

using namespace std;
using namespace grinliz;
using namespace tinyxml2;

typedef SDL_Surface* (SDLCALL * PFN_IMG_LOAD) (const char *file);
PFN_IMG_LOAD libIMG_Load;

string inputDirectory;
string inputFullPath;
string outputPath;

int totalModelMem = 0;
int totalTextureMem = 0;
int totalDataMem = 0;

gamedb::Writer* writer;

static const int MAX_ATLAS = 4;
Atlas atlasArr[MAX_ATLAS];


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



void LoadLibrary()
{
	#if defined(__APPLE__)
		libIMG_Load = &IMG_Load;
	#else
		void* handle = grinliz::grinlizLoadLibrary( "SDL_image" );
		if ( !handle )
		{	
			ExitError( "Initialization", 0, 0, "Could not load SDL_image library." );
		}
		libIMG_Load = (PFN_IMG_LOAD) grinliz::grinlizLoadFunction( handle, "IMG_Load" );
		GLASSERT( libIMG_Load );
	#endif
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
	memset( metaData, 0, sizeof(ModelMetaData)*EL_MAX_METADATA );
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

	DBSet( node, "bounds", bounds ); 
/*	gamedb::WItem* boundNode = node->FetchChild( "bounds" );
	boundNode->SetFloat( "min.x", bounds.min.x );
	boundNode->SetFloat( "min.y", bounds.min.y );
	boundNode->SetFloat( "min.z", bounds.min.z );
	boundNode->SetFloat( "max.x", bounds.max.x );
	boundNode->SetFloat( "max.y", bounds.max.y );
	boundNode->SetFloat( "max.z", bounds.max.z );
*/
	gamedb::WItem* metaNode = node->FetchChild( "metaData" );
	for( int i=0; i<EL_MAX_METADATA; ++i ) {
		if ( !metaData[i].name.empty() ) {
			gamedb::WItem* data = metaNode->FetchChild( metaData[i].name.c_str() );
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
			data->SetString( "metaData", effectData[i].metaData.c_str() );
			data->SetString( "name", effectData[i].name.c_str() );
		}
	}

	gamedb::WItem* boneNode = node->FetchChild( "bones" );
	for( int i=0; i<EL_MAX_BONES && !boneName[i].empty(); ++i ) {
		gamedb::WItem* data = boneNode->FetchChild( boneName[i].c_str() );
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
    /* Here p is the address to the pixel we want to retrieve */
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
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            c =  p[0] << 16 | p[1] << 8 | p[2];
        else
            c = p[0] | p[1] << 8 | p[2] << 16;
		break;

    case 4:
        c = *(U32 *)p;
		break;
    }
	Color4U8 color;
	SDL_GetRGBA( c, surface->format, &color.r, &color.g, &color.b, &color.a );
	return color;
}


void PutPixel(SDL_Surface *surface, int x, int y, const Color4U8& c )
{
	U32 pixel = SDL_MapRGBA( surface->format, c.r, c.g, c.b, c.a );
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
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(U32*)p = pixel;
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

	GLString fullIn = inputDirectory.c_str();
	fullIn += filename.c_str();	

	GLString fullIn2;
	if ( filename2.size() > 0 ) {
		fullIn2 = inputDirectory.c_str();
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




void ProcessData( XMLElement* data )
{
	bool compression = true;
	const char* compress = data->Attribute( "compress" );
	if ( compress && StrEqual( compress, "false" ) ) {
		compression = false;
	}

	string filename;
	AssignIf( filename, data, "filename" );
	string fullIn = inputDirectory + filename;

	GLString assetName, pathName;
	ParseNames( data, &assetName, &pathName, 0 );

	// copy the entire file.
#pragma warning ( push )
#pragma warning ( disable : 4996 )	// fopen is unsafe. For video games that seems extreme.
	FILE* read = fopen( pathName.c_str(), "rb" );
#pragma warning (pop)

	if ( !read ) {
		ExitError( "Data", pathName.c_str(), assetName.c_str(), "Could not load asset file." );
	}

	// length of file.
	fseek( read, 0, SEEK_END );
	int len = ftell( read );
	fseek( read, 0, SEEK_SET );

	char* mem = new char[len];
	fread( mem, len, 1, read );

	int index = 0;
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
#pragma warning ( push )
#pragma warning ( disable : 4996 )
		sscanf( str, "%f %f %f", &vec->x, &vec->y, &vec->z );
#pragma warning ( pop )
}


void StringToAxisAngle( const char* str, Vector3F* axis, float* angle )
{
#pragma warning ( push )
#pragma warning ( disable : 4996 )
		sscanf( str, "%f %f %f %f", &axis->x, &axis->y, &axis->z, angle );
#pragma warning ( pop )
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
	builder->SetAtlasPool( atlasArr, MAX_ATLAS );
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
		header.boneName[i] = builder->boneNames[i];
	}

	if ( grinliz::StrEqual( model->Attribute( "shadowCaster" ), "false" ) ) {
		header.flags |= ModelHeader::RESOURCE_NO_SHADOW;
	}

	int nMeta = 0;
	for(	const XMLElement* metaEle = model->FirstChildElement( "meta" );
			metaEle && nMeta < EL_MAX_METADATA;
			metaEle = metaEle->NextSiblingElement( "meta" ) )
	{
		header.metaData[nMeta].name = StringPool::Intern( metaEle->Attribute( "name" ) );
		if ( metaEle->Attribute( "pos" )) {
			StringToVector( metaEle->Attribute( "pos" ), &header.metaData[nMeta].pos );
			header.metaData[nMeta].pos = header.metaData[nMeta].pos - origin;
		}
		if ( metaEle->Attribute( "rot" )) {
			StringToAxisAngle( metaEle->Attribute( "rot" ), &header.metaData[nMeta].axis, &header.metaData[nMeta].rotation );
		}
		header.metaData[nMeta].boneName = StringPool::Intern( metaEle->Attribute( "boneName" ) );
		++nMeta;
	}

	int nEffect = 0;
	for(	const XMLElement* effectEle = model->FirstChildElement( "particle" );
			effectEle && nEffect < EL_MAX_MODEL_EFFECTS;
			effectEle = effectEle->NextSiblingElement( "particle" ))
	{
		header.effectData[nEffect].metaData = StringPool::Intern( effectEle->Attribute( "meta" ) );
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

	int vertexID = 0, indexID = 0;

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

	const char* tableName = texture->Attribute( "table" );
	if ( tableName ) {
		gamedb::WItem* table = witem->FetchChild( "table" );

		XMLDocument doc;
		std::string tpath = inputDirectory + tableName;
		doc.LoadFile( tpath.c_str() );
		if ( doc.Error() ) {
			ExitError( "ProcessTexture", tableName, 0, "Failed to load texture table." );
		}

		const XMLElement* atlasEle = doc.FirstChildElement( "TextureAtlas" );
		if ( !atlasEle ) {
			ExitError( "ProcessTexture", tableName, 0, "Failed to load texture table." );
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
			
			float x=0, y=0, w=0, h=0, oX=0, oY=0, oW=0, oH=0;
			spriteEle->QueryAttribute( "x",  &x );
			spriteEle->QueryAttribute( "y",  &y );
			spriteEle->QueryAttribute( "w",  &w );
			spriteEle->QueryAttribute( "h",  &h );
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

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
	if ( !surface ) {
		ExitError( "Palette", pathName.c_str(), assetName.c_str(), "Could not load asset." );
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

	SDL_Surface* surface = libIMG_Load( pathName.c_str() );
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
	printf( "UFO Builder. version=%d argc=%d argv[1]=%s\n", VERSION, argc, argv[1] );
	if ( argc < 3 ) {
		printf( "Usage: ufobuilder ./path/xmlFile.xml ./outPath/filename.db <options>\n" );
		printf( "options:\n" );
		printf( "    -d    print database\n" );
		exit( 0 );
	}

    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER );
	LoadLibrary();

	inputFullPath = argv[1];
	outputPath = argv[2];
	bool printDatabase = false;
	for( int i=3; i<argc; ++i ) {
		if ( StrEqual( argv[i], "-d" ) ) {
			printDatabase = true;
		}
	}

	GLString _inputFullPath( inputFullPath.c_str() ), _inputDirectory, name, extension;
	grinliz::StrSplitFilename( _inputFullPath, &_inputDirectory, &name, &extension );
	inputDirectory = _inputDirectory.c_str();

	const char* xmlfile = argv[2];
	printf( "Opening, path: '%s' filename: '%s'\n", inputDirectory.c_str(), inputFullPath.c_str() );
	
	// Test:
	
	{
		string testInput = inputDirectory + "Lenna.png";
		SDL_Surface* surface = libIMG_Load( testInput.c_str() );
		BTexture btexture;
		btexture.Create( surface->w, surface->h, BTexture::RGB16 );
		btexture.ToBuffer();

		if ( surface ) {
			// 444
			OrderedDitherTo16( surface, RGBA16, false, btexture.pixelBuffer16 );
			SDL_Surface* newSurf = SDL_CreateRGBSurfaceFrom(	btexture.pixelBuffer16, surface->w, surface->h, 16, surface->w*2,
																0xf000, 0x0f00, 0x00f0, 0 );
			string out = inputDirectory + "Lenna4440.bmp";
			SDL_SaveBMP( newSurf, out.c_str() );
			
			SDL_FreeSurface( newSurf );

			// 565
			OrderedDitherTo16( surface, RGB16, false, btexture.pixelBuffer16 );
			newSurf = SDL_CreateRGBSurfaceFrom(	btexture.pixelBuffer16, surface->w, surface->h, 16, surface->w*2,
												0xf800, 0x07e0, 0x001f, 0 );
			string out1 = inputDirectory + "Lenna565.bmp";
			SDL_SaveBMP( newSurf, out1.c_str() );

			SDL_FreeSurface( newSurf );

			// 565 Diffusion
			DiffusionDitherTo16( surface, RGB16, false, btexture.pixelBuffer16 );
			newSurf = SDL_CreateRGBSurfaceFrom(	btexture.pixelBuffer16, surface->w, surface->h, 16, surface->w*2,
												0xf800, 0x07e0, 0x001f, 0 );
			string out2 = inputDirectory + "Lenna565Diffuse.bmp";
			SDL_SaveBMP( newSurf, out2.c_str() );

			SDL_FreeSurface( newSurf );
			
			SDL_FreeSurface( surface );
		}
	}
	

	XMLDocument xmlDoc;
	xmlDoc.LoadFile( inputFullPath.c_str() );
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
		else if ( StrEqual( child->Value(), "atlas" )) {
			ProcessAtlas( child );
		}
		else {
			printf( "Unrecognized element: %s\n", child->Value() );
		}
	}

	int total = totalTextureMem + totalModelMem + totalDataMem;

	printf( "Total memory=%dk Texture=%dk Model=%dk Data=%dk\n", 
			total/1024, totalTextureMem/1024, totalModelMem/1024, totalDataMem/1024 );
	printf( "All done.\n" );
	SDL_Quit();

	writer->Save( outputPath.c_str() );
	delete writer;

	if ( printDatabase ) {
		gamedb::Reader reader;
		reader.Init( 0, outputPath.c_str(), 0 );
		reader.RecWalk( reader.Root(), 0 );
	}

	delete StringPool::Instance();
	return 0;
}