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

	void SetAudioOn( bool value );

	bool AudioOn() const;
	float SpawnDate() const;
	float WorldGenDone() const;

	bool DebugGLCalls() const;
	bool DebugUI() const;
	bool DebugFPS() const;

protected:
	SettingsManager( const char* path );
	virtual ~SettingsManager()	{ instance = 0; }
	
	template<typename T>
	T Read(const char* parent, const char* name, T defaultValue) const {
		T value = defaultValue;
		const tinyxml2::XMLElement* ele = tinyxml2::XMLConstHandle(xmlDoc)
			.FirstChildElement("Settings")
			.FirstChildElement(parent)
			.ToElement();
		if (ele) {
			ele->QueryAttribute(name, &value);
		}
		return value;
	}

private:
	static SettingsManager* instance;
	tinyxml2::XMLDocument xmlDoc;
	grinliz::GLString path;
};


#endif // UFO_SETTINGS_INCLUDED