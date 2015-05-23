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

#include "settings.h"
#include "../tinyxml2/tinyxml2.h"
#include "../engine/serialize.h"
#include "../xegame/cgame.h"

using namespace grinliz;
using namespace tinyxml2;

SettingsManager* SettingsManager::instance = 0;

void SettingsManager::Create( const char* savepath )
{
	GLASSERT( instance == 0 );
	instance = new SettingsManager( savepath );
	SetCheckGLError(instance->DebugGLCalls());
	GLOUTPUT_REL(("Checking GL Errors=%s\n", instance->DebugGLCalls() ? "true" : "false"));
}


void SettingsManager::Destroy()
{
	delete instance;
}


SettingsManager::SettingsManager( const char* savepath )
{
	GLOUTPUT_REL(("SettingsManager::SettingsManager opening '%s'\n", savepath));
	GLASSERT( instance == 0 );
	instance = this;
	path = savepath;

	xmlDoc.LoadFile(path.safe_str());
	if (xmlDoc.Error()) {
		const char* str0 = xmlDoc.GetErrorStr1();
		const char* str1 = xmlDoc.GetErrorStr2();
		GLOUTPUT_REL(("XML error: %s  %s\n", str0, str1));

		xmlDoc.Clear();

		tinyxml2::XMLNode* ele = xmlDoc.InsertEndChild(xmlDoc.NewElement("Settings"));
		ele->InsertEndChild(xmlDoc.NewElement("Game"));
		ele->InsertEndChild(xmlDoc.NewElement("Debug"));
	}
}


void SettingsManager::SetAudioOn( bool value )
{
	if ( AudioOn() != value ) {
		XMLElement* ele = XMLHandle(xmlDoc).FirstChildElement("Settings").FirstChildElement("Game").ToElement();
		if (ele) {
			ele->SetAttribute("audioOn", value);
			xmlDoc.SaveFile(path.safe_str());
		}
	}
}

void SettingsManager::SetTouchOn( bool value )
{
	if ( TouchOn() != value ) {
		XMLElement* ele = XMLHandle(xmlDoc).FirstChildElement("Settings").FirstChildElement("Game").ToElement();
		if (ele) {
			ele->SetAttribute("touchOn", value);
			xmlDoc.SaveFile(path.safe_str());
		}
	}
}


bool SettingsManager::AudioOn() const
{
	return Read("Game", "audioOn", true);
}

bool SettingsManager::TouchOn() const
{
	return Read("Game", "touchOn", false);
}

float SettingsManager::DenizenDate() const
{
	return Read("Game", "spawDate", 0.2f);
}

float SettingsManager::SpawnDate() const
{
	return Read("Game", "spawDate", 0.5f);
}

float SettingsManager::WorldGenDone() const
{
	return Read("Game", "worldGenDone", 1.0f);
}


bool SettingsManager::DebugGLCalls() const
{
	return Read("Debug", "debugGLCalls", false);
}

bool SettingsManager::DebugUI() const
{
	return Read("Debug", "debugUI", false);
}

bool SettingsManager::DebugFPS() const
{
	return Read("Debug", "debugFPS", false);
}
