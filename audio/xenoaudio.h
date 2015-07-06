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

	void PlayVariation(const grinliz::IString& sound, int seed, const grinliz::Vector3F* pos);
	void Play(const grinliz::IString& sound, const grinliz::Vector3F* pos);
	void SetListener(const grinliz::Vector3F& pos, const grinliz::Vector3F& dir);

private:
	static XenoAudio* instance;
	void SetChannelPos(int i);

	enum {
		CHANNELS = 16
	};

	struct Sound {
		Sound() : channel(0) { pos.Zero(); }

		int					channel;
		grinliz::Vector3F	pos;		// zero if not positional
	};

	const gamedb::Reader* database;
	bool audioOn = false;
	grinliz::Vector3F listenerPos;
	grinliz::Vector3F listenerDir;
	grinliz::GLString glString;		// don't want to re-allocate.

	struct SoundVariation
	{
//		grinliz::IString name;
		enum { NUM_VARIATIONS = 4 };
		grinliz::IString variation[NUM_VARIATIONS];
	};

	grinliz::HashTable< grinliz::IString, Mix_Chunk*, grinliz::CompValueString > chunks;
	grinliz::HashTable< grinliz::IString, SoundVariation, grinliz::CompValueString > variations;
	grinliz::CArray< Sound, CHANNELS > sounds;
};

#endif // XENO_AUDIO_INCLUDED
