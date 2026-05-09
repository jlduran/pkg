/*-
 * Copyright (c) 2016 Brad Davis <brd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>

#include <errno.h>

#include "pkg.h"
#include "private/pkg.h"
#include "private/event.h"

#ifndef ALLPERMS
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

static FILE *metalogfp = NULL;

int
metalog_open(const char *metalog)
{
	metalogfp = fopen(metalog, "ae");
	if (metalogfp == NULL)
		pkg_fatal_errno("Unable to open metalog '%s'", metalog);
	/* Package install scripts may add entries, so avoid interleaving. */
	setvbuf(metalogfp, NULL, _IOLBF, 0);
	return (EPKG_OK);
}

int
metalog_add(int type, const char *path, const char *uname, const char *gname,
    int mode, unsigned long fflags, const char *link)
{
	char *fflags_buffer = NULL;
	const char *type_str;
	int ret = EPKG_FATAL;

	if (metalogfp == NULL)
		goto out;

#ifdef HAVE_FFLAGSTOSTR
	if (fflags) {
		fflags_buffer = fflagstostr(fflags);
	}
#endif

	switch (type) {
	case PKG_METALOG_DIR:
		type_str = "dir";
		break;
	case PKG_METALOG_FILE:
		type_str = "file";
		break;
	case PKG_METALOG_LINK:
		type_str = "link";
		break;
	default:
		goto out;
	}

	if (fprintf(metalogfp, "./%s type=%s uname=%s gname=%s mode=%#o",
	    path, type_str, uname, gname, mode & ALLPERMS) < 0)
		goto err;

	if (type == PKG_METALOG_LINK && link != NULL) {
		if (fprintf(metalogfp, " link=%s", link) < 0)
		goto err;
	}

	if (fprintf(metalogfp, "%s%s\n",
	    fflags ? " flags=" : "",
	    fflags_buffer ? fflags_buffer : "") < 0)
		goto err;

	ret = EPKG_OK;
	goto out;

err:
	pkg_errno("%s", "Unable to write to the metalog");
out:
	free(fflags_buffer);
	return (ret);
}

void
metalog_close(void)
{
	if (metalogfp != NULL) {
		fclose(metalogfp);
	}
}
