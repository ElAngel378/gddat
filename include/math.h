#include <tonc.h>
#include "memory.h"

s32 snap_to_tan_theta_1_2(s32 current_rotation);
s32 snap_to_tan_theta_1_2_rotated_90(s32 current_rotation);
s32 snap_to_45(s32 current_rotation);

u64 approach_value_asymptotic(u64 current, u64 target, u32 multiplier, u32 max_adjustment);
u64 approach_value(u64 current, u64 target, s32 inc, s32 dec);
s16 lerp_angle(s16 current, s16 target, FIXED divisor, u8 cap_angle);
