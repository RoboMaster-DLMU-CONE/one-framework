// C
#ifndef OF_HEATER_PID_H
#define OF_HEATER_PID_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float Kp;
    float Ki;
    float Kd;
    float IntegralLimit; /* limit for integral term */
    float MaxOutput; /* maximum absolute output */
    float Deadband; /* error deadband (absolute) */
} PID_Params_t;

/* Simplified, concrete PID object (no dynamic allocation) */
typedef struct PID_Obj_t
{
    PID_Params_t p;
    float prev_error;
    float integral;
} PID_t;

/* Initialize a PID object with provided parameters. */
static inline void PID_Init(PID_t* pid, const PID_Params_t* params)
{
    if (pid == NULL || params == NULL)
    {
        return;
    }
    pid->p = *params;
    pid->prev_error = 0.0f;
    pid->integral = 0.0f;
}

/* Reset integral and derivative state */
static inline void PID_Reset(PID_t* pid)
{
    if (pid == NULL)
    {
        return;
    }
    pid->prev_error = 0.0f;
    pid->integral = 0.0f;
}

/* Compute PID output. Operates on the provided PID_t object. */
static inline float PID_Compute(PID_t* pid, float setpoint, float measurement)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    float error = setpoint - measurement;

    if (fabsf(error) <= pid->p.Deadband)
    {
        /* in deadband, do not integrate or change previous error */
        return 0.0f;
    }

    /* simple discrete integration (assumes fixed call interval) */
    pid->integral += error;
    if (pid->integral > pid->p.IntegralLimit)
    {
        pid->integral = pid->p.IntegralLimit;
    }
    else if (pid->integral < -pid->p.IntegralLimit)
    {
        pid->integral = -pid->p.IntegralLimit;
    }

    float derivative = error - pid->prev_error;

    float output = pid->p.Kp * error
        + pid->p.Ki * pid->integral
        + pid->p.Kd * derivative;

    /* clamp output to configured max magnitude */
    if (output > pid->p.MaxOutput)
    {
        output = pid->p.MaxOutput;
    }
    else if (output < -pid->p.MaxOutput)
    {
        output = -pid->p.MaxOutput;
    }

    pid->prev_error = error;
    return output;
}

#ifdef __cplusplus
}
#endif

#endif // OF_HEATER_PID_H