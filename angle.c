/* angle.c:  find the angle of maximum response and steer to it 

  Copyright (c) 2005, 2020  Tyler Folsom.  All rights reserved.

*/

//#include "stdafx.h"  // for Windows
#include "features.h"
#include "basetypes.h"
#include <math.h>  /* for sin, cos */

/* Solve for angles within 0.1 degree */
#define ANGLE_CONVEGENCE ((float) 0.00174)
#define G2_ORN 3
#define WAY_SMALL  ((float) -1.0e20)

float steer_60(float *sampled, float theta);
float steer_90(float *sampled, float theta);

extern float corrEven[3], corrOdd[2];

/*------------------------------------------------------------------------*/
/* 
    steer_60: Given input filters sampled at 60 degrees, interpolate
    to the filter response at angle theta.

    input:  sampled - response of filters at 0, 60, 120 degrees.
            theta   - angle for desired response in radians.
    returned value: the interpolated filter response at theta.
*/
float steer_60( 
    float *sampled,
    float theta)
{
    float angle;
    float steered;
    int i_angle;

    angle = (float) (PI_1 / G2_ORN);
    steered = (float) 0.0;
    for (i_angle = 0; i_angle < G2_ORN; i_angle++)
        steered += sampled[i_angle] * ((float) 1.0 + (float) 2.0 *
         (float) cos( (2.0 * theta - PI_2 * (float)i_angle / 
         (float) G2_ORN)));
    steered /= G2_ORN;
    return (steered);
}

/*------------------------------------------------------------------------*/
/* 
    steer_90: Given input filters sampled at 90 degrees, interpolate
    to the filter response at angle theta.

    input:  sampled - response of filters at 0, 90 degrees.
            theta   - angle for desired response in radians.
    returned value: the interpolated filter response at theta.
	*/
