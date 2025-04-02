#ifndef _MATH_H
#define _MATH_H

#include <sys/cdefs.h>

__BEGIN_DECLS

# define M_E		2.7182818284590452354	/* e */
# define M_LOG2E	1.4426950408889634074	/* log_2 e */
# define M_LOG10E	0.43429448190325182765	/* log_10 e */
# define M_LN2		0.69314718055994530942	/* log_e 2 */
# define M_LN10		2.30258509299404568402	/* log_e 10 */
# define M_PI		3.14159265358979323846	/* pi */
# define M_PI_2		1.57079632679489661923	/* pi/2 */
# define M_PI_4		0.78539816339744830962	/* pi/4 */
# define M_1_PI		0.31830988618379067154	/* 1/pi */
# define M_2_PI		0.63661977236758134308	/* 2/pi */
# define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
# define M_SQRT2	1.41421356237309504880	/* sqrt(2) */
# define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

# define M_Ef		2.7182818284590452354f	/* e */
# define M_LOG2Ef	1.4426950408889634074f	/* log_2 e */
# define M_LOG10Ef	0.43429448190325182765f	/* log_10 e */
# define M_LN2f		0.69314718055994530942f	/* log_e 2 */
# define M_LN10f	2.30258509299404568402f	/* log_e 10 */
# define M_PIf		3.14159265358979323846f	/* pi */
# define M_PI_2f	1.57079632679489661923f	/* pi/2 */
# define M_PI_4f	0.78539816339744830962f	/* pi/4 */
# define M_1_PIf	0.31830988618379067154f	/* 1/pi */
# define M_2_PIf	0.63661977236758134308f	/* 2/pi */
# define M_2_SQRTPIf	1.12837916709551257390f	/* 2/sqrt(pi) */
# define M_SQRT2f	1.41421356237309504880f	/* sqrt(2) */
# define M_SQRT1_2f	0.70710678118654752440f	/* 1/sqrt(2) */

# define M_El		2.718281828459045235360287471352662498L /* e */
# define M_LOG2El	1.442695040888963407359924681001892137L /* log_2 e */
# define M_LOG10El	0.434294481903251827651128918916605082L /* log_10 e */
# define M_LN2l		0.693147180559945309417232121458176568L /* log_e 2 */
# define M_LN10l	2.302585092994045684017991454684364208L /* log_e 10 */
# define M_PIl		3.141592653589793238462643383279502884L /* pi */
# define M_PI_2l	1.570796326794896619231321691639751442L /* pi/2 */
# define M_PI_4l	0.785398163397448309615660845819875721L /* pi/4 */
# define M_1_PIl	0.318309886183790671537767526745028724L /* 1/pi */
# define M_2_PIl	0.636619772367581343075535053490057448L /* 2/pi */
# define M_2_SQRTPIl	1.128379167095512573896158903121545172L /* 2/sqrt(pi) */
# define M_SQRT2l	1.414213562373095048801688724209698079L /* sqrt(2) */
# define M_SQRT1_2l	0.707106781186547524400844362104849039L /* 1/sqrt(2) */

extern double fabs(double x);

extern int isnan(double d);
extern int isinf(double d);

double sin(double d) __THROW;
double cos(double d) __THROW;
double tan(double d) __THROW;

double acos(double d) __THROW;
double atan(double d) __THROW;

double floor(double x);
double ceil(double x);

double pow(double x, double y) __THROW;
double fmod(double x, double y) __THROW;
double sqrt(double x) __THROW;


double atan2(double x, double y) __THROW;

__END_DECLS

#endif