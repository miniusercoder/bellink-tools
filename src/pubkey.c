// SPDX-License-Identifier: GPL-2.0 OR MIT
/*
 * Copyright (C) 2015-2026 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#include <errno.h>
#include <stdio.h>

#include "bee2/crypto/bign128.h"
#include "bee2/core/err.h"
#include "bee2/core/mem.h"
#include "encoding.h"
#include "subcommands.h"
#include "ctype.h"

int pubkey_main(int argc, const char *argv[])
{
	uint8_t key[WG_KEY_LEN] __attribute__((aligned(sizeof(uintptr_t))));
	uint8_t pubkey[WG_PUBKEY_LEN] __attribute__((aligned(sizeof(uintptr_t))));
	char key_base64[WG_KEY_LEN_BASE64];
	char pubkey_base64[WG_PUBKEY_LEN_BASE64];
	int trailing_char;

	if (argc != 1) {
		fprintf(stderr, "Usage: %s %s\n", PROG_NAME, argv[0]);
		return 1;
	}

	if (fread(key_base64, 1, sizeof(key_base64) - 1, stdin) != sizeof(key_base64) - 1) {
		errno = EINVAL;
		fprintf(stderr, "%s: Key is not the correct length or format\n", PROG_NAME);
		return 1;
	}
	key_base64[WG_KEY_LEN_BASE64 - 1] = '\0';

	for (;;) {
		trailing_char = getc(stdin);
		if (!trailing_char || char_is_space(trailing_char))
			continue;
		if (trailing_char == EOF)
			break;
		fprintf(stderr, "%s: Trailing characters found after key\n", PROG_NAME);
		return 1;
	}

	if (!key_from_base64(key, key_base64)) {
		fprintf(stderr, "%s: Key is not the correct length or format\n", PROG_NAME);
		return 1;
	}

		// Вычисляем публичный ключ
	err_t code = bign128PubkeyCalc(pubkey, key);

	if (code == ERR_OK) {
		pubkey_to_base64(pubkey_base64, pubkey);
		puts(pubkey_base64);
	} else if (code == ERR_BAD_PRIVKEY) {
		fprintf(stderr, "%s: Private key is out of range\n", PROG_NAME);
		return 1;
	} else if (code == ERR_BAD_PARAMS) {
		fprintf(stderr, "%s: Internal error with cryptographic parameters\n", PROG_NAME);
		return 1;
	} else {
		fprintf(stderr, "%s: An unexpected error occurred (code %d)\n", PROG_NAME, code);
		return 1;
	}

	return 0;
}
