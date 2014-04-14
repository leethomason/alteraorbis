#ifndef XENO_AUDIO_INCLUDED
#define XENO_AUDIO_INCLUDED

#include "../grinliz/glstringutil.h"
#include "../grinliz/glcontainer.h"
#include "../grinliz/glvector.h"
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

	void Play(const char* sound, const grinliz::Vector3F* pos);
	void SetListener(const grinliz::Vector3F& pos, const grinliz::Vector3F& dir);

private:
	static XenoAudio* instance;
	void SetChannelPos(int i);

	enum {
		CHANNELS = 8		// if this is changed, the default # of channels needs to be set in Mix
	};

	struct Sound {
		Sound() : channel(0) { pos.Zero(); }

		int					channel;
		grinliz::Vector3F	pos;		// zero if not positional
	};

	const gamedb::Reader* database;
	SDL_RWops* dataFP;
	bool audioOn;
	grinliz::Vector3F listenerPos;
	grinliz::Vector3F listenerDir;

	// IStrings! the char* is unique
	grinliz::HashTable< const char*, Mix_Chunk* > chunks;
	grinliz::CArray< Sound, CHANNELS > sounds;
};

#endif // XENO_AUDIO_INCLUDED
