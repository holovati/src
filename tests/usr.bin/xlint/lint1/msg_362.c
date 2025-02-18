/*	$NetBSD: msg_362.c,v 1.2 2024/03/03 13:09:23 rillig Exp $	*/
# 3 "msg_362.c"

// Test for message: directive '%.*s' should not be escaped [362]

/*
 * Since the characters used for the directive type are chosen to be easily
 * readable, it doesn't make sense to obfuscate them.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+9: warning: directive '\142' should not be escaped [362] */
	/* expect+8: warning: bit position 'o' in '\142old-style-lsb\0' should be escaped as octal or hex [369] */
	/* expect+7: warning: bit position 'o' (111) in '\142old-style-lsb\0' out of range 0..63 [371] */
	/* expect+6: warning: unknown directive '\001', must be one of 'bfF=:*' [374] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "\142old-style-lsb\0"
	    "\001old-style-lsb\0"
	    "\142\000old-style-lsb\0",
	    u32);
}
