#pragma once

#ifndef RTWEEKEND_STB_IMAGE_H
#define RTWEEKEND_STB_IMAGE_H

// Disable pedantic warnings for this external library.
#ifdef _MSC_VER
	// Microsoft Visual C++ Compiler
	#pragma warning (push, 0)
#endif // _MSC_VER

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

// Restore warning levels.
#ifdef _MSC_VER
	// Microsoft Visual C++ Compiler
	#pragma warning (pop)
#endif // _MSC_VER

#endif // !RTWEEKEND_STB_IMAGE_H
