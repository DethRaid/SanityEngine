#ifndef RX_CORE_PP_H
#define RX_CORE_PP_H

#define RX_PP_PASTE(_a, _b) \
  _a ## _b

#define RX_PP_CAT(_a, _b) \
  RX_PP_PASTE(_a, _b)

#define RX_PP_UNIQUE(_base) \
  RX_PP_CAT(_base, __LINE__)

#endif // RX_CORE_PP_H
