/* -*- c -*- */
#ifndef __TELEGRAM_TOKEN_H__
#define __TELEGRAM_TOKEN_H__

/* Copyright (C) 2016 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

struct _bson;

/* tokens for bot interaction */
struct telegram_token
{
    unsigned char *_id;
    unsigned char *bot_id;
    int user_id;
    unsigned char *user_login;
    unsigned char *user_name;
    unsigned char *token;
    int contest_id;
    int locale_id;
    time_t expiry_time;
};

struct telegram_token *
telegram_token_free(struct telegram_token *token);
struct telegram_token *
telegram_token_parse_bson(struct _bson *bson);
struct telegram_token *
telegram_token_create(void);
struct _bson *
telegram_token_unparse_bson(const struct telegram_token *token);

#endif

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */