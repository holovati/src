/*	$NetBSD: s_sinl.c,v 1.2 2024/01/26 12:32:49 ryoon Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2007 Steven G. Kargl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: s_sinl.c,v 1.2 2024/01/26 12:32:49 ryoon Exp $");

#include "namespace.h"
#include <float.h>
#ifdef __i386__
#include <ieeefp.h>
#endif

#ifdef __weak_alias
__weak_alias(sinl, _sinl)
#endif

#include "math.h"
#include "math_private.h"

#ifdef __HAVE_LONG_DOUBLE

#if LDBL_MANT_DIG == 64
#include "../ld80/e_rem_pio2l.h"
#include "../ld80/k_sinl.c"
#elif LDBL_MANT_DIG == 113
#include "../ld128/e_rem_pio2l.h"
#include "../ld128/k_sinl.c"
#else
#error "Unsupported long double format"
#endif

long double
sinl(long double x)
{
	union ieee_ext_u z;
	int e0, s;
	long double y[2];
	long double hi, lo;

	z.extu_ld = x;
	s = z.extu_sign;
	z.extu_sign = 0;

	/* If x = +-0 or x is a subnormal number, then sin(x) = x */
	if (z.extu_exp == 0)
		return (x);

	/* If x = NaN or Inf, then sin(x) = NaN. */
	if (z.extu_exp == 32767)
		return ((x - x) / (x - x));

	ENTERI();

	/* Optimize the case where x is already within range. */
	if (z.extu_ld < M_PI_4) {
		hi = __kernel_sinl(z.extu_ld, 0, 0);
		RETURNI(s ? -hi : hi);
	}

	e0 = __ieee754_rem_pio2l(x, y);
	hi = y[0];
	lo = y[1];

	switch (e0 & 3) {
	case 0:
	    hi = __kernel_sinl(hi, lo, 1);
	    break;
	case 1:
	    hi = __kernel_cosl(hi, lo);
	    break;
	case 2:
	    hi = - __kernel_sinl(hi, lo, 1);
	    break;
	case 3:
	    hi = - __kernel_cosl(hi, lo);
	    break;
	}
	
	RETURNI(hi);
}

#else

long double
sinl(long double x)
{
	return sin(x);
}

#endif /* __HAVE_LONG_DOUBLE */
