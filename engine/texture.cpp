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

#include "texture.h"
#include "platformgl.h"
#include "surface.h"

#include "../grinliz/glstringutil.h"
using namespace grinliz;


/*static*/ void TextureManager::Create( const gamedb::Reader* reader )
{
	GLASSERT( instance == 0 );
	instance = new TextureManager( reader );
}


/*static*/ void TextureManager::Destroy()
{
	GLASSERT( instance );
	delete instance;
	instance = 0;
}


TextureManager::TextureManager( const gamedb::Reader* reader )
{
	cacheHit = 0;
	cacheReuse = 0;
	cacheMiss = 0;
	emptySpace = 0;
	database = reader;
}


TextureManager::~TextureManager()
{
	DeviceLoss();
}


void TextureManager::DeviceLoss()
{
	for( int i=0; i<textureArr.Size(); ++i ) {
		if ( textureArr[i].glID ) {
			glDeleteTextures( 1, (const GLuint*) &textureArr[i].glID );
			textureArr[i].glID = 0;
		}
	}
}


void TextureManager::Reload()
{
	DeviceLoss();

	for ( int i=0; i<textureArr.Size(); ++i ) {
		Texture* tex = &textureArr[i];
		if ( tex->creator == 0 ) {
			if ( tex->Name() ) {
				GetTexture( tex->Name(), true );
			}
		}
	}
}


/*static*/ TextureManager* TextureManager::instance = 0;


bool TextureManager::IsTexture( const char* name )
{
	const gamedb::Item* item = database->Root()->Child( "textures" )->Child( name );
	if ( item != 0 )
		return true;

	return texMap.Query( name, 0 );
}


Texture* TextureManager::GetTexture( const char* name, bool reload )
{
	// First check for an existing texture, or one
	// that was added. Failing that, check the database.
	// The texture may not be allocated on the GPU - that
	// happens when the GLID is hit.
	Texture* t = 0;
	texMap.Query( name, &t );

	if ( !t || reload ) {
		const gamedb::Item* item = database->Root()->Child( "textures" )->Child( name );
#ifdef DEBUG
		if ( !item ) 
			GLOUTPUT(( "GetTexture '%s' failed.\n", name ));
		GLASSERT( item );
#endif
		
		if ( item ) {
			// Found the item. Generate a new texture.
			int w = item->GetInt( "width" );
			int h = item->GetInt( "height" );
			const char* fstr = item->GetString( "format" );
			TextureType format = Surface::QueryFormat( fstr );
			int flags = Texture::PARAM_NONE;

			if ( item->GetBool( "noMip" )) {
				flags = Texture::PARAM_LINEAR;
			}
			if ( item->GetBool( "colorMap" )) {
				flags |= Texture::PARAM_COLORMAP;
			}
			if ( item->GetBool( "emissive" )) {
				flags |= Texture::PARAM_EMISSIVE;
			}

			if ( reload ) {
				GLASSERT( t );
			}
			else {
				t = textureArr.PushArr(1);
			}
			t->Set( name, w, h, format, flags );
			t->item = item;

			if ( !reload ) {
				GLASSERT( !texMap.Query( t->Name(), 0 ));
				texMap.Add( t->Name(), t );
			}
		}
	}
	return t;
}


void TextureManager::TextureCreatorInvalid( ITextureCreator* creator )
{
	for( int i=0; i<textureArr.Size(); ++i ) {
		if ( textureArr[i].creator == creator ) {
			textureArr[i].creator = 0;
		}
	}
}


Texture* TextureManager::CreateTexture( const char* name, int w, int h, TextureType format, int flags, ITextureCreator* creator )
{
	GLASSERT( w > 1 );	// some drivers don't like size=1 textures
	GLASSERT( h > 1 );

	Texture* t = 0;
	if ( texMap.Query( name, &t ) ) {
		GLASSERT( t->creator == 0 || (t->creator && creator) );
		t->Set( name, w, h, format, flags );
		t->creator = creator;
	}
	else {
		GLASSERT( emptySpace >= 0 );
		if ( emptySpace > 0 ) {
			for( int i=0; i<textureArr.Size(); ++i ) {
				if ( textureArr[i].Empty() ) {
					t = &textureArr[i];
					--emptySpace;
					break;
				}
			}
		}
		else {
			t = textureArr.PushArr(1);
		}
		GLASSERT( t );

		t->Set( name, w, h, format, flags );
		t->creator = creator;
		GLASSERT( !texMap.Query( t->Name(), 0 ));
		texMap.Add( t->Name(), t );
	}
	return t;
}


