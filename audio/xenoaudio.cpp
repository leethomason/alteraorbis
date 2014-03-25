#include "xenoaudio.h"

#include "../libs/SDL2/include/SDL_mixer.h"
#include "../libs/SDL2/include/SDL.h"

using namespace grinliz;

XenoAudio* XenoAudio::instance = 0;

XenoAudio::XenoAudio(const gamedb::Reader* db, const char* pathToDB)
{
	GLASSERT(!instance);
	instance = this;
	database = db;

	
//	Mix_Init(0);	// just need wave.

	fp = SDL_RWFromFile(pathToDB, "rb");
	GLASSERT(fp);
}


XenoAudio::~XenoAudio()
{
	if (audioOn) {
		Mix_CloseAudio();
	}
//	Mix_Quit();
	GLASSERT(instance == this);
	instance = 0;
	SDL_RWclose(fp);
}


void XenoAudio::SetAudio(bool on) 
{
	if (on == audioOn) {
		return;
	}
	audioOn = on;
	if (audioOn) {
		int error = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);
		GLASSERT(error == 0);
	}
	else {
		Mix_CloseAudio();
	}
}

void XenoAudio::Play(const char* _sound)
{
	if (!audioOn) return;

	IString iSound = StringPool::Intern(_sound);
	const char* sound = iSound.c_str();

	Mix_Chunk* chunk = 0;
	chunks.Query(sound, &chunk);

	if (!chunk) {
		const gamedb::Item* data = database->Root()->Child("data");
		const gamedb::Item* item = data->Child(sound);
		GLASSERT(item);
		if (item) {
			int offset = 0, size = 0;
			bool compressed = false;

			item->GetDataInfo("binary", &offset, &size, &compressed);
			GLASSERT(compressed == false);
			SDL_RWseek(fp, offset, RW_SEEK_SET);

			SDL_AudioSpec spec;
			U8* buf = 0;
			U32 len = 0;
			chunk = Mix_LoadWAV_RW(fp, false);
			if (!chunk) {
				GLOUTPUT(("Audio error: %s\n", Mix_GetError()));
			}
			else {
				chunks.Add(sound, chunk);
			}
		}
	}
	GLASSERT(chunk);
	if (chunk) {
		Mix_PlayChannel(-1, chunk, 0);
	}
}
