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

#ifndef UFO_SETTINGS_INCLUDED
#define UFO_SETTINGS_INCLUDED

#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glstringutil.h"
#include "../tinyxml2/tinyxml2.h"
#include <stdio.h>

class TiXmlElement;


class SettingsManager
{
public:
	static SettingsManager* Instance()	{ GLASSERT( instance );	return instance; }

	static void Create( const char* savepath );
	static void Destroy();

	bool AudioOn() const				{ return audioOn != 0; }
	void SetAudioOn( bool value );
	bool DebugGLCalls() const			{ return debugGLCalls; }
	bool DebugUI() const				{ return debugUI;  }
	float SpawnDate() const				{ return spawnDate; }
	float WorldGenDone() const			{ return worldGenDone; }

protected:
	SettingsManager( const char* path );
	virtual ~SettingsManager()	{ instance = 0; }
	
	void Load();
	void Save();

	virtual void ReadAttributes( const tinyxml2::XMLElement* element );

private:
	static SettingsManager* instance;

	bool audioOn;
	bool debugGLCalls;
	bool debugUI;
	float spawnDate;	// would like to be 0.20, but in interest of performance, 0.90 is used. On a debug 0.98 may be correct.
	float worldGenDone;
	grinliz::GLString path;
};


#endif // UFO_SETTINGS_INCLUDED