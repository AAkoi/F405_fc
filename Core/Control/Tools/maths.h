#include "math.h"

#define M_PIf       3.14159265358979323846f
#define RAD    (M_PIf / 180.0f)

#ifndef MIN
#define MIN(a,b) \
  __extension__ ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a < _b ? _a : _b; })
#endif

#ifndef MAX
#define MAX(a,b) \
  __extension__ ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })
#endif

float sin_approx(float x);
float cos_approx(float x);
float atan2_approx(float y, float x);

