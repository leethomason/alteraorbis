#include "../grinliz/gldebug.h"
#include "../grinliz/glmatrix.h"
#include "../grinliz/glgeometry.h"

using namespace grinliz;

int main(int argc, const char* argv[])
{
	Matrix4::Test();
	Quaternion::Test();
	return 0;
}