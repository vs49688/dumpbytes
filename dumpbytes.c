/*
 * dumpbytes
 * https://github.com/vs49688/dumpbytes
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Zane van Iperen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "parg.h"

#if defined(_MSC_VER)
#       define ftello   ftell
#       define fseeko   fseek
typedef long off_t;
#endif

typedef struct
{
	int has_from;
	off_t from;

	int has_to;
	off_t to;

	int has_width;
	size_t width;

	const char *fname;
	const char *bufname;
} args_t;

static struct parg_option argdefs[] = {
	{ "from",	PARG_REQARG,	NULL, 'f' },
	{ "to",		PARG_REQARG,	NULL, 't' },
	{ "width",	PARG_REQARG,	NULL, 'w' },
	{ "name",	PARG_REQARG,	NULL, 'n' },
	{ NULL,		0,				NULL, 0 }
};

static const char *USAGE_OPTIONS =
"  -f, --from              from index (default: 0)\n"
"  -t, --to                to index (default: EOF)\n"
"  -w, --width             hex characters per line (default: 8)\n"
"  -n, --name              buffer name (default: buffer)\n"
;

static void write_array(FILE *fp, const char *name, const void *p, size_t size, size_t width)
{
	const uint8_t *b = (const uint8_t*)p;

	fprintf(fp, "const uint8_t %s[%zu] =\n{", name, size);

	for(size_t i = 0; i < size; ++i)
	{
		if(i % width == 0)
			fprintf(fp, "\n\t");

		fprintf(fp, "0x%02X", b[i]);

		if(i != size - 1)
			fprintf(fp, ", ");
	}

	fprintf(fp, "\n};\n");
}

/* Replicate Python's array indexing. */
static off_t calculate_extents(off_t size, off_t *from, off_t *to)
{
	assert(size >= 0 && from != NULL && to != NULL);

	off_t _from = *from;
	off_t _to = *to;

	if(_from < 0)
		_from = size + _from;

	if(_to < 0)
		_to = size + _to;

	if(_from > size)
		_from = size;

	if(_to > size)
		_to = size;

	if((size = _to - _from) < 0)
		size = 0;

	*from = _from;
	*to = _to;
	return size;
}

static int parse_args(int argc, char **argv, args_t *args)
{
	struct parg_state ps;
	parg_init(&ps);

	args->has_from = 0;
	args->from = 0;
	args->has_to = 0;
	args->has_width = 0;
	args->width = 0;
	args->to = 0;
	args->fname = NULL;
	args->bufname = NULL;

	for(int c; (c = parg_getopt_long(&ps, argc, argv, "f:t:w:n:", argdefs, NULL)) != -1; )
	{
		switch(c)
		{
			case 1:
				if(args->fname != NULL)
					return 2;

				args->fname = ps.optarg;
				break;
			case 'f':
				if(args->has_from || sscanf(ps.optarg, "%ld", &args->from) != 1)
					return 2;
				args->has_from = 1;
				break;
			case 't':
				if(args->has_to || sscanf(ps.optarg, "%ld", &args->to) != 1)
					return 2;
				args->has_to = 1;
				break;
			case 'w':
				if(args->has_width || sscanf(ps.optarg, "%zu", &args->width) != 1)
					return 2;
				args->has_width = 1;
				break;
			case 'n':
				if(args->bufname != NULL)
					return 2;
				args->bufname = ps.optarg;
				break;
			case '?':
			default:
				return 2;
		}
	}

	if(!args->fname)
		return 2;

	if(!args->has_width)
		args->width = 8;

	if(!args->bufname)
		args->bufname = "buffer";

	return 0;
}

int main(int argc, char **argv)
{
	int ret;

	args_t args;
	if((ret = parse_args(argc, argv, &args)) != 0)
	{
		fprintf(stderr, "Usage: %s [OPTIONS] <infile>\nOptions:\n", argv[0]);
		fprintf(stderr, "%s", USAGE_OPTIONS);
		return ret;
	}

//	calculate_extents3(end, 0, end);
//	calculate_extents3(end, -1, -1);
//	calculate_extents3(end, -1, -5);
//	calculate_extents3(end, -5, -1);
//	calculate_extents3(end, -5, -5);
//	calculate_extents3(end, 17, -1);
//	calculate_extents3(end, 1000, end);
//	calculate_extents3(end, 12, 17);
//	calculate_extents3(end, 12, 16);
//	calculate_extents3(end, 20, 30);
	off_t end = 0;
	FILE *fp = NULL;
	void *buf = NULL;
	size_t size;

	ret = 1;
	if((fp = fopen(args.fname, "rb")) == NULL)
		goto end;

	if(fseeko(fp, 0, SEEK_END) < 0)
		goto end;

	if((end = ftello(fp)) < 0)
		goto end;

	if(!args.has_from)
		args.from = 0;

	if(!args.has_to)
		args.to = end;

	size = (size_t)calculate_extents(end, &args.from, &args.to);

	if(size > 0)
	{
		if(fseeko(fp, args.from, SEEK_SET) < 0)
			goto end;

		if((buf = malloc(size)) == NULL)
			goto end;

		if(fread(buf, size, 1, fp) != 1)
			goto end;
	}

	fclose(fp);
	fp = NULL;

	write_array(stdout, args.bufname, buf, size, args.width);

	ret = 0;

end:
	if(ret != 0)
		fprintf(stderr, "%s\n", strerror(errno));

	if(buf)
		free(buf);

	if(fp)
		fclose(fp);

	return ret;
}
