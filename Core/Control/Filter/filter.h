/* These codes come from Betaflight */
#include <stdint.h>
#include "maths.h"

typedef struct pt1Filter_s {
    float state;
    float k;
} pt1Filter_t;

typedef enum {
    FILTER_LPF,    // 2nd order Butterworth section
    FILTER_NOTCH,
    FILTER_BPF,
} biquadFilterType_e;


/* this holds the data required to update samples thru a filter */
typedef struct biquadFilter_s {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
    float weight;
} biquadFilter_t;

float pt1FilterGain(float f_cut, float dT);
float pt1FilterGainFromDelay(float delay, float dT);
void pt1FilterInit(pt1Filter_t *filter, float k);
void pt1FilterUpdateCutoff(pt1Filter_t *filter, float k);
float pt1FilterApply(pt1Filter_t *filter, float input);

float filterGetNotchQ(float centerFreq, float cutoffFreq);

void biquadFilterInitLPF(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate);
void biquadFilterInit(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate, float Q, biquadFilterType_e filterType, float weight);
void biquadFilterUpdate(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate, float Q, biquadFilterType_e filterType, float weight);
void biquadFilterUpdateLPF(biquadFilter_t *filter, float filterFreq, uint32_t refreshRate);
float biquadFilterApplyDF1(biquadFilter_t *filter, float input);
float biquadFilterApplyDF1Weighted(biquadFilter_t *filter, float input);
float biquadFilterApply(biquadFilter_t *filter, float input);
