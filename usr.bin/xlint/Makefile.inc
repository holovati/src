#	$NetBSD: Makefile.inc,v 1.28 2024/01/23 19:44:28 rillig Exp $

.include <bsd.own.mk>

.PATH:		${.CURDIR}/../common

# Special cases first, then check for a MACHINE_ARCH specific
# subdir before looking for MACHINE_CPU.
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${.CURDIR}/../arch/${MACHINE_ARCH:C/e[lb]$//})
ARCHSUBDIR=	${MACHINE_ARCH:C/e[lb]$//}
.else
ARCHSUBDIR=	${MACHINE_CPU}
.endif

CPPFLAGS+=	-I${.CURDIR}/../arch/${ARCHSUBDIR}
CPPFLAGS+=	-I${.CURDIR}/../common

CLEANFILES+=	*.gcno *.gcda *.gcov

.if ${MACHINE_CPU:Nalpha:Nvax}
CFLAGS+=	${ACTIVE_CC:Mgcc:%=-ftrapv}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

LINTFLAGS+=	-aa	# warn about all lossy conversions
LINTFLAGS+=	-e	# strict enum mode
LINTFLAGS+=	-T	# strict bool mode
LINTFLAGS+=	-w	# treat warnings as errors
