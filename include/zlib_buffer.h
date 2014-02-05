#ifndef _ZLIB_H_
#define _ZLIB_H_
// C-StdLib
#include <stdio.h>
// zlib
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

class Zlib {
public:
	// Adapted deflate and inflate methods to compress a buffer.
	// See http://www.zlib.net/zlib_how.html for more info.

	static int def(char* buffer, int size, FILE* dest, int level);
	static int inf(FILE* source, char* buffer, int size);
	static void zerr(int ret);
};
#endif // _ZLIB_H_