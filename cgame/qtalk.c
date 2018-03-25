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

#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= 511)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

void
print_usage (const gchar *exec_name) 
{
        g_print ("Usage: %s -s <server> -u <username> -p <password> -t <recipient> [--port <port>] [-r <resource>]\n", exec_name);
}

int
main (int argc, char **argv)
{
        LmConnection *connection;
        const gchar  *server = NULL;
	const gchar  *alias  = NULL;
	gchar         groupserver [512];
        const gchar  *resource = "jabber-send";
        const gchar  *recipient = NULL;
        gchar         message [512],buffer[512];
        const gchar  *username = NULL;
        const gchar  *password = NULL;
        guint         port = LM_CONNECTION_DEFAULT_PORT;
        GError       *error = NULL;
        gint          i;
	LmMessage    *m;

        for (i = 1; i < argc - 1; ++i) {
                gboolean arg = FALSE;
                
                if (strcmp ("-s", argv[i]) == 0) {
                        server = argv[i+1];
                        arg = TRUE;
                }
		if (strcmp ("-a", argv[i]) == 0) {
                        alias = argv[i+1];
                        arg = TRUE;
                }
                else if (strcmp ("--port", argv[i]) == 0) {
                        port = atoi (argv[i+1]);
                        arg = TRUE;
                }
                /*
                else if (strcmp ("-m", argv[i]) == 0) {
                        message = argv[i+1];
                        arg = TRUE;
                }
                */
                else if (strcmp ("-r", argv[i]) == 0) {
                        resource = argv[i+1];
                        arg = TRUE;
                }
                else if (strcmp ("-t", argv[i]) == 0) {
                        recipient = argv[i+1];
                        arg = TRUE;
                }
                else if (strcmp ("-u", argv[i]) == 0) {
                        username = argv[i+1];
                        arg = TRUE;
                }
                else if (strcmp ("-p", argv[i]) == 0) {
                        password = argv[i+1];
                        arg = TRUE;
                }

                if (arg) {
                        ++i;
                }
        }

        if (!server || !recipient || !username || !password) {
                print_usage (argv[0]);
                return -1;
        }
        
        connection = lm_connection_new (server);

        if (!lm_connection_open_and_block (connection, &error)) {
                g_error ("Failed to open: %s\n", error->message);
        }

	if (!lm_connection_authenticate_and_block (connection,
						   username, password, resource,
						   &error)) {
		g_error ("Failed to authenticate: %s\n", error->message);
	}
        
	
	
	m = lm_message_new (NULL , LM_MESSAGE_TYPE_PRESENCE);
        if (!lm_connection_send (connection, m, &error)) {
                g_error ("Send failed: %s\n", error->message);
        }
        lm_message_unref (m);
        
	if (alias) {
		strcpy(groupserver, recipient);
		strcat(groupserver, "/");
		strcat(groupserver, alias);
		
		m = lm_message_new (groupserver, LM_MESSAGE_TYPE_PRESENCE);
		if (!lm_connection_send (connection, m, &error)) {
			g_error ("Send failed: %s\n", error->message);
		}
		lm_message_unref (m);
		
	}
	
        
	while (fgets(buffer,512,stdin) != NULL) {
		buffer[511] = 0; //incase there were more ..
		
		/* fix message */
		SanitizeString2(buffer,message);
		
                if (message[strlen(message)-1] == '\n') {
                        message[strlen(message)-1] = 0;
                }
                if (strncmp(message, "say: "           ,5 ) == 0 ||
		    strncmp(message, "tell: "          ,6 ) == 0 || 
		    strncmp(message, "ERROR: "         ,7 ) == 0 ||
		    //strncmp(message, "Hitch warning: " ,15) == 0 ||
		    strncmp(message, "info: "          ,6 ) == 0) {
			
			/* send message */
			
			if (alias) {
				
				m = lm_message_new_with_sub_type (recipient, LM_MESSAGE_TYPE_MESSAGE,LM_MESSAGE_SUB_TYPE_GROUPCHAT);
			} else {
				
				
				m = lm_message_new (recipient, LM_MESSAGE_TYPE_MESSAGE);
			}
			
			lm_message_node_add_child (m->node, "body", (strchr(message, ':') + 2));
			if (!lm_connection_send (connection, m, &error)) {
				g_error ("Send failed: %s\n", error->message);
			}
			
			lm_message_unref (m);
		}
		
        }
        
        lm_connection_close (connection, NULL);
        lm_connection_unref (connection);
	
        return 0;
}
