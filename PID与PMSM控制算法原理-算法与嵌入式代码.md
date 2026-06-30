# PID与PMSM控制算法原理：算法设计与嵌入式代码

## 1. 控制对象与总体结构

永磁同步电机（PMSM, Permanent Magnet Synchronous Motor）的高性能控制通常采用磁场定向控制（FOC, Field Oriented Control）。FOC 的核心思想是通过坐标变换把三相交流电机等效为直流电机形式，将电流分解为励磁分量 `Id` 和转矩分量 `Iq`，再分别闭环控制。

典型控制结构如下：

```text
速度给定 w_ref
    |
    v
速度 PID/PI  ---->  Iq_ref  ----+
                                  |
Id_ref = 0 --------------------+  |
                               v  v
                         电流 PI 控制器
                         |         |
                       Vd_ref    Vq_ref
                         |         |
                         +----+----+
                              v
                         反 Park 变换
                              v
                         SVPWM / PWM
                              v
                            逆变器
                              v
                             PMSM
                              |
               采样 ia, ib, ic, theta, speed
                              |
             Clarke/Park 变换与速度估算反馈
```

工程上常用三闭环或双闭环结构：

- 位置环：输出速度给定，适用于伺服定位。
- 速度环：输出 `Iq_ref`，控制转矩。
- 电流环：输出 `Vd_ref`、`Vq_ref`，控制定子电流。

电流环的带宽最高，速度环次之，位置环最低。常见经验是：电流环控制周期为 `50 us ~ 100 us`，速度环周期为 `0.5 ms ~ 2 ms`，位置环周期为 `1 ms ~ 10 ms`。

## 2. PID 离散控制算法

连续 PID 控制律为：

```text
u(t) = Kp * e(t) + Ki * integral(e(t)) + Kd * de(t)/dt
```

其中：

- `e(t) = ref(t) - feedback(t)` 为控制误差。
- `Kp` 决定响应速度和刚度。
- `Ki` 用于消除静差，但过大会引起积分饱和和振荡。
- `Kd` 用于抑制误差变化率，但对噪声敏感，电机电流环和速度环中通常较少直接使用纯微分项。

嵌入式控制中使用离散形式。设采样周期为 `Ts`，位置式 PID 为：

```text
error[k] = ref[k] - fdb[k]
integral[k] = integral[k-1] + error[k] * Ts
derivative[k] = (error[k] - error[k-1]) / Ts
out[k] = Kp * error[k] + Ki * integral[k] + Kd * derivative[k]
```

为了便于工程使用，需要加入：

- 输出限幅：防止控制量超过物理范围。
- 积分限幅：防止积分项长期累积导致恢复慢。
- 抗积分饱和：输出已经饱和时，限制积分继续朝错误方向增长。
- 低通滤波微分：微分项容易放大采样噪声。

在 PMSM FOC 中，电流环常用 PI 控制器：

```text
Vd = Kp_id * (Id_ref - Id) + Ki_id * integral(Id_ref - Id)
Vq = Kp_iq * (Iq_ref - Iq) + Ki_iq * integral(Iq_ref - Iq)
```

速度环也常用 PI：

```text
Iq_ref = Kp_w * (w_ref - w) + Ki_w * integral(w_ref - w)
```

## 3. PMSM 数学模型

三相静止坐标下，PMSM 的电压、电流都是交流量，直接控制复杂。FOC 先通过 Clarke 变换把三相量变为两相静止坐标系 `alpha-beta`，再通过 Park 变换变为旋转坐标系 `d-q`。

### 3.1 Clarke 变换

若三相电流满足：

```text
ia + ib + ic = 0
```

则可使用两电流采样：

```text
Ialpha = ia
Ibeta  = (ia + 2 * ib) / sqrt(3)
```

### 3.2 Park 变换

将静止坐标系电流变换到转子磁场同步旋转坐标系：

```text
Id =  Ialpha * cos(theta) + Ibeta * sin(theta)
Iq = -Ialpha * sin(theta) + Ibeta * cos(theta)
```

