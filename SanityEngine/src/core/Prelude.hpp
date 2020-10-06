#pragma once

#ifdef SANITY_EXPORT_API
#define SANITY_API __declspec(dllexport)
#else
#define SANITY_API __declspec(dllimport)
#endif
