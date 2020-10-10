#ifndef RX_CORE_PRELUDE_H
#define RX_CORE_PRELUDE_H

#ifdef RX_EXPORT_API
#define RX_API __declspec(dllexport)
#else
#define RX_API __declspec(dllimport)
#endif

#endif