void TextureManager::ContextShift()
{
//	DeviceLoss();
}


void Texture::Set(const char* p_name, int p_w, int p_h, TextureType p_format, int p_flags)
{
	name = p_name;
	w = p_w;
	h = p_h;
	format = p_format;
	flags = p_flags;
	creator = 0;
	item = 0;
	glID = 0;
}


U32 Texture::GLID() 
{
	if ( glID ) 
		return glID;
	
	TextureManager* manager = TextureManager::Instance();
	GLASSERT( manager );
	glID = manager->CreateGLTexture( w, h, format, flags );
	GLASSERT( glID );

	// Need to actually generate the texture. Either pull it from 
	// the database, or call the ICreator to push it to the GPU.
	if ( item ) {
		const gamedb::Reader* database = gamedb::Reader::GetContext( item );
		GLASSERT( database );
		GLASSERT( database->ItemInReader( item ));
		GLASSERT( item->HasAttribute( "pixels" ) );
		int size;
		const void* pixels = database->AccessData( item, "pixels", &size );
		Upload( pixels, size );
	}
	else if ( creator ) {
		creator->CreateTexture( this );
	}
	else {
		GLOUTPUT(( "No texture creator for '%s'\n", this->name.c_str() ));
		GLASSERT( 0 );
	}
	return glID;
}


void TextureManager::CalcOpenGL( int format, int* glFormat, int* glType )
{
	*glFormat = GL_RGB;
	*glType = GL_UNSIGNED_BYTE;

	switch( format ) {
		case TEX_RGBA16:
			*glFormat = GL_RGBA;
			*glType = GL_UNSIGNED_SHORT_4_4_4_4;
			break;

		case TEX_RGB16:
			*glFormat = GL_RGB;
			*glType = GL_UNSIGNED_SHORT_5_6_5;
			break;

		case TEX_RGBA32:
			*glFormat = GL_RGBA;
			*glType = GL_UNSIGNED_BYTE;
			break;

		case TEX_RGB24:
			*glFormat = GL_RGB;
			*glType = GL_UNSIGNED_BYTE;
			break;

			/*
		case TEX_ALPHA:
			// GL_ALPHA not supported in some contexts. Is in others.
			// Oh GL, you have become unusable. 
			// http://stackoverflow.com/questions/9355869/invalid-enumerant-when-creating-16-bit-texture
			*glFormat = GL_ALPHA;
			*glType = GL_UNSIGNED_BYTE;
			break;
			*/

		default:
			GLASSERT( 0 );
	}
}

// Hating intel drivers right now. Not very happy with the GL3 vs GL3.2 vs. ES2 spec either.
#define GEN_MIP			// unreliable on the "3000" driver. Just started working with update. Yay!
//#define PARAM_MIP		// works fine on the "3000" driver, doesn't upload correctly on the "family" driver

U32 TextureManager::CreateGLTexture( int w, int h, int format, int flags )
{
	int glFormat, glType;
	CalcOpenGL( format, &glFormat, &glType );

	GLASSERT( w && h && (format >= 0) );
	CHECK_GL_ERROR;

	GLuint texID;
	glGenTextures( 1, &texID );
	glBindTexture( GL_TEXTURE_2D, texID );

	if ( flags & Texture::PARAM_NEAREST ) {
#ifdef PARAM_MIP
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_TRUE );
#endif

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_NEAREST );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_NEAREST );
	}
	else if ( flags & Texture::PARAM_LINEAR ) {
#ifdef PARAM_MIP
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_FALSE );
#endif

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_LINEAR );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_LINEAR );
	}
	else {
#ifdef PARAM_MIP
		glTexParameteri(	GL_TEXTURE_2D,
							GL_GENERATE_MIPMAP,
							GL_TRUE );
#endif

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MAG_FILTER,
							GL_LINEAR );

		glTexParameteri(	GL_TEXTURE_2D,
							GL_TEXTURE_MIN_FILTER,
							GL_LINEAR_MIPMAP_NEAREST );
	}

	//GLOUTPUT(( "OpenGL texture %d created.\n", texID ));				
	CHECK_GL_ERROR;

	return texID;
}