其中 `theta` 为电角度。机械角度与电角度的关系为：

```text
theta_e = pole_pairs * theta_m
```

### 3.3 反 Park 变换

电流环输出 `Vd` 和 `Vq` 后，需要变回 `alpha-beta` 静止坐标系：

```text
Valpha = Vd * cos(theta) - Vq * sin(theta)
Vbeta  = Vd * sin(theta) + Vq * cos(theta)
```

随后通过 SVPWM 或 SPWM 生成三相占空比。

## 4. FOC 控制算法步骤

一次电流环中断通常执行以下步骤：

1. 采样三相电流，获得 `ia`、`ib`，必要时重构 `ic`。
2. 读取编码器、霍尔传感器或观测器输出，获得电角度 `theta_e`。
3. 执行 Clarke 变换，得到 `Ialpha`、`Ibeta`。
4. 执行 Park 变换，得到 `Id`、`Iq`。
5. 速度环按较低频率运行，输出 `Iq_ref`。
6. 设置 `Id_ref`。表贴式 PMSM 常用 `Id_ref = 0`，弱磁区可为负值。
7. 电流 PI 控制器计算 `Vd`、`Vq`。
8. 对电压矢量限幅，避免超过母线可输出范围。
9. 反 Park 变换得到 `Valpha`、`Vbeta`。
10. 通过 SVPWM 计算三相 PWM 占空比。
11. 更新定时器比较寄存器。

## 5. 可移植嵌入式 C 代码

下面代码不依赖具体芯片外设，适合先在 STM32、TI C2000、NXP 或其他 MCU 工程中作为算法层使用。ADC 采样、编码器读取和 PWM 更新需要对接各平台驱动。

```c
#include <math.h>
#include <stdint.h>

#define FOC_SQRT3        1.7320508075688772f
#define FOC_INV_SQRT3    0.5773502691896258f
#define FOC_TWO_PI       6.2831853071795865f

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
    float a;
    float b;
    float c;
} PhaseABC;

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
    next_integral = foc_clampf(next_integral, pid->integral_min, pid->integral_max);

    float output = proportional + pid->ki * next_integral + pid->kd * derivative;
    float saturated = foc_clampf(output, pid->out_min, pid->out_max);

    /*
     * Conditional integration anti-windup:
     * if output is saturated and the error would push it further into saturation,
     * discard this integration step.
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
    float vmax = vbus * 0.577350269f;
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
    /*
     * Center-aligned equivalent SVPWM using zero-sequence injection.
     * Input voltage is phase-neutral alpha-beta voltage command.
     * Output duty is normalized to [0, 1].
     */
    PhaseABC vabc;
    vabc.a = v_alpha_beta.alpha;
    vabc.b = -0.5f * v_alpha_beta.alpha + 0.5f * FOC_SQRT3 * v_alpha_beta.beta;
    vabc.c = -0.5f * v_alpha_beta.alpha - 0.5f * FOC_SQRT3 * v_alpha_beta.beta;

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
```

## 6. MCU 工程接入示例

下面示例展示控制中断如何调用算法层。实际工程中，`adc_get_phase_a_current()`、`encoder_get_mech_angle_rad()`、`pwm_set_duty()` 等函数由芯片驱动实现。

```c
static PMSM_FOC g_foc;

void motor_control_init(void)
{
    g_foc.pole_pairs = 4;
    g_foc.vbus = 24.0f;
    g_foc.id_ref = 0.0f;
    g_foc.iq_ref = 0.0f;
    g_foc.speed_ref_rad_s = 0.0f;
    g_foc.speed_fdb_rad_s = 0.0f;

    pid_init(&g_foc.id_pi, 1.2f, 800.0f, 0.0f, 0.0001f,
             -12.0f, 12.0f, -0.02f, 0.02f);
    pid_init(&g_foc.iq_pi, 1.2f, 800.0f, 0.0f, 0.0001f,
             -12.0f, 12.0f, -0.02f, 0.02f);
    pid_init(&g_foc.speed_pi, 0.03f, 0.6f, 0.0f, 0.001f,
             -8.0f, 8.0f, -4.0f, 4.0f);
}

void speed_control_1khz_task(void)
{
    g_foc.speed_fdb_rad_s = encoder_get_speed_rad_s();
    pmsm_foc_speed_loop(&g_foc);
}

void current_control_10khz_isr(void)
{
    float ia = adc_get_phase_a_current();
    float ib = adc_get_phase_b_current();
    float mech_angle = encoder_get_mech_angle_rad();

    PWM_Duty duty = pmsm_foc_current_loop(&g_foc, ia, ib, mech_angle);

    pwm_set_duty(duty.duty_a, duty.duty_b, duty.duty_c);
}

void motor_set_speed_rpm(float rpm)
{
    g_foc.speed_ref_rad_s = rpm * 0.104719755f;
}
```

