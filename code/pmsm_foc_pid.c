#include "pmsm_foc_pid.h"

#include <math.h>

#define FOC_SQRT3        1.7320508075688772f
#define FOC_INV_SQRT3    0.5773502691896258f
#define FOC_TWO_PI       6.2831853071795865f

typedef struct {
    float a;
    float b;
    float c;
} PhaseABC;

static float foc_clampf(float x, float min_value, float max_value)
{
    if (x > max_value) {
        return max_value;
    }
    if (x < min_value) {
        return min_value;
    }
    return x;
}

static float foc_wrap_0_2pi(float angle)
{
    while (angle >= FOC_TWO_PI) {
        angle -= FOC_TWO_PI;
    }
    while (angle < 0.0f) {
        angle += FOC_TWO_PI;
    }
    return angle;
}

void pid_init(PID_Controller *pid,
              float kp,
              float ki,
              float kd,
              float ts,
              float out_min,
              float out_max,
              float integral_min,
              float integral_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->ts = ts;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_derivative = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    pid->derivative_lpf_alpha = 0.1f;
}

void pid_reset(PID_Controller *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_derivative = 0.0f;
}

float pid_update(PID_Controller *pid, float ref, float fdb)
{
    float error = ref - fdb;
    float proportional = pid->kp * error;

    float derivative_raw = (error - pid->prev_error) / pid->ts;
    float derivative = pid->prev_derivative
        + pid->derivative_lpf_alpha * (derivative_raw - pid->prev_derivative);

    float next_integral = pid->integral + error * pid->ts;
    next_integral = foc_clampf(next_integral,
                               pid->integral_min,
                               pid->integral_max);

    float output = proportional + pid->ki * next_integral + pid->kd * derivative;
    float saturated = foc_clampf(output, pid->out_min, pid->out_max);

    /*
     * Conditional integration anti-windup.
     * When output is already saturated, keep the integral from pushing farther
     * into saturation.
     */
    if (!((output > pid->out_max && error > 0.0f) ||
          (output < pid->out_min && error < 0.0f))) {
        pid->integral = next_integral;
    }

    pid->prev_error = error;
    pid->prev_derivative = derivative;

    return saturated;
}

AlphaBeta clarke_transform(float ia, float ib)
{
    AlphaBeta out;
    out.alpha = ia;
    out.beta = (ia + 2.0f * ib) * FOC_INV_SQRT3;
    return out;
}

DQ park_transform(AlphaBeta in, float theta_e)
{
    float s = sinf(theta_e);
    float c = cosf(theta_e);

    DQ out;
    out.d = in.alpha * c + in.beta * s;
    out.q = -in.alpha * s + in.beta * c;
    return out;
}

AlphaBeta inv_park_transform(DQ in, float theta_e)
{
    float s = sinf(theta_e);
    float c = cosf(theta_e);

    AlphaBeta out;
    out.alpha = in.d * c - in.q * s;
    out.beta = in.d * s + in.q * c;
    return out;
}

static AlphaBeta voltage_vector_limit(AlphaBeta v, float vbus)
{
    float vmax = vbus * FOC_INV_SQRT3;
    float mag_sq = v.alpha * v.alpha + v.beta * v.beta;
    float vmax_sq = vmax * vmax;

    if (mag_sq > vmax_sq && mag_sq > 0.0f) {
        float scale = vmax / sqrtf(mag_sq);
        v.alpha *= scale;
        v.beta *= scale;
    }

    return v;
}

PWM_Duty svpwm_generate(AlphaBeta v_alpha_beta, float vbus)
{
    PhaseABC vabc;
    vabc.a = v_alpha_beta.alpha;
    vabc.b = -0.5f * v_alpha_beta.alpha
        + 0.5f * FOC_SQRT3 * v_alpha_beta.beta;
    vabc.c = -0.5f * v_alpha_beta.alpha
        - 0.5f * FOC_SQRT3 * v_alpha_beta.beta;

    float vmax = fmaxf(vabc.a, fmaxf(vabc.b, vabc.c));
    float vmin = fminf(vabc.a, fminf(vabc.b, vabc.c));
    float vzero = -0.5f * (vmax + vmin);

    PWM_Duty duty;
    duty.duty_a = foc_clampf((vabc.a + vzero) / vbus + 0.5f, 0.0f, 1.0f);
    duty.duty_b = foc_clampf((vabc.b + vzero) / vbus + 0.5f, 0.0f, 1.0f);
    duty.duty_c = foc_clampf((vabc.c + vzero) / vbus + 0.5f, 0.0f, 1.0f);
    return duty;
}

float pmsm_mech_to_elec_angle(float mech_angle_rad, uint8_t pole_pairs)
{
    return foc_wrap_0_2pi(mech_angle_rad * (float)pole_pairs);
}

void pmsm_foc_speed_loop(PMSM_FOC *foc)
{
    foc->iq_ref = pid_update(&foc->speed_pi,
                             foc->speed_ref_rad_s,
                             foc->speed_fdb_rad_s);
}

PWM_Duty pmsm_foc_current_loop(PMSM_FOC *foc,
                               float ia,
                               float ib,
                               float mech_angle_rad)
{
    float theta_e = pmsm_mech_to_elec_angle(mech_angle_rad, foc->pole_pairs);

    AlphaBeta i_ab = clarke_transform(ia, ib);
    DQ i_dq = park_transform(i_ab, theta_e);

    DQ v_dq;
    v_dq.d = pid_update(&foc->id_pi, foc->id_ref, i_dq.d);
    v_dq.q = pid_update(&foc->iq_pi, foc->iq_ref, i_dq.q);

    AlphaBeta v_ab = inv_park_transform(v_dq, theta_e);
    v_ab = voltage_vector_limit(v_ab, foc->vbus);

    return svpwm_generate(v_ab, foc->vbus);
}