float steer_90( 
    float *sampled,
    float theta)
{
    return ( (float) (sampled[0] * cos( theta)
                    + sampled[1] * sin( theta)) );
}
/*------------------------------------------------------------------------*/
/* 
    solve_max: Given sampled input filters, and an initial estimate of the angle 
    of the dominant response, solve numerically for this angle.

    input:  sampled - coefficients from response of filters.
            trig    - the expression whose maximum we are looking for.
    modified input: theta - angle of dominant response (radians).
    returned value: value of function at theta (less a constant)
*/
float solve_max( 
    float *sampled,
    float *theta,
    float (*trig)(float *, float, float *, float *))
{

    float theta1, theta2, value1, value2;
    float th_low, th_hi, val_low, val_hi, d_low, d_hi;
    float deriv1, deriv2, delta;
    BOOL bracketed = FALSE;

    /* Try to bracket the maximum */
    theta1 = *theta;
    value1 = (*trig) (sampled, theta1, &deriv1, &deriv2);
    if (FL_ABS(deriv1) < EPSILON && deriv2 <= 0)
    {
        return value1;
    }
    while (!bracketed)
    {
        d_low = deriv1;
        if (FL_ABS(deriv2) < EPSILON)
        {
            delta = (deriv1 > 0? (float) 0.1: (float) -0.1);   /* 0.1 = about 5 degrees */
        }
        else
        {   /* Newton-Raphson method for finding the zero of deriv1 */
            delta = deriv1 / FL_ABS(deriv2);
            if (delta < -PI8TH)
                delta = -PI8TH;
            if (delta >  PI8TH)
                delta =  PI8TH;
            if (FL_ABS(delta) < (float) 0.02)  /* about 1 degree */
                delta = (delta < 0? (float) -0.02: (float) 0.02);
        }
        theta2 = theta1 + delta;  // In the direction in which the derivative increases
        value2 = (*trig) (sampled, theta2, &deriv1, &deriv2);
        if (FL_ABS(deriv1) < EPSILON && deriv2 < 0)
        {
            while (theta2 >= PI_1)
                theta2 -= PI_1;
            while (theta2 < 0)
                theta2 += PI_1;
            *theta = theta2;
            return value2;
        }
        if (value2 < value1 ||
           (theta2 > theta1  && deriv1 < 0) ||
           (theta2 < theta1  && deriv1 > 0))
        {
            bracketed = TRUE;
        }
        else
        {   // we are still increasing from the starting point
            theta1 = theta2;
            value1 = value2;
        }
    }  // bracketed

    if (theta1 < theta2)
    {
        th_low = theta1;
        th_hi  = theta2;
        val_low = value1;
        val_hi  = value2;
        d_hi  =  deriv1;
    }
    else
    {
        th_low = theta2;
        th_hi  = theta1;
        val_low = value2;
        val_hi  = value1;
        d_hi  =  d_low;
        d_low =  deriv1;
    }
    while (th_hi - th_low > ANGLE_CONVEGENCE)
    {
        theta1 = theta2;
        if (FL_ABS(deriv2) < EPSILON)
        {
            delta = (deriv1 > 0? (float) 0.1: (float) -0.1);   /* 0.1 = about 5 degrees */
        }
        else
        {   /* Newton-Raphson method for finding the zero of deriv1 */
            delta = deriv1 / FL_ABS(deriv2);
            if (FL_ABS(delta) < ANGLE_CONVEGENCE / 2)
                delta = delta < 0? -ANGLE_CONVEGENCE / 2: ANGLE_CONVEGENCE / 2;
        }
        theta2 = theta1 + delta;  // In the direction in which the derivative increases
        if (theta2 <= th_low)
        {
            theta2 = th_low + FL4TH * (th_hi - th_low);
        }
        if (theta2 >= th_hi)
        {
            theta2 = th_hi - FL4TH * (th_hi - th_low);
        }
        value2 = (*trig) (sampled, theta2, &deriv1, &deriv2);
        if (FL_ABS(deriv1) < EPSILON && deriv2 < 0)
        {
            break;
        }
        /* A positive derivative replaces the old lower bound,
           except possibly when derivatives were positive at both bounds.
           In this case the old upper bound is replaced by an angle with a
           lower value.
        */
        if (d_hi > 0 && value2 <= val_hi)
        {  // bracketed region has a max and a min
            th_hi = theta2;
            d_hi = deriv1;
            val_hi = value2;
        }
        else if (d_low < 0 && value2 <= val_low)
        {  // bracketed region has a min and a max
            th_low = theta2;
            d_low = deriv1;
            val_low = value2;
        }
        else if (deriv1 > 0)
        {
            th_low = theta2;
            d_low = deriv1;
            val_low = value2;
        }
        else /* negative or zero derivative replaces old upper bound */
        {
            th_hi = theta2;
            d_hi = deriv1;
            val_hi = value2;
        }
    }
    while (theta2 < -PI_1)
        theta2 += PI_2;
    while (theta2 >= PI_1 - EPSILON)
        theta2 -= PI_2;
    if (theta2 < -PI_1)
        theta2 = PI_1;
    *theta = theta2;
    return value2;
}
/*------------------------------------------------------------------------*/
/* 
    g1h1: Given coefficients from 3 input filters sampled at 60 degrees 
    and 2 filters at 90 degrees, and a steering angle, compute the 
    squared magnitude of the filter at this angle and its derivatives.
    Since the function is trigonometric, all derivatives are well behaved.

    input:  coef - 0,1,2,3: coefficients of cos2x, sin2x, cos4x, sin4x.
              4,5,6,7:  coefs derivative of cos2x, sin2x, cos4x, sin4x.
            theta - angle of dominant response (radians).
    returned value: squared magnitude of filter response (less a constant).
    output:  *deriv - first derivative
             *deriv2 - second derivative

    The expression that we are maximizing is the squared response of steering
	the even and odd filters.

	The odd steering equation is
	G(theta) = G0 * cos(theta) + G90 * sin(theta)
	where G0 = Alpha1 = gl[3] = corrOdd[0] is vertical odd filter response 
	and  G90 = Alpha2 = gl[4] = corrOdd[1] is horizontal odd filter response.
	Thus 
	G(theta)^2 = 0.5 * (G0^2 + G90^2) + 0.5 * (G0^2 - G90^2) * cos(2*theta)
	            + G0 * G90 * sin(2*theta)

    The even steering equation is
	H(theta) = (H0 + H60 + H120)/3 + (2*H0 - H60 - H120) * cos(2*theta)/3
	          + (H60 - H120) * sin(2*theta) / sqrt(3)
    where H0 = corrEven[0] is vertical even filter response
	     H60 = corrEven[1] is even filter response at 60 degrees
		H120 = corrEven[2] is even filter response at 120 degrees
	H(theta) = gl[0] + gl[1] * cos(2*theta) + gl[2] * sin(2*theta)
	where gl[0] = Gamma1 = (H0 + H60 + H120)/3 
	      gl[1] = Gamma2 = (2*H0 - H60 - H120)/3
		  gl[2] = Gamma3 = (H60 - H120) / sqrt(3)
    H(theta)^2 = gl[0]^2 + 0.5*gl[1]^2 + 0.5*gl[2]^2
	            + 2 * gl[0] * gl[1] * cos(2*theta) + 2 * gl[0] * gl[2] * sin(2*theta)
				+ 0.5*(gl[1]^2 - gl[2]^2) * cos(4*theta) + gl[1]*gl[2] * sin(4*theta)

    Combining the two equations gives
	G(theta)^2 + H(theta)^2 
	= 0.5 * (gl[3]^2 + gl[4]^2) + 0.5 * (gl[3]^2 - gl[4]^2) * cos(2*theta)
	+ gl[3] * gl[4] * sin(2*theta)
	+ gl[0]^2 + 0.5*gl[1]^2 + 0.5*gl[2]^2
	+ 2 * gl[0] * gl[1] * cos(2*theta) + 2 * gl[0] * gl[2] * sin(2*theta)
	+ 0.5*(gl[1]^2 - gl[2]^2) * cos(4*theta) + gl[1]*gl[2] * sin(4*theta)

  	G(theta)^2 + H(theta)^2 
	= 0.5 * (gl[3]^2 + gl[4]^2) + gl[0]^2 + 0.5*gl[1]^2 + 0.5*gl[2]^2
	+ (0.5 * (gl[3]^2 - gl[4]^2) + 2 * gl[0] * gl[1]) * cos(2*theta)
	+ (gl[3] * gl[4] + 2 * gl[0] * gl[2])* sin(2*theta)
	+ 0.5*(gl[1]^2 - gl[2]^2) * cos(4*theta) + gl[1]*gl[2] * sin(4*theta)

  	G(theta)^2 + H(theta)^2 
	= 0.5 * (gl[3]^2 + gl[4]^2) + gl[0]^2 + 0.5*gl[1]^2 + 0.5*gl[2]^2
	+ coef[0] * cos(2*theta) + coef[1] * sin(2*theta)
	+ coef[2] * cos(4*theta) + coef[3] * sin(4*theta)

    The constant term is ignored, since it is irrelevant to maximizing the response.
	Thus the returned value can be negative.

    The first derivative of this expression is:
	-2 * coef[0] * sin(2*theta) + 2 * coef[1] * cos(2*theta)
	-4 * coef[2] * sin(4*theta) + 4 * coef[3] * cos(4*theta)
	=    coef[4] * cos(2*theta) + coef[5] * sin(2*theta)
	   + coef[6] * cos(4*theta) + coef[7] * sin(4*theta)

    The second derivative  is:
	-4  * coef[0] * cos(2*theta) - 4 * coef[1] * sin(2*theta)
	-16 * coef[2] * cos(4*theta) -16 * coef[3] * sin(4*theta)

	*/
