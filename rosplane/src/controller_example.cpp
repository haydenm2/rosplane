#include "controller_example.h"

namespace rosplane
{

controller_example::controller_example() : controller_base()
{

  c_error_ = 0;
  c_integrator_ = 0;
  r_error_ = 0;
  r_integrator = 0;
  p_error_ = 0;
  p_integrator_ = 0;
  at_error_ = 0;
  at_integrator_ = 0;
  at_differentiator_ = 0;
  a_error_ = 0;
  a_integrator_ = 0;
  a_differentiator_ = 0;
  h_c_sat = 0;

}

void controller_example::control(const params_s &params, const input_s &input, output_s &output)
{
  //Lateral Control
  output.phi_c = course_hold(input.chi_c, input.chi, input.phi_ff, input.r, params, input.Ts);
  output.delta_a = roll_hold(output.phi_c, input.phi, input.p, params, input.Ts);
  output.delta_r = 0; //yaw_damper();
  
  //Longitudinal Control
  h_c_sat = sat(input.h_c, input.h+params.alt_hz, input.h-params.alt_hz);
  output.theta_c = altitude_hold(h_c_sat, input.h, params, input.Ts);
  output.delta_e = pitch_hold(output.theta_c, input.theta, input.q, params, input.Ts);
  output.delta_t = sat(airspeed_with_throttle_hold(input.Va_c, input.va, params, input.Ts), 1.0, 0.0);
}

float controller_example::course_hold(float chi_c, float chi, float phi_ff, float r, const params_s &params, float Ts)
{
  float error = chi_c - chi;

  c_integrator_ = c_integrator_ + (Ts/2.0)*(error + c_error_);

  float up = params.c_kp*error;
  float ui = params.c_ki*c_integrator_;
  float ud = params.c_kd*r;

  float phi_c = sat(up + ui + ud + phi_ff, 40.0*3.14/180.0, -40.0*3.14/180.0);
  if (fabs(params.c_ki) >= 0.00001)
  {
    float phi_c_unsat = up + ui + ud + phi_ff;
    c_integrator_ = c_integrator_ + (Ts/params.c_ki)*(phi_c - phi_c_unsat);
  }

  c_error_ = error;
  return phi_c;
}

float controller_example::roll_hold(float phi_c, float phi, float p, const params_s &params, float Ts)
{
  float error = phi_c - phi;

  r_integrator = r_integrator + (Ts/2.0)*(error + r_error_);

  float up = params.r_kp*error;
  float ui = params.r_ki*r_integrator;
  float ud = params.r_kd*p;

  float delta_a = sat(up + ui + ud, params.max_a, -params.max_a);
  if (fabs(params.r_ki) >= 0.00001)
  {
    float delta_a_unsat = up + ui + ud;
    r_integrator = r_integrator + (Ts/params.r_ki)*(delta_a - delta_a_unsat);
  }

  r_error_ = error;
  return delta_a;
}

float controller_example::pitch_hold(float theta_c, float theta, float q, const params_s &params, float Ts)
{
  float error = theta_c - theta;

  p_integrator_ = p_integrator_ + (Ts/2.0)*(error + p_error_);

  float up = params.p_kp*error;
  float ui = params.p_ki*p_integrator_;
  float ud = params.p_kd*q;

  float delta_e = sat(params.trim_e/params.pwm_rad_e + up + ui + ud, params.max_e, -params.max_e);
  if (fabs(params.p_ki) >= 0.00001)
  {
    float delta_e_unsat = params.trim_e/params.pwm_rad_e + up + ui + ud;
    p_integrator_ = p_integrator_ + (Ts/params.p_ki)*(delta_e - delta_e_unsat);
  }

  p_error_ = error;
  return delta_e;
}

float controller_example::airspeed_with_throttle_hold(float Va_c, float Va, const params_s &params, float Ts)
{
  float error = Va_c - Va;

  at_integrator_ = at_integrator_ + (Ts/2.0)*(error + at_error_);
  at_differentiator_ = (2.0*params.tau - Ts)/(2.0*params.tau + Ts)*at_differentiator_ + (2.0 /
                       (2.0*params.tau + Ts))*(error - at_error_);

  float up = params.a_t_kp*error;
  float ui = params.a_t_ki*at_integrator_;
  float ud = params.a_t_kd*at_differentiator_;

  float delta_t = sat(params.trim_t + up + ui + ud, params.max_t, 0);
  if (fabs(params.a_t_ki) >= 0.00001)
  {
    float delta_t_unsat = params.trim_t + up + ui + ud;
    at_integrator_ = at_integrator_ + (Ts/params.a_t_ki)*(delta_t - delta_t_unsat);
  }

  at_error_ = error;
  return delta_t;
}

float controller_example::altitude_hold(float h_c, float h, const params_s &params, float Ts)
{
  float error = h_c - h;

  a_integrator_ = a_integrator_ + (Ts/2.0)*(error + a_error_);
  a_differentiator_ = (2.0*params.tau - Ts)/(2.0*params.tau + Ts)*a_differentiator_ + (2.0 /
                      (2.0*params.tau + Ts))*(error - a_error_);

  float up = params.a_kp*error;
  float ui = params.a_ki*a_integrator_;
  float ud = params.a_kd*a_differentiator_;

  float theta_c = sat(up + ui + ud, 35.0*3.14/180.0, -35.0*3.14/180.0);
  if (fabs(params.a_ki) >= 0.00001)
  {
    float theta_c_unsat = up + ui + ud;
    a_integrator_ = a_integrator_ + (Ts/params.a_ki)*(theta_c - theta_c_unsat);
  }

  a_error_ = error;
  return theta_c;
}

//float controller_example::cooridinated_turn_hold(float v, const params_s &params, float Ts)
//{
//    //todo finish this if you want...
//    return 0;
//}

float controller_example::sat(float value, float up_limit, float low_limit)
{
  float rVal;
  if (value > up_limit)
    rVal = up_limit;
  else if (value < low_limit)
    rVal = low_limit;
  else
    rVal = value;

  return rVal;
}

} //end namespace
