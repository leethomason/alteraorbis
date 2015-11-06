#include "platformpath.h"
#include "../grinliz/glstringutil.h"

#if defined( UFO_WIN32_SDL )
#include "../libs/SDL2/include/SDL_filesystem.h"
#include <Shlobj.h>
static const char* winResourcePath = "./res/lumos.db";
#endif


static const int PREF_PATH_SIZE = 300;
static char prefPath[PREF_PATH_SIZE] = { 0 };

void GetSystemPath(int root, const char* filename, grinliz::GLString* out)
{
	grinliz::GLString path;
	*out = "";

	if (root == GAME_SAVE_DIR) {
		if (!prefPath[0]) {
#if defined(UFO_WIN32_SDL)
			// The SDL path has a couple of problems on Windows.
			//	1. It doesn't work. The code doesn't actually create the
			//	   directory even though it returns without an error.
			//	   I haven't run the code through a debugger to see why.
			//  2. It creates it in AppData/Roaming. While that makes 
			//	   some sense - it is a standard - it's also a hidden
			//	   directory which is awkward and weird.
			PWSTR pwstr = 0;
			PWSTR append = L"\\AlteraOrbis";
			WCHAR buffer[MAX_PATH];

			SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pwstr);
			PWSTR p = pwstr;
			WCHAR* q = buffer;
			while (*p && q < (buffer+MAX_PATH-1)) {
				*q++ = *p++;
			}
			p = append;
			while (*p && q < (buffer + MAX_PATH - 1)) {
				*q++ = *p++;
			}
			*q = 0;
			CreateDirectory(buffer, NULL);

			int n = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, prefPath, PREF_PATH_SIZE, 0, 0);
			prefPath[255] = 0;
			if (n && n < 255) {
				prefPath[n-1] = '\\';
				prefPath[n] = 0;
			}
			CoTaskMemFree(static_cast<void*>(pwstr));
#elif defined(UFO_LINUX_SDL)
			mkdir("save", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			strcpy(prefPath, "save/");
#else
	#error Platform undefined.
#endif
		}
		out->append(prefPath);
	}
	out->append(filename);
}
