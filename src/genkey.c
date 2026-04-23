// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * Copyright (C) 2015-2026 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#ifndef MAC_OS_X_VERSION_10_12
#define MAC_OS_X_VERSION_10_12 101200
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12
#include <sys/random.h>
#endif
#endif

#include "bee2/core/mem.h"
#include "bee2/crypto/bign128.h"
#include "encoding.h"
#include "subcommands.h"

#ifndef _WIN32
static inline bool __attribute__((__warn_unused_result__)) get_random_bytes(uint8_t *out, size_t len)
{
	ssize_t ret = 0;
	size_t i;
	int fd;

	if (len > 256) {
		errno = EOVERFLOW;
		return false;
	}

#if defined(__OpenBSD__) || (defined(__APPLE__) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12) || (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25)))
	if (!getentropy(out, len))
		return true;
#endif

#if defined(__NR_getrandom) && defined(__linux__)
	if (syscall(__NR_getrandom, out, len, 0) == (ssize_t)len)
		return true;
#endif

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return false;
	for (errno = 0, i = 0; i < len; i += ret, ret = 0) {
		ret = read(fd, out + i, len - i);
		if (ret <= 0) {
			ret = errno ? -errno : -EIO;
			break;
		}
	}
	close(fd);
	errno = -ret;
	return i == len;
}
#else
#include <ntsecapi.h>
static inline bool __attribute__((__warn_unused_result__)) get_random_bytes(uint8_t *out, size_t len)
{
        return RtlGenRandom(out, len);
}
#endif

static void my_bee2_rng(void *buf, size_t count, void *state)
{
	(void)state;
	get_random_bytes((uint8_t *)buf, count);
}

int genkey_main(int argc, const char *argv[])
{
	uint8_t key[WG_KEY_LEN];
	char base64[WG_KEY_LEN_BASE64];
	struct stat stat;

	if (argc != 1) {
		fprintf(stderr, "Usage: %s %s\n", PROG_NAME, argv[0]);
		return 1;
	}

	if (!fstat(STDOUT_FILENO, &stat) && S_ISREG(stat.st_mode) && stat.st_mode & S_IRWXO)
		fputs("Warning: writing to world accessible file.\nConsider setting the umask to 077 and trying again.\n", stderr);

	octet temp_pubkey[WG_PUBKEY_LEN];
	err_t code = bign128KeypairGen(key, temp_pubkey, my_bee2_rng, 0);
	memSetZero(temp_pubkey, sizeof(temp_pubkey));
	if (code != ERR_OK) {
		fprintf(stderr, "%s: Key generation failed (code %d)\n", PROG_NAME, code);
		return 1;
	}

	key_to_base64(base64, key);
	puts(base64);
	return 0;
}
