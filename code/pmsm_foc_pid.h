#ifndef PMSM_FOC_PID_H
#define PMSM_FOC_PID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;
    float ts;

    float integral;
    float prev_error;
    float prev_derivative;

    float out_min;
    float out_max;
    float integral_min;
    float integral_max;
    float derivative_lpf_alpha;
} PID_Controller;

typedef struct {
    float alpha;
    float beta;
} AlphaBeta;

typedef struct {
    float d;
    float q;
} DQ;

typedef struct {
    float duty_a;
    float duty_b;
    float duty_c;
} PWM_Duty;

typedef struct {
    uint8_t pole_pairs;
    float vbus;
    float id_ref;
    float iq_ref;
    float speed_ref_rad_s;
    float speed_fdb_rad_s;

    PID_Controller id_pi;
    PID_Controller iq_pi;
    PID_Controller speed_pi;
} PMSM_FOC;

void pid_init(PID_Controller *pid,
              float kp,
              float ki,
              float kd,
              float ts,
              float out_min,
              float out_max,
              float integral_min,
              float integral_max);

void pid_reset(PID_Controller *pid);

float pid_update(PID_Controller *pid, float ref, float fdb);

AlphaBeta clarke_transform(float ia, float ib);

DQ park_transform(AlphaBeta in, float theta_e);

AlphaBeta inv_park_transform(DQ in, float theta_e);

PWM_Duty svpwm_generate(AlphaBeta v_alpha_beta, float vbus);

float pmsm_mech_to_elec_angle(float mech_angle_rad, uint8_t pole_pairs);

void pmsm_foc_speed_loop(PMSM_FOC *foc);

PWM_Duty pmsm_foc_current_loop(PMSM_FOC *foc,
                               float ia,
                               float ib,
                               float mech_angle_rad);

#ifdef __cplusplus
}
#endif

#endif
