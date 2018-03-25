/*
 * Copyright (C) 2003 David Sjöstrand  <david@kth.se>
 */


#include "g_local.h"
//Ufo: discarded
#ifdef _NAKEN
//#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/poll.h>
#define MAX_RETRIES 10
//#define RETRY_TIME  2000

int                 tcp_sock  = -2;
//int                 retryTime = 0;
int                 retries   = 0;
extern vmCvar_t     g_nakenPassword;
extern vmCvar_t     g_nakenAddress;
extern vmCvar_t     g_nakenRoom;

typedef struct nakenMsg_s
{
        char *msg;
        struct nakenMsg_s *next;
} nakenMsg_t;

nakenMsg_t nakenQueue = {"",NULL};

void
naken_connect (void);
qboolean naken_done = qfalse;

void
naken_queue(const char *msg) 
{
        if (!naken_done) {
                return;
        }
        nakenMsg_t *newMsg, *p;
        newMsg = (nakenMsg_t*)G_Alloc(sizeof(nakenMsg_t));
        //assert(newMsg);
        
        p = &nakenQueue;
        while (p->next) {
                p = p->next;
        }
        p->next = newMsg;
        newMsg->next = NULL;
        newMsg->msg = G_NewString(msg);
}

qboolean
naken_send (/*const char *name,*/ const char *msg) 
{
        if (!msg) {
                return qfalse;
        }

        if (tcp_sock < 0) {
                //        naken_connect();
                //if (tcp_sock < 0) {
                return qfalse;
                //}
        }
        
        char                buf[MAX_STRING_CHARS];
        //char                cleanname[MAX_NAME_LENGTH + 1];
        buf[0] = 0;
        
        /*
        if (name) {
                Q_strncpyz(cleanname, name, sizeof(cleanname));
                Q_CleanStr(cleanname);
                //Q_strcat(buf, sizeof(buf), ".n ");
                
                //Q_strcat(buf, sizeof(buf), cleanname);        
                //Q_strcat(buf, sizeof(buf), "\n");        
                Q_strcat(buf, sizeof(buf), cleanname);        
        }
        */
        Q_strcat(buf, sizeof(buf), msg);        
        Q_strcat(buf, sizeof(buf), "\n");
        
        buf[sizeof(buf) - 1] = 0;
        Q_CleanStrC(buf);
        if (send(tcp_sock, buf, strlen(buf),
#ifndef MACOS_X
                 MSG_NOSIGNAL|MSG_DONTWAIT
#else
                 0
#endif
                    ) == -1) {
                //if (errno == EPIPE 
                //    || errno == ENOTCONN 
                //    || errno == ECONNRESET) {
                //        close(tcp_sock);
                //        tcp_sock = -2;
                //}
                close(tcp_sock);
                tcp_sock = -2;
                naken_connect();
                return qfalse;
        };
        //Com_Printf("Sent: %s\n", msg);
        return qtrue;
}

qboolean
naken_poll(char *buf, int len, int time) 
{
        if (!buf || !len) {
                return qfalse;
        }
        
        if (tcp_sock < 0) {
                naken_connect();
                if (tcp_sock < 0) {
                        return qfalse;
                }
        }
        
        int            i;
        struct pollfd  p[1];
        //int            fsz;
        //fsz = sizeof (rm_addr);
        p[0].events = POLLIN;
        p[0].fd     = tcp_sock;
        
        buf[0] = 0;
        i = 0;
        
        if (poll(p, 1, time) & POLLIN) {
                if ((//i = recvfrom(tcp_sock, buf, len - 1, 0, 
                     //             (struct sockaddr*)&rm_addr,
                     //             (socklen_t*)&fsz)) == -1){
                            i = recv(tcp_sock, buf, len, 0)) == -1) {
                        return qfalse;
                }
                buf[i]=0;
                //if (i) {
                //        Com_Printf("received: %s", buf);
                //}
                return qtrue;
        }
        return qfalse;
}

int nakenTime = 0;
char nakenCount = 0;

void
naken_run(void) 
{
        if (level.startTime + 8000 > level.time) {
                return;
        }
        
        if (g_nakenAddress.string[0] == 0) {
                return;
        }
        
        char *buf = (char*)G_Alloc(MAX_STRING_CHARS);
        //char buf[MAX_STRING_CHARS];
        //assert(buf);
        int count = 0;
        while (naken_poll(buf, MAX_STRING_CHARS - 1, 1) 
               && count++ < 200) {
                if (!buf[0]) {
                        continue;
                }
                if (/*Q_strncmp(">> You just", buf, 11) == 0 ||*/!naken_done) {
                        naken_done = qtrue;
                        if (g_nakenPassword.string[0]) {
                                naken_queue(va(".P %s",
                                               g_nakenPassword.string));
                                naken_queue(".Y");
                        }
                        naken_queue(va(".c %s",g_nakenRoom.string));
                        naken_queue(".n ja");
                        
                        if (g_nakenPassword.string[0]) {
                                naken_queue(".l");
                        }
                } else if (g_nakenPassword.string[0] &&
                           Q_strncmp(">> Channel is now un", buf, 20) == 0) {
                        naken_queue(".l");
                } /*else if (g_nakenPassword.string[0] &&
                           Q_strncmp(">> Invisible sysop mode of", buf, 26) == 0) {
                        naken_queue(".Y");
                        } */
                
                
        }
        G_Free(buf);
        
        if (nakenTime > level.time) {
                return;
        }
        
        if (nakenQueue.next) {
                if (nakenTime + 2100 < level.time) {
                        nakenCount = 0;
                }
                
                nakenTime = level.time;
                
                if (++nakenCount >= 3) {
                        nakenCount = 0;
                        nakenTime += 2100;
                }
                nakenMsg_t *p;
                p = nakenQueue.next;
                if (naken_send(p->msg)) {       
                        nakenQueue.next = p->next;
                        G_Free(p->msg);
                        G_Free(p);
                }
        }
}

void
naken_free_queue (void) 
{
        nakenMsg_t *p;
        while (nakenQueue.next) {
                p = nakenQueue.next;
                nakenQueue.next = p->next;
                G_Free(p->msg);
                G_Free(p);
        }
}

void
naken_disconnect (void)
{
        if (tcp_sock < 0) {
                return;
        }
        naken_free_queue();
        //naken_send(".q");
        close(tcp_sock);
        tcp_sock = -2;
}

qboolean naken_sock (void) 
{
        if (tcp_sock >= 0) {
                return qtrue;
        }
        tcp_sock = socket(PF_INET, SOCK_STREAM, 0);
        if (tcp_sock < 0) {
                return qfalse;
        }
        return qtrue;
}
#ifdef Q3_VM
long int
strtol (char* string, char* endp, int base) 
{
        char* s = string;
        long int val;
        char digit;
        
        if (s[0] == '-') {
                s++;
        }
        while ((endp == NULL || s <= endp) 
               && (*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z')
                || (*s >= 'A' && *s <= 'Z')) {
                digit = *s;
                if (digit >= '0' && digit <= '9') {
                        digit -= '0';
                } else if (digit >= 'a' && digit <= 'z') {
                        digit -= 'a';
                        digit += 10;
                } else {
                        digit -= 'A';
                        digit += 10;
                }
                if (digit >= base) {
                        break;
                }
                val = base * val + digit;
        }
        if (s[0] == '-') {
                val = -val;
        }
        return val;
}
#endif

void
naken_connect (void)
{
        char                defaultport[] = "6666";
        char               *port;
        char                address[MAX_STRING_CHARS];
        //Com_Printf("debug: naken connect.\n");
        struct sockaddr_in  rm_addr;
        struct hostent     *rm_host;
        
        if (retries++ > MAX_RETRIES) {
                return;
        }
        //if (retryTime > level.time) {
        //        return;
        // }
        if (!naken_sock()) {
                return;
        }
        
        //retryTime = level.time + RETRY_TIME;
        Q_strncpyz(address, g_nakenAddress.string, sizeof(address));
        
        if ((port = Q_strrchr(address,':'))) {
                port[0] = 0;
                port++;
        } else {
                port = defaultport;
        }
        
        rm_host = gethostbyname(address);
        memset(&rm_addr, 0, sizeof rm_addr); //zero out memory space
        rm_addr.sin_family = AF_INET;
        rm_addr.sin_port   = htons(strtol(port,NULL,10));
        rm_addr.sin_addr.s_addr = *(u_int32_t*)rm_host->h_addr_list[0];
        if (connect(tcp_sock,(struct sockaddr*)&rm_addr, sizeof(rm_addr))) {
                close(tcp_sock);
                tcp_sock = -2;
                nakenTime = level.time + 2100;
                return;
        }
        retries = 0;
        nakenTime = level.time + 2100;
}

/*
#else
void naken_send (void) 
{
}

qboolean naken_poll (char **str) 
{
        return qfalse;
}
#endif
*/
#endif //_NAKEN
