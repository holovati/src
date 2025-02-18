/*	$NetBSD: mkboot.c,v 1.15 2024/02/24 15:34:47 christos Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mkboot.c	8.1 (Berkeley) 7/15/93
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1990, 1993\
The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#ifdef notdef
static char sccsid[] = "@(#)mkboot.c	7.2 (Berkeley) 12/16/90";
#endif
__RCSID("$NetBSD: mkboot.c,v 1.15 2024/02/24 15:34:47 christos Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#ifndef HAVE_NBTOOL_CONFIG_H
#include <sys/endian.h>
#endif

#include <time.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "volhdr.h"

#define LIF_NUMDIR	8

#define LIF_VOLSTART	0
#define LIF_VOLSIZE	sizeof(struct lifvol)
#define LIF_DIRSTART	512
#define LIF_DIRSIZE	(LIF_NUMDIR * sizeof(struct lifdir))
#define LIF_FILESTART	8192

#define btolifs(b)	(((b) + (SECTSIZE - 1)) / SECTSIZE)
#define lifstob(s)	((s) * SECTSIZE)

int	loadpoint = -1;
struct  load ld;
struct	lifvol lifv;
struct	lifdir lifd[LIF_NUMDIR];
time_t repro_epoch = 0;

int	 main(int, char **);
void	 bcddate(char *, char *);
char	*lifname(char *);
int	 putfile(char *, int);
void	 usage(void);

#define CLEAR(a, b, c)	\
__CTASSERT(sizeof(b) - 1 == c); \
memcpy((a), (b), (c))

/*
 * Old Format:
 *	sector 0:	LIF volume header (40 bytes)
 *	sector 1:	<unused>
 *	sector 2:	LIF directory (8 x 32 == 256 bytes)
 *	sector 3-:	LIF file 0, LIF file 1, etc.
 * where sectors are 256 bytes.
 *
 * New Format:
 *	sector 0:	LIF volume header (40 bytes)
 *	sector 1:	<unused>
 *	sector 2:	LIF directory (8 x 32 == 256 bytes)
 *	sector 3:	<unused>
 *	sector 4-31:	disklabel (~300 bytes right now)
 *	sector 32-:	LIF file 0, LIF file 1, etc.
 */
int
main(int argc, char **argv)
{
	char *n1, *n2, *n3;
	int n, to, ch;
	int count;


	while ((ch = getopt(argc, argv, "l:t:")) != -1)
		switch (ch) {
		case 'l':
			loadpoint = strtol(optarg, NULL, 0);
			break;
		case 't':
			repro_epoch = (time_t)atoll(optarg);
			break;
		default:
			usage();
		}

	argc -= optind;
	argv += optind;
	if (loadpoint == -1 || argc == 0)
		usage();
	n1 = argv[0];
	argv++;
	argc--;
	if (argc == 0)
		usage();
	if (argc > 1) {
		n2 = argv[0];
		argv++;
		argc--;
		if (argc > 1) {
			n3 = argv[0];
			argv++;
			argc--;
		} else
			n3 = NULL;
	} else
		n2 = n3 = NULL;

	if ((to = open(argv[0], O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1)
		err(1, "Can't open `%s'", argv[0]);
	/* clear possibly unused directory entries */
	CLEAR(lifd[1].dir_name, "          ", sizeof(lifd[1].dir_name));
	lifd[1].dir_type = htobe16(-1);
	lifd[1].dir_addr = htobe32(0);
	lifd[1].dir_length = htobe32(0);
	lifd[1].dir_flag = htobe16(0xFF);
	lifd[1].dir_exec = htobe32(0);
	lifd[7] = lifd[6] = lifd[5] = lifd[4] = lifd[3] = lifd[2] = lifd[1];
	/* record volume info */
	lifv.vol_id = htobe16(VOL_ID);
	CLEAR(lifv.vol_label, "BOOT43", sizeof(lifv.vol_label));
	lifv.vol_addr = htobe32(btolifs(LIF_DIRSTART));
	lifv.vol_oct = htobe16(VOL_OCT);
	lifv.vol_dirsize = htobe32(btolifs(LIF_DIRSIZE));
	lifv.vol_version = htobe16(1);
	/* output bootfile one */
	lseek(to, LIF_FILESTART, SEEK_SET);
	count = putfile(n1, to);
	n = btolifs(count);
	strcpy(lifd[0].dir_name, lifname(n1));
	lifd[0].dir_type = htobe16(DIR_TYPE);
	lifd[0].dir_addr = htobe32(btolifs(LIF_FILESTART));
	lifd[0].dir_length = htobe32(n);
	bcddate(n1, lifd[0].dir_toc);
	lifd[0].dir_flag = htobe16(DIR_FLAG);
	lifd[0].dir_exec = htobe32(loadpoint);
	lifv.vol_length = htobe32(be32toh(lifd[0].dir_addr) +
				  be32toh(lifd[0].dir_length));
	/* if there is an optional second boot program, output it */
	if (n2) {
		lseek(to, LIF_FILESTART+lifstob(n), SEEK_SET);
		count = putfile(n2, to);
		n = btolifs(count);
		strcpy(lifd[1].dir_name, lifname(n2));
		lifd[1].dir_type = htobe16(DIR_TYPE);
		lifd[1].dir_addr = htobe32(lifv.vol_length);
		lifd[1].dir_length = htobe32(n);
		bcddate(n2, lifd[1].dir_toc);
		lifd[1].dir_flag = htobe16(DIR_FLAG);
		lifd[1].dir_exec = htobe32(loadpoint);
		lifv.vol_length = htobe32(be32toh(lifd[1].dir_addr) +
					  be32toh(lifd[1].dir_length));
	}
	/* ditto for three */
	if (n3) {
		lseek(to, LIF_FILESTART+lifstob(lifd[0].dir_length+n),
		      SEEK_SET);
		count = putfile(n3, to);
		n = btolifs(count);
		strcpy(lifd[2].dir_name, lifname(n3));
		lifd[2].dir_type = htobe16(DIR_TYPE);
		lifd[2].dir_addr = htobe32(lifv.vol_length);
		lifd[2].dir_length = htobe32(n);
		bcddate(n3, lifd[2].dir_toc);
		lifd[2].dir_flag = htobe16(DIR_FLAG);
		lifd[2].dir_exec = htobe32(loadpoint);
		lifv.vol_length = htobe32(be32toh(lifd[2].dir_addr) +
					  be32toh(lifd[2].dir_length));
	}
	/* output volume/directory header info */
	lseek(to, LIF_VOLSTART, SEEK_SET);
	write(to, &lifv, LIF_VOLSIZE);
	lseek(to, LIF_DIRSTART, SEEK_SET);
	write(to, lifd, LIF_DIRSIZE);
	return EXIT_SUCCESS;
}

int
putfile(char *from, int to)
{
	int fd;
	struct stat statb;
	void *bp;

	if ((fd = open(from, 0)) < 0)
		err(EXIT_FAILURE, "Unable to open file `%s'", from);
	fstat(fd, &statb);
	ld.address = htobe32(loadpoint);
	ld.count = htobe32(statb.st_size);
	if ((bp = malloc(statb.st_size)) == NULL)
		err(EXIT_FAILURE, "Can't allocate buffer");
	if (read(fd, bp, statb.st_size) < 0)
		err(EXIT_FAILURE, "Error reading from file `%s'", from);
	(void)close(fd);
	write(to, &ld, sizeof(ld));
	write(to, bp, statb.st_size);
	free(bp);
	return (statb.st_size + sizeof(ld));
}

void
usage(void)
{

	fprintf(stderr, "Usage:	%s -l <loadpoint> [-t <timestamp>] prog1 "
	    "[ prog2 ] outfile\n", getprogname());
	exit(EXIT_FAILURE);
}

char *
lifname(char *str)
{
	static char lname[10] = "SYS_XXXXX";
	char *cp;
	int i;

	if ((cp = strrchr(str, '/')) != NULL)
		str = ++cp;
	for (i = 4; i < 9; i++) {
		if (islower((unsigned char)*str))
			lname[i] = toupper((unsigned char)*str);
		else if (isalnum((unsigned char)*str) || *str == '_')
			lname[i] = *str;
		else
			break;
		str++;
	}
	for ( ; i < 10; i++)
		lname[i] = '\0';
	return(lname);
}

void
bcddate(char *name, char *toc)
{
	struct stat statb;
	struct tm *tm;

	if (repro_epoch)
		tm = gmtime(&repro_epoch);
	else {
		stat(name, &statb);
		tm = localtime(&statb.st_ctime);
	}
	*toc = ((tm->tm_mon+1) / 10) << 4;
	*toc++ |= (tm->tm_mon+1) % 10;
	*toc = (tm->tm_mday / 10) << 4;
	*toc++ |= tm->tm_mday % 10;
	*toc = (tm->tm_year / 10) << 4;
	*toc++ |= tm->tm_year % 10;
	*toc = (tm->tm_hour / 10) << 4;
	*toc++ |= tm->tm_hour % 10;
	*toc = (tm->tm_min / 10) << 4;
	*toc++ |= tm->tm_min % 10;
	*toc = (tm->tm_sec / 10) << 4;
	*toc |= tm->tm_sec % 10;
}
