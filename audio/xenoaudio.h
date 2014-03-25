#ifndef XENO_AUDIO_INCLUDED
#define XENO_AUDIO_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../shared/gamedbreader.h"

struct SDL_RWops;
struct Mix_Chunk;

/*
	Wrap up the hardware sound.

	Current implementation: the 'data' segment of the database is
	left on-disc, not in-memory. This keeps a stream to the on-disc
	database and uses it. May want to consider a 'data-memory'
	segment that does get loaded in the future.
*/
class XenoAudio
{
public:
	XenoAudio(const gamedb::Reader* db, const char* pathToDB);
	~XenoAudio();

	void SetAudio(bool on);
	bool IsAudioOn() const { return audioOn;  }

	static XenoAudio* Instance()	{ GLASSERT(instance); return instance; }

	void Play(const char* sound);

private:
	static XenoAudio* instance;

	/*
	struct Sound {
		Sound() : start(0), len(0), pos(0) {}

		grinliz::IString sound;
		const void* start;
		int len;
		int pos;
	};
	grinliz::CDynArray< Sounds > inQueue, playing;
	*/
	const gamedb::Reader* database;
	SDL_RWops* fp;
	bool audioOn;

	// IStrings! the char* is unique
	grinliz::HashTable< const char*, Mix_Chunk* > chunks;
};

#endif // XENO_AUDIO_INCLUDED
