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

#ifndef UFOATTACK_TEXT_INCLUDED
#define UFOATTACK_TEXT_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glvector.h"
#include "screenport.h"
#include "texture.h"
#include "vertex.h"
#include "shadermanager.h"

class GPUState;
class GPUVertexBuffer;

class UFOText : public IDeviceLossHandler
{
public:
	static UFOText* Instance() 	{ GLASSERT( instance ); return instance; }

	// Queues drawing, actually. Need to call CommitDraw() to send to screen.
	void Draw( int x, int y, const char* format, ... );
	void FinalDraw();
	virtual void DeviceLoss();

	static void Create(Texture* texture);
	static void Destroy();

private:
	UFOText(Texture* texture);
	virtual ~UFOText();

	void TextOut( const char* str, int x, int y, int h, int *w );

	static UFOText* instance;
	
	Texture* texture;
	GPUVertexBuffer* vbo;

	enum {
		VBO_MEM		= 128*1024,
		CHAR_OFFSET = 32,
		CHAR_RANGE  = 128 - CHAR_OFFSET,
		END_CHAR = CHAR_OFFSET + CHAR_RANGE
	};

	grinliz::CDynArray< PTVertex2 > quadBuf;
};

#endif // UFOATTACK_TEXT_INCLUDED