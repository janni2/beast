// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIGNAL_H__
#define __BSE_SIGNAL_H__

#include <bse/bsemath.hh>
#include <bse/signalmath.hh>
#include <bse/bseglobals.hh>
#include <bse/bsetype.hh> // for BseMusicalTuningType

/**
 * smallest value of a signal sample, greater than zero
 */
#define BSE_SIGNAL_EPSILON      (1.15e-14)      /* 1.16415321826934814453125e-9 ~= 1/2^33 */

/**
 * maximum value of a signal sample
 */
#define BSE_SIGNAL_KAPPA        (1.5)

/**
 * Catch edges in sync signals.
 * sync signals should be constant, do comparing against
 * an epsilon just hurts speed in the common case.
 */
#define BSE_SIGNAL_RAISING_EDGE(v1,v2)	((v1) < (v2))
/**
 * Inverse variant of BSE_SIGNAL_RAISING_EDGE().
 */
#define BSE_SIGNAL_FALLING_EDGE(v1,v2)	((v1) > (v2))

/**
 * Value changes in signals which represent frequencies.
 */
#define BSE_SIGNAL_FREQ_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-7)
/**
 * Inverse variant of BSE_SIGNAL_FREQ_CHANGED().
 */
#define BSE_SIGNAL_FREQ_EQUALS(v1,v2)	(!BSE_SIGNAL_FREQ_CHANGED (v1, v2))

/**
 * Value changes in signals which represent modulation.
 */
#define BSE_SIGNAL_MOD_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/**
 * Value changes in signals which represent dB ranges.
 */
#define BSE_SIGNAL_GAIN_CHANGED(v1,v2)	(fabs ((v1) - (v2)) > 1e-8)

/**
 * Convert between literal frequencies and signal values.
 */
#define BSE_SIGNAL_TO_FREQ_FACTOR	(BSE_MAX_FREQUENCY)
#define BSE_SIGNAL_FROM_FREQ_FACTOR	(1.0 / BSE_MAX_FREQUENCY)
#define BSE_SIGNAL_TO_FREQ(value)	(BSE_FREQ_FROM_VALUE (value))
#define BSE_SIGNAL_FROM_FREQ(freq)	(BSE_VALUE_FROM_FREQ (freq))

#define BSE_SIGNAL_CLIP(v)      bse_signal_value_clip (v)

static inline double   bse_signal_value_clip (double x)  G_GNUC_CONST;
static inline double G_GNUC_CONST
bse_signal_value_clip (double x)
{
  if (G_UNLIKELY (x > 1.0))
    return 1.0;
  if (G_UNLIKELY (x < -1.0))
    return -1.0;
  return x;
}


/* --- frequency modulation --- */
typedef struct {
  gfloat	fm_strength;		/* linear: 0..1, exponential: n_octaves */
  guint		exponential_fm : 1;
  gfloat	signal_freq;		/* for ifreq == NULL (as BSE_SIGNAL_FROM_FREQ) */
  gint		fine_tune;		/* -100..+100 */
} BseFrequencyModulator;

void	bse_frequency_modulator	(const BseFrequencyModulator	*fm,
				 guint				 n_values,
				 const gfloat			*ifreq,
				 const gfloat			*ifmod,
				 gfloat				*fm_buffer);


/* --- windows --- */
double	bse_window_bartlett	(double x);	/* narrowest */
double	bse_window_blackman	(double x);
double	bse_window_cos		(double x);
double	bse_window_hamming	(double x);
double	bse_window_sinc		(double x);
double	bse_window_rect		(double x);	/* widest */


/* --- function approximations --- */

/**
 * @param x	x as in atan(x)
 *
 * Fast atan(x)/(PI/2) approximation, with maximum error < 0.01 and
 * bse_approx_atan1(0)==0, according to the formula:
 * n1 = -0.41156875521951602506487246309908;
 * n2 = -1.0091272542790025586079663559158;
 * d1 = 0.81901156857081841441890603235599;
 * d2 = 1.0091272542790025586079663559158;
 * positive_atan1(x) = 1 + (n1 * x + n2) / ((1 + d1 * x) * x + d2);
 */
static inline double	bse_approx_atan1 	  (double x)  G_GNUC_CONST;

/**
 * @param boost_amount	boost amount between [0..1]
 * @return		prescale factor for bse_approx_atan1()
 *
 * Calculate the prescale factor for bse_approx_atan1(x*prescale) from
 * a linear boost factor, where 0.5 amounts to prescale=1.0, 1.0 results
 * in maximum boost and 0.0 results in maximum attenuation.
 */
double			bse_approx_atan1_prescale (double	   boost_amount);

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the upper right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle1	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the upper left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle2	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the lower left quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle3	  (double x)  G_GNUC_CONST;

/**
 * @param x	x within [0..1]
 * @return	y for circle approximation within [0..1]
 *
 * Fast approximation of the lower right quadrant of a circle.
 * Errors at x=0 and x=1 are zero, for the rest of the curve, the error
 * wasn't minimized, but distributed to best fit the curverture of a
 * quarter circle. The maximum error is below 0.092.
 */
static inline double	bse_approx_qcircle4	  (double x)  G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 0.01275 which corresponds to a sample
 * precision of 6.2 bit, the average error amounts to 0.001914.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 12.81 ns.
 */
static inline double    bse_approx2_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 0.001123415 which corresponds to a sample
 * precision of 9.7 bit, the average error amounts to 0.000133.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 13.74 ns.
 */
static inline double    bse_approx3_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 7.876055e-05 which corresponds to a sample
 * precision of 13.6 bit, the average error amounts to 7.7012792e-06.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 16.46 ns.
 */
static inline double    bse_approx4_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.60807023e-06 which corresponds to a sample
 * precision of 17.7 bit, the average error amounts to 3.842199e-07.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 18.51 ns.
 */
static inline double    bse_approx5_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 2.5505813e-07 which corresponds to a sample
 * precision of 21.9 bit, the average error amounts to 2.1028377e-08.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 21.84 ns.
 */
static inline double    bse_approx6_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.7448985e-09.
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 23.79 ns.
 */
static inline double    bse_approx7_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.6776048e-09.
 * Note that there is no significant precision increment over bse_approx7_exp2().
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 26.59 ns.
 */
static inline double    bse_approx8_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param ex	exponent within [-127..+127]
 * @return	y approximating 2^ex
 *
 * Fast approximation of 2 raised to the power of ex.
 * Within -1..+1, the error stays below 4.1074325e-08 which corresponds to a sample
 * precision of 24.5 bit, the average error amounts to 7.677515903e-09.
 * Note that there is no significant precision increment over bse_approx7_exp2().
 * For integer values of @a ex (i.e. @a ex - floor (@a ex) -> 0), the error
 * approaches zero. On a 2GHz machine, execution takes roughly 29.40 ns.
 */
static inline double    bse_approx9_exp2        (float ex)      G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 0.00436 which corresponds to a sample
 * precision of 7.8 bit, the average error amounts to 0.00069220.
 * On a 2GHz machine, execution takes roughly 24.48 ns.
 */
static inline double    bse_approx2_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 0.0003857 which corresponds to a sample
 * precision of 7.8 bit, the average error amounts to 0.00004827.
 * On a 2GHz machine, execution takes roughly 25.78 ns.
 */
static inline double    bse_approx3_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 2.7017507e-05 which corresponds to a sample
 * precision of 15.1 bit, the average error amounts to 2.799594e-06.
 * On a 2GHz machine, execution takes roughly 28.41 ns.
 */
static inline double    bse_approx4_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 1.582042006e-06 which corresponds to a sample
 * precision of 19.2 bit, the average error amounts to 1.42780810e-07.
 * On a 2GHz machine, execution takes roughly 30.35 ns.
 */
static inline double    bse_approx5_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 9.7878796e-08 which corresponds to a sample
 * precision of 23.2 bit, the average error amounts to 1.3016999e-08.
 * On a 2GHz machine, execution takes roughly 34.29 ns.
 */
static inline double    bse_approx6_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.5028421e-09.
 * On a 2GHz machine, execution takes roughly 36.86 ns.
 */
static inline double    bse_approx7_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.49155722e-09.
 * Note that there is no significant precision increment over bse_approx7_tanh().
 * On a 2GHz machine, execution takes roughly 42.03 ns.
 */
static inline double    bse_approx8_tanh        (float x)       G_GNUC_CONST;

/**
 * @param x	exponent within [-127..+127]
 * @return	y approximating tanh(x)
 *
 * Fast approximation of the hyperbolic tangent of x.
 * Within -1..+1, the error stays below 4.4375671e-08 which corresponds to a sample
 * precision of 24.4 bit, the average error amounts to 9.49141881e-09.
 * Note that there is no significant precision increment over bse_approx7_tanh().
 * On a 2GHz machine, execution takes roughly 43.83 ns.
 */
static inline double    bse_approx9_tanh        (float x)       G_GNUC_CONST;

/**
 * @param value		value to saturate
 * @param limit		limit not to be exceeded by value
 * @return		@a value bounded by -limit and @a limit
 *
 * Clamp @a value within -limit and +limit. Limiting is performed
 * by floating point operations only, thus executing faster than
 * condition based branching code on most modern architectures.
 * On a 2GHz machine, execution takes roughly 6.86 ns.
 */
static inline double    bse_saturate_hard       (double value,
                                                 double limit)  G_GNUC_CONST;

/**
 * @param value		value to saturate
 * @param limit		limit not to be exceeded by value
 * @return		@a value bounded by -limit and @a limit
 *
 * Clamp @a value within -limit and +limit. Limiting is performed
 * by executing conditions and branches, so it will probably run
 * slower than bse_saturate_hard() on many machines.
 * On a 2GHz machine, execution takes roughly 8.29 ns.
 */
static inline double    bse_saturate_branching (double value,
                                                double limit)   G_GNUC_CONST;

/* --- semitone factors (for +-11 octaves) --- */
const double* bse_semitone_table_from_tuning (Bse::MusicalTuning musical_tuning); /* returns [-132..+132] */
double        bse_transpose_factor           (Bse::MusicalTuning musical_tuning, int index /* [-132..+132] */);

/* --- cents (1/100th of a semitone) --- */

double                  bse_cent_tune (double fine_tune);

/**
 * @param fine_tune	fine tuning in cent between -100 and 100
 * @return		a factor corresponding to this
 *
 * This function computes a factor which corresponds to a given fine tuning in
 * cent.  The result can be used as factor for the frequency or the play speed.
 * It is a faster alternative to bse_cent_tune(), and can only deal with
 * integer values between -100 and 100. The input is always CLAMPed to ensure
 * that it lies in this range.
 */
static inline double	bse_cent_tune_fast (int fine_tune /* -100..+100 */)   G_GNUC_CONST;

/* --- implementation details --- */
static inline double  G_GNUC_CONST
bse_approx_atan1 (double x)
{
  if (x < 0)	/* make use of -atan(-x)==atan(x) */
    {
      double numerator, denominator = -1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * 0.41156875521951602506487246309908; /* -n1 */
      denominator *= x;
      numerator  += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return -1.0 - numerator / denominator;
    }
  else
    {
      double numerator, denominator = 1.0;

      denominator += x * 0.81901156857081841441890603235599; /* d1 */
      numerator = x * -0.41156875521951602506487246309908; /* n1 */
      denominator *= x;
      numerator  += -1.0091272542790025586079663559158; /* n2 */
      denominator += 1.0091272542790025586079663559158; /* d2 */

      return 1.0 + numerator / denominator;
    }
  /* atan1_positive(x)=1+(x*-0.411568755219516-1.009127254279)/((1+x*0.81901156857)*x+1.009127254279)
   * atan1(x)=x<0 ? -atan1_positive(-x) : atan1_positive(x)
   */
}

