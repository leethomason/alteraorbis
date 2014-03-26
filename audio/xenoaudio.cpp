#include "xenoaudio.h"
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
	fp = SDL_RWFromFile(pathToDB, "rb");
	GLASSERT(fp);
	sounds.PushArr(CHANNELS);
	listenerPos.Zero();
	listenerDir.Set(1, 0, 0);
}


XenoAudio::~XenoAudio()
{
	if (audioOn) {
		Mix_CloseAudio();
	}
	for (int i = 0; i < chunks.NumValues(); ++i) {
		Mix_FreeChunk(chunks.GetValue(i));
	}
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

void XenoAudio::Play(const char* _sound, const Vector3F* pos)
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
	listenerDir.Normalize();
	for (int i = 0; i < CHANNELS; ++i) {
		SetChannelPos(i);
	}
}


void XenoAudio::SetChannelPos(int i)
{
#ifdef DEBUG
	if (Mix_Playing(i)) {
		int debug = 1;
	}
#endif

	if (sounds[i].pos.IsZero()) {
		Mix_SetPosition(i, 0, 0);
	}
	else {
		Vector3F delta = sounds[i].pos - listenerPos;
		float len = delta.Length();
		float df = len / MAX_DISTANCE;
		int d = LRintf(df*255.0f);
		d = Clamp(d, 0, 255);

		delta.Normalize();
		float dotFront = DotProduct(delta, listenerDir);

		static const Vector3F UP = { 0, 1, 0 };
		Vector3F right;
		CrossProduct(UP, listenerDir, &right);

		float dotRight = DotProduct(delta, right);

		// 0 is north, 90 is east, etc. WTF.
		float rad = atan2(dotRight, dotFront);
		float deg = rad * 180.0f / PI;
		int degi = int(deg);
		if (degi < 0) degi += 360;

		int result = Mix_SetPosition(i, degi, d);
		GLASSERT(result != 0);
	}
}