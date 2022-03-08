
#ifndef LNSOCKET_COMMON_H
#define LNSOCKET_COMMON_H

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
	#define EXPORT
#endif

#endif
