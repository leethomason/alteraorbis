#ifndef PLATFORM_PATH_INCLUDED
#define PLATFORM_PATH_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

	enum {
	GAME_APP_DIR,
	GAME_SAVE_DIR,
};

namespace grinliz {
	class GLString;
};

// Get a system path. The directory will be created, if
// needed, but the path is a standard fopen(). And
// hopefully fopen() is portable enough. I'm sure I'll
// revisit this for the 50th time.
void GetSystemPath(int root, const char* filename, grinliz::GLString* out);

#ifdef __cplusplus
}
#endif
#endif // PLATFORM_PATH_INCLUDED