void Texture::Upload( const void* pixels, int size )
{
	GLASSERT( pixels );
	GLASSERT( size == BytesInImage() );

	if ( glID == 0 ) {
		// Make sure we have on OpenGL ID
		TextureManager* manager = TextureManager::Instance();
		glID = manager->CreateGLTexture( w, h, format, flags );
		GLASSERT( glID );
		//this->GLID();
	}

	int glFormat, glType;
	TextureManager::Instance()->CalcOpenGL( format, &glFormat, &glType );
	glBindTexture( GL_TEXTURE_2D, glID );

#if defined( UFO_WIN32_SDL ) && defined( DEBUG )
	int data;
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &data );
	GLASSERT( data == 0 || data == w );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &data );
	GLASSERT( data == 0 || data == h );
#endif

	glTexImage2D(	GL_TEXTURE_2D,
					0,
					glFormat,
					w,
					h,
					0,
					glFormat,
					glType,
					pixels );
//	GLOUTPUT(( "OpenGL texture %d Upload.\n", gpuMem->glID ));
	CHECK_GL_ERROR;

	if ( !(flags & PARAM_LINEAR) ) {
		if (!(flags & PARAM_SOFTWARE_MIP)) {
#ifdef GEN_MIP
			glGenerateMipmap(GL_TEXTURE_2D);
#endif
		}
		else {
			Surface s;
			s.Set( format, w, h );
			memcpy( s.Pixels(), pixels, s.BytesInImage() );

			int level=0;
			while ( s.Width() > 1 && s.Height() > 1 ) {
				++level;
				s.ScaleByHalf();
				glTexImage2D(	GL_TEXTURE_2D,
								level,
								glFormat,
								s.Width(),
								s.Height(),
								0,
								glFormat,
								glType,
								s.Pixels() );
			}
			glTexParameteri(	GL_TEXTURE_2D,
								GL_TEXTURE_MAX_LEVEL,
								level );
		}
	}
	CHECK_GL_ERROR;
}


void Texture::Upload( const Surface& surface )
{
	Upload( surface.Pixels(), surface.BytesInImage() );
}


U32 TextureManager::CalcTextureMem() const
{
	U32 mem = 0;
	for( int i=0; i<textureArr.Size(); ++i ) {
		mem += textureArr[i].BytesInImage();
	}
	return mem;
}


int Texture::NumTableEntries() const
{
	if ( item ) {
		const gamedb::Item* table = item->Child( "table" );
		if ( table ) {
			return table->NumChildren();
		}
	}
	return 0;
}


void Texture::GetTableEntry( int i, TableEntry* te ) const
{
	te->name = IString();
	GLASSERT( item );
	if ( item ) {
		const gamedb::Item* table = item->Child( "table" );
		GLASSERT( table );
		if ( table ) {
			const gamedb::Item* child = table->ChildAt( i );
			GetTE( child, te );
		}
	}
}


bool Texture::HasTableEntry(const char* name) const
{
	if (item) {
		const gamedb::Item* table = item->Child("table");
		if (table) {
			return table->Child(name) != 0;
		}
	}
	return false;
}


void Texture::GetTableEntry( const char* name, TableEntry* te ) const
{
	te->name = IString();
	if ( item ) {
		const gamedb::Item* table = item->Child( "table" );
		if ( table ) {
			const gamedb::Item* child = table->Child( name );
			GLASSERT( child );
			GetTE( child, te );
		}
	}
}


void Texture::GetTE( const gamedb::Item* child, TableEntry* te ) const
{
	te->name = StringPool::Intern( child->Name() );

	float x = child->GetFloat( "x" );
	float y = child->GetFloat( "y" );
	float w = child->GetFloat( "w" );
	float h = child->GetFloat( "h" );
	float oX = child->GetFloat( "oX" );
	float oY = child->GetFloat( "oY" );
	float oW = child->GetFloat( "oW" );
	float oH = child->GetFloat( "oH" );

	float x0 = x-oX;
	float y0 = 1.0f - (y-oY+oH);	// stupid opengl inverted coordinates.
	float x1 = x-oX+oW;
	float y1 = 1.0f - (y-oY);

	te->uv.Set( x0, y0, x1, y1 );
	te->clip.Set( x, 1.0f - (y+h), x+w, 1.0f - y );

	float a = x1-x0;
	float d = y1-y0;
	float tx = x0;
	float ty = y0;

	te->uvXForm.Set( a, d, tx, ty );
}
