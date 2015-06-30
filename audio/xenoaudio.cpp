#include "xenoaudio.h"
#include "../xegame/cgame.h"
#include "../grinliz/glgeometry.h"
#include "../libs/SDL2/include/SDL_mixer.h"
#include "../libs/SDL2/include/SDL.h"

using namespace grinliz;

XenoAudio* XenoAudio::instance = 0;

static const float MAX_DISTANCE = 20.0f;

XenoAudio::XenoAudio(const gamedb::Reader* db, const char* pathToDB)
{
	GLASSERT(!instance);
	instance = this;
	database = db;

//	Mix_Init(0);	// just need wav.
//	dataFP = SDL_RWFromFile(pathToDB, "rb");
//	GLASSERT(dataFP);
	sounds.PushArr(CHANNELS);
	listenerPos.Zero();
	listenerDir.Set(1, 0, 0);
}


XenoAudio::~XenoAudio()
{
	if (audioOn) {
		Mix_CloseAudio();
	}
	for (int i = 0; i < chunks.Size(); ++i) {
		Mix_FreeChunk(chunks.GetValue(i));
	}
	GLASSERT(instance == this);
	instance = 0;
//	SDL_RWclose(dataFP);
}


void XenoAudio::SetAudio(bool on) 
{
	if (on == audioOn) {
		return;
	}
	audioOn = on;
	if (audioOn) {
//		int error = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 2048);	// default, but laggy
		int error = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024);	// button response notably snappier at 1024
		GLASSERT(error == 0);
		int channels = Mix_AllocateChannels(CHANNELS);
		GLASSERT(channels == CHANNELS);
		(void)channels;
		(void)error;
	}
	else {
		Mix_CloseAudio();
	}
}

void XenoAudio::Play(const IString& iSound, const Vector3F* pos)
{
	if (!audioOn) return;

	// The listener will get updated at the end of the frame,
	// so this isn't quite correct. But hopefully good enough
	// to prevent saturating the game with sounds all
	// over the world.
	if (   pos 
		&& (*pos - listenerPos).LengthSquared() > (MAX_DISTANCE*MAX_DISTANCE)) 
	{
		return;
	}

	Mix_Chunk* chunk = 0;
	chunks.Query(iSound, &chunk);

	if (!chunk) {
		const gamedb::Item* data = database->Root()->Child("data");
		const gamedb::Item* item = data->Child(iSound.c_str());
		SDL_RWops* fp = 0;
		bool needClose = false;

#if 0
		// Search external path first.
		GLString path;
		GLString inPath = "res/";
		inPath.append(iSound.c_str());
		inPath.append(".wav");
		GetSystemPath(GAME_APP_DIR, inPath.c_str(), &path);

		fp = SDL_RWFromFile(path.c_str(), "rb");
		if (fp) {
			needClose = true;
		}
#endif
		// Now check the database
		if (!fp) {
			int size = 0;
			const void* mem = database->AccessData(item, "binary", &size);
			GLASSERT(mem);
			fp = SDL_RWFromConstMem(mem, size);
			needClose = true;
		}
		if (fp) {
			//U8* buf = 0;
			//U32 len = 0;
			chunk = Mix_LoadWAV_RW(fp, false);
			if (!chunk) {
				GLOUTPUT(("Audio error: %s\n", Mix_GetError()));
			}
			else {
				chunks.Add(iSound, chunk);
			}
			if (needClose) {
				SDL_RWclose(fp);
			}
		}
	}
	GLASSERT(chunk);
	if (chunk) {
		int channel = Mix_PlayChannel(-1, chunk, 0);
		if (channel >= 0 && channel < CHANNELS ) {
			sounds[channel].channel = channel;
			sounds[channel].pos.Zero();
			if (pos) {
				sounds[channel].pos = *pos;
			}
			SetChannelPos(channel);
		}
	}
}


void XenoAudio::SetListener(const grinliz::Vector3F& pos, const grinliz::Vector3F& dir)
{
	listenerPos = pos;
	listenerDir = dir;
	if (listenerDir.Length())
		listenerDir.Normalize();
	else
		listenerDir.Set(1, 0, 0);
	for (int i = 0; i < CHANNELS; ++i) {
		SetChannelPos(i);
	}
}


void XenoAudio::SetChannelPos(int i)
{
	if (!audioOn) return;

	if (sounds[i].pos.IsZero()) {
		Mix_SetPosition(i, 0, 0);
	}
	else {
		Vector3F delta = sounds[i].pos - listenerPos;
		float len = delta.Length();
		float df = len / MAX_DISTANCE;
		int d = LRintf(df*255.0f);
		d = Clamp(d, 0, 255);

		if (delta.LengthSquared() > 0.001f) {
			delta.Normalize();
		}
		else {
			delta.Set(0, 0, -1);
		}

		static const Vector3F UP = { 0, 1, 0 };
		Vector3F listenerRight = CrossProduct(listenerDir, UP);

		float dotFront = DotProduct(delta, listenerDir);
		float dotRight = DotProduct(delta, listenerRight);

		// 0 is north, 90 is east, etc. WTF.
		float rad = atan2(dotRight, dotFront);
		float deg = rad * 180.0f / PI;
		int degi = int(deg);
		if (degi < 0) degi += 360;

		int result = Mix_SetPosition(i, degi, d);
		GLASSERT(result != 0);
		(void)result;
	}
}


void XenoAudio::PlayVariation(const grinliz::IString& base, int seed, const grinliz::Vector3F* pos)
{
	if (!variations.Query(base, 0)) {
		const gamedb::Item* data = database->Root()->Child("data");
		const gamedb::Item* item = 0;

		SoundVariation sv;
		sv.variation[0] = base;

		// Go look for variations.
		// Always create 4 variations, even if 
		// some (all) are copies of the base.
		for (int i = 2; i < 5; ++i) {
			glString.Format("%s%d", base.safe_str(), i);
			item = data->Child(glString.c_str());
			if (item)
				sv.variation[i-1] = StringPool::Intern(glString.c_str());
			else
				sv.variation[i-1] = base;
		}
		variations.Add(base, sv);
	}
	SoundVariation sv;
	bool okay = variations.Query(base, &sv);
	GLASSERT(okay);
	if (okay) {
		Play(sv.variation[seed & 3], pos);
	}
}
