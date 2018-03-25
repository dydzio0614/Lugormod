/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#if 0
#ifdef WITH_JABBER
#ifndef __QTALK_H__
#define __QTALK_H__

#include "q_shared.h"

extern vmCvar_t jb_server;
extern vmCvar_t jb_recipient;
extern vmCvar_t jb_password;
extern vmCvar_t jb_username;

int JB_openConnection   (void);
int JB_Send             (const char *alias, const char *message);
void JB_closeConnection (void);

#endif //__QTALK_H__
#endif //WITH_JABBER
#endif
