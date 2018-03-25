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
#ifdef __linux__
#ifdef WITH_JABBER
#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "g_local.h"

vmCvar_t jb_server;
vmCvar_t jb_recipient;
vmCvar_t jb_password;
vmCvar_t jb_username;
LmConnection *connection;


int
JB_openConnection (void)
{
        //const gchar  *server = NULL;
	//const gchar  *alias  = NULL;
	gchar         groupserver [512];
        const gchar  *resource = "jabber-send";
        //const gchar  *recipient = NULL;
        gchar         message [512],buffer[512];
        //const gchar  *username = NULL;
        //const gchar  *password = NULL;
        guint         port = LM_CONNECTION_DEFAULT_PORT;
        GError       *error = NULL;
        gint          i;
	LmMessage    *m;

	trap_Cvar_Register(&jb_server, "jb_server", "", CVAR_LATCH);
	trap_Cvar_Register(&jb_recipient, "jb_recipient", "", CVAR_LATCH);
	trap_Cvar_Register(&jb_username, "jb_username", "", CVAR_LATCH);
	trap_Cvar_Register(&jb_password, "jb_password", "", CVAR_LATCH);
	
        if (!jb_server.string[0] || !jb_recipient.string[0] || !jb_username.string[0] || !jb_password.string[0]) {
                return -1;
        }
        
        connection = lm_connection_new (jb_server.string);
	
        if (!lm_connection_open_and_block (connection, &error)) {
                //g_error ("Failed to open: %s\n", error->message);
		return -1;
        }
	
	if (!lm_connection_authenticate_and_block (connection,jb_username.string, jb_password.string, resource,&error)) {
		//g_error ("Failed to authenticate: %s\n", error->message);
		return -1;
	}
        
	
	
	m = lm_message_new (NULL , LM_MESSAGE_TYPE_PRESENCE);
        if (!lm_connection_send (connection, m, &error)) {
                //g_error ("Send failed: %s\n", error->message);
		return -1;
        }
        lm_message_unref (m);
        
	//if (alias) {
	strcpy(groupserver, jb_recipient.string);
	strcat(groupserver, "/");
	strcat(groupserver, "JA");
	
	m = lm_message_new (groupserver, LM_MESSAGE_TYPE_PRESENCE);
	if (!lm_connection_send (connection, m, &error)) {
		//g_error ("Send failed: %s\n", error->message);
		return -1;
	}
	lm_message_unref (m);
	
	//}
	return 1;
}

int
JB_Send(const char *alias, const char *message)
{
	GError       *error = NULL;

        if (!jb_server.string[0] || !jb_recipient.string[0] || !jb_username.string[0] || !jb_password.string[0]) {
                return -1;
        }
	
	LmMessage    *m;
	// fix message
	//SanitizeString2(buffer,message);
	
	// send message
	if (alias) {
		m = lm_message_new_with_sub_type (jb_recipient.string, LM_MESSAGE_TYPE_MESSAGE,LM_MESSAGE_SUB_TYPE_GROUPCHAT);
	} else {
		m = lm_message_new (jb_recipient.string, LM_MESSAGE_TYPE_MESSAGE);
	}
	
	lm_message_node_add_child (m->node, "body", (strchr(message, ':') + 2));
	if (!lm_connection_send (connection, m, &error)) {
		//g_error ("Send failed: %s\n", error->message);
		return -1;
		
	}
	
	lm_message_unref (m);
	return 1;
}

void
JB_closeConnection (void)
{
        if (!jb_server.string[0] || !jb_recipient.string[0] || !jb_username.string[0] || !jb_password.string[0]) {
                return;
        }
	lm_connection_close (connection, NULL);
	lm_connection_unref (connection);	
}


#endif //WITH_JABBER
#endif //__linux__