float g1h1( 
    float *coef,
    float theta,
    float *deriv,
    float *deriv2)
{
    float sin_2th, cos_2th, sin_4th, cos_4th;
    float dtheta;
    float funct;

    dtheta = 2 * theta;
    sin_2th = (float) sin( dtheta);
    cos_2th = (float) cos( dtheta);
    sin_4th = FL2 * sin_2th * cos_2th;
    cos_4th = cos_2th * cos_2th - sin_2th * sin_2th;

    *deriv = coef[4] * cos_2th
           + coef[5] * sin_2th
           + coef[6] * cos_4th
           + coef[7] * sin_4th;
    funct =  coef[2] * cos_4th
           + coef[3] * sin_4th;
    *deriv2 = -12 * funct;  // pick up another -4 * these terms below.
    funct += coef[0] * cos_2th
           + coef[1] * sin_2th;
    *deriv2 -= FL4 * funct;
    return funct;
}
/*------------------------------------------------------------------------*/
// Finds the angle (radians) of the feature based on filter correlations
/* 
    GetAngle: Given input filters sampled at 60 and 90 degrees, find the angle
    of the dominant response.

    input:  corrEven - response of even filters at 0, 60, 120 degrees.
            corrOdd - response of odd filters at 0 and 90 degrees.
    output: dom_resp - squared response of the filter at the dominant angle.
            *steeredEven - interpolated even response at the dominant angle.
            *steeredOdd - interpolated odd response at the dominant angle.
    returned value: angle of dominant response in radians.


    The newer, more efficient, method uses solve_max() and g1h1() to find the 
	angle that gives maximum response.  For documentation, see the header for
	g1h1()  and  Folsom & Pinter, "Primitive Features by Steering, Quadrature
	and Scale", IEEE Trans PAMI, Nov 1998, pp. 1161-1173.

*/
float GetAngle( 
    float *dom_resp,
    float *steeredEven,
    float *steeredOdd)