## 7. 参数整定方法

### 7.1 电流环 PI 整定

电流环直接决定 FOC 的动态性能。常用步骤：

1. 先关闭速度环，直接给定较小 `Iq_ref`。
2. 设置 `Id_ref = 0`。
3. 令 `Ki = 0`，逐步增大 `Kp`，直到电流响应足够快但无明显振荡。
4. 在保持稳定的基础上逐步增加 `Ki`，直到稳态误差消失。
5. 若电流噪声大，应降低带宽或增加采样滤波。

电流环输出限幅一般与母线电压有关。SVPWM 线性区可近似取：

```text
Vmax = Vbus / sqrt(3)
```

### 7.2 速度环 PI 整定

速度环输出是 `Iq_ref`，因此输出限幅应根据电机额定电流或驱动器最大电流设置。

调试步骤：

1. 确认电流环稳定。
2. 速度环先设置较小 `Kp`，`Ki = 0`。
3. 增大 `Kp`，观察转速响应与超调。
4. 增加 `Ki` 消除速度静差。
5. 若出现低频振荡，降低 `Ki` 或增加速度反馈滤波。

## 8. 工程注意事项

- 电角度方向必须与相序一致。若一给 `Iq_ref` 电机就抖动或反转，应检查编码器方向、相序和 Park 变换符号。
- ADC 电流零点偏置必须校准，否则 `Id/Iq` 会有明显静差。
- PWM 更新应与 ADC 采样同步，优先在中心对齐 PWM 的中点采样。
- 启动前应完成编码器零位标定或转子初始位置检测。
- `sin`、`cos` 在高频 ISR 中可能开销较大，可改用查表、CORDIC 或 MCU DSP 库。
- 电流环 ISR 中不要执行阻塞函数、浮点打印或复杂通信协议。
- 速度反馈应滤波，但滤波过强会增加相位滞后，导致速度环振荡。

## 9. 可扩展功能

基础 FOC 稳定后，可以继续增加：

- 弱磁控制：高速区设置负 `Id_ref`，扩大转速范围。
- MTPA 控制：内嵌式 PMSM 可通过最优 `Id/Iq` 分配提高效率。
- 前馈解耦：补偿 `d-q` 轴交叉耦合，提高高速动态性能。
- 无感控制：通过反电动势、滑模观测器或 PLL 估算转子角度。
- 电流采样重构：单电阻或双电阻采样场景中重构三相电流。

## 10. 前馈解耦补充

PMSM 在 `d-q` 坐标系下的电压方程可写为：

```text
Vd = Rs * Id + Ld * dId/dt - we * Lq * Iq
Vq = Rs * Iq + Lq * dIq/dt + we * (Ld * Id + flux)
```

其中：

- `Rs` 为定子电阻。
- `Ld`、`Lq` 为 d/q 轴电感。
- `we` 为电角速度。
- `flux` 为永磁体磁链。

电流 PI 输出后加入前馈项：

```text
Vd_cmd = Vd_pi - we * Lq * Iq
Vq_cmd = Vq_pi + we * (Ld * Id + flux)
```

这样可以减小高速运行时的交叉耦合，提高电流跟踪能力。初期调试时建议先关闭前馈，等基础闭环稳定后再加入。

