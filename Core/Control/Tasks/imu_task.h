#include "icm42688p.h"
#include <stdbool.h>
//此任务用于得到gyro的raw数据，并进行滤波，量程转换操作
/*
gyro_raw (LSB)
  → gyro_notch
  → gyro_LPF
  → dynamic_LPF
  → RPM_Filter
  → gyro_filtered (LSB)
*/



bool gyro_raw_Flitter(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);