{

    float gl[7];  /* gamma and lamda coefficients */
    float coef[8];  /* combinations of gamma and lambda for trig terms*/
    float angles[4];  /* 4 solutions */
    float theta;
    float max_mag, value;
	int i;

    max_mag = WAY_SMALL;
    /* Gamma1, gamma2, gamma3 */
    gl[0] = (corrEven[0] + corrEven[1] + corrEven[2]) / FL3;
    gl[1] = (FL2 * corrEven[0] - corrEven[1] - corrEven[2]) / FL3;
    gl[2] = (corrEven[1] - corrEven[2]) / ROOT_3;

	/* Alpha 1, alpha 2 */
	gl[3] = corrOdd[0];
	gl[4] = corrOdd[1];
	/* [4 ...7] derivative coefficients for cos 2x, sin 2x, cos 4x, sin 4x */
	coef[5] = -(FL4*gl[0]*gl[1] + gl[3]*gl[3] - gl[4]*gl[4]);
	coef[7] = FL2 * (-gl[1]*gl[1] + gl[2]*gl[2]);
	/* [0...1] coefficients for cos 2x, sin 2x, cos 4x, sin 4x */
	coef[0] = -FLHALF * coef[5];
	coef[1] =  FL2*gl[0]*gl[2] + gl[3]*gl[4];
	coef[4] =  FL2 * coef[1];
	coef[2] = -FL4TH  * coef[7];
	coef[3] =  gl[1] * gl[2];
	coef[6] =  FL4  * coef[3];
	/* Can find explicit solution when gl[2]*gl[3] == gl[1]*gl[4] */
	//      if (FL_ABS(gl[2]*gl[3] - gl[1]*gl[4]) < EPSILON * GetStrengthBound())
	// set initial guesses at solutions for odd only or even only
	angles[0] = (float) atan2(gl[4], gl[3]);
  // phase is unstable at 0 or +/-180 when  y is close to zero
	angles[1] = angles[0] + (float) PI_HALF;
	angles[2] = (float) atan2(gl[2], gl[1]) * FLHALF;
	angles[3] = angles[2] + (float) PI_HALF;
	for (i = 0; i < 4; i++)
	{
		value = solve_max (coef, &angles[i], g1h1);
		if (value > max_mag)
		{
			max_mag = value;
			theta = angles[i];
		}
	}
	// Check whether we want this angle or + 180.
	// The dark side of the image should be to our left.
	*steeredOdd = steer_90 (corrOdd, theta);
	if (*steeredOdd < 0)
	{
		theta += PI_1;
		*steeredOdd = steer_90 (corrOdd, theta);
	}
	while (theta > PI_1)
		theta -= PI_2;
	while (theta < -PI_1)
		theta += PI_2;


	*steeredEven = steer_60 (corrEven, theta);
    *dom_resp = (float) sqrt(*steeredOdd * *steeredOdd + *steeredEven * *steeredEven);

#ifdef VERT_STEREO
	theta += PI_HALF;
	while (theta > PI_1)
		theta -= PI_2;
#endif
    return theta;

}