static inline double	G_GNUC_CONST
bse_approx_qcircle1 (double x)
{
  double numerator = 1.20460124790369468987715633298929 * x - 1.20460124790369468987715633298929;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R1(x)=(1.2046012479036946898771563 * x - 1.2046012479036946898771563) / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle2 (double x)
{
  double numerator = 1.20460124790369468987715633298929*x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R2(x)=1.2046012479036946898771563*x/(x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle3 (double x)
{
  double numerator = 0.20460124790369468987715633298929 - 0.20460124790369468987715633298929 * x;
  double denominator = x + 0.20460124790369468987715633298929;
  /* R3(x)=(0.2046012479036946898771563 - 0.2046012479036946898771563 * x) / (x + 0.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double	G_GNUC_CONST
bse_approx_qcircle4 (double x)
{
  double numerator = -0.20460124790369468987715633298929 * x;
  double denominator = x - 1.20460124790369468987715633298929;
  /* R4(x)=-0.2046012479036946898771563 * x / (x - 1.2046012479036946898771563) */
  return numerator / denominator;
}

static inline double G_GNUC_CONST
bse_approx2_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333)));
}

static inline double G_GNUC_CONST
bse_approx3_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622))));
  /* exp2frac(x)=x-ftoi(x)
   * exp2a3(x)=2**ftoi(x)*(1+exp2frac(x)*(0.6931471805599453+exp2frac(x)*(0.2402265069591+exp2frac(x)*0.0555041086648)))
   */
}

static inline double G_GNUC_CONST
bse_approx4_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589)))));
  /* ftoi(x)=int(x<-0.0 ? x - 0.5 : x + 0.5)
   * exp2frac(x)=x-ftoi(x)
   * exp2a4(x)=2**ftoi(x)*(1+exp2frac(x)*(0.6931471805599453+exp2frac(x)*(0.2402265069591+exp2frac(x)*(0.0555041086648+exp2frac(x)*0.009618129107628477))))
   */
}

static inline double G_GNUC_CONST
bse_approx5_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996))))));
}

static inline double G_GNUC_CONST
bse_approx6_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742)))))));
}

static inline double G_GNUC_CONST
bse_approx7_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201))))))));
}

static inline double G_GNUC_CONST
bse_approx8_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201 +
                                                                x * (0.0000013215486790144309488403758228288)))))))));
}

static inline double G_GNUC_CONST
bse_approx9_exp2 (float ex)
{
  BseFloatIEEE754 fp = { 0, };
  int i = bse_ftoi (ex);
  fp.mpn.biased_exponent = BSE_FLOAT_BIAS + i;
  double x = ex - i;
  return fp.v_float * (1.0 + x * (0.69314718055994530941723212145818 +
                                  x * (0.24022650695910071233355126316333 +
                                       x * (0.055504108664821579953142263768622 +
                                            x * (0.0096181291076284771619790715736589 +
                                                 x * (0.0013333558146428443423412221987996 +
                                                      x * (0.00015403530393381609954437097332742 +
                                                           x * (0.00001525273380405984028002543901201 +
                                                                x * (0.0000013215486790144309488403758228288 +
                                                                     x * 0.00000010178086009239699727490007597745)))))))));
}

static inline double G_GNUC_CONST
bse_approx2_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx3_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx4_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
  /* tanha4(x)=x<-20 ? -1 : x>20 ? 1 : (exp2a4(x*2.885390081777926814719849362)-1) / (exp2a4(x*2.885390081777926814719849362)+1) */
}

static inline double G_GNUC_CONST
bse_approx5_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx6_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx7_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx8_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_approx9_tanh (float x)
{
  if (G_UNLIKELY (x < -20))
    return -1;
  if (G_UNLIKELY (x > 20))
    return 1;
  double bpot = Bse::fast_exp2 (x * BSE_2_DIV_LN2);
  return (bpot - 1) / (bpot + 1);
}

static inline double G_GNUC_CONST
bse_saturate_hard (double value,
                   double limit)
{
  double v1 = fabs (value + limit);
  double v2 = fabs (value - limit);
  return 0.5 * (v1 - v2); /* CLAMP() without branching */
}

static inline double G_GNUC_CONST
bse_saturate_branching (double value,
                        double limit)
{
  if (G_UNLIKELY (value >= limit))
    return limit;
  if (G_UNLIKELY (value <= limit))
    return -limit;
  return value;
}

extern const double * const bse_cent_table;

static inline double G_GNUC_CONST
bse_cent_tune_fast (int fine_tune)
{
  return bse_cent_table[CLAMP (fine_tune, -100, 100)];
}

#endif /* __BSE_SIGNAL_H__ */
