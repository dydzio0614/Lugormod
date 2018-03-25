

qboolean Lmd_IPs_CompareIP(IP_t primary, IP_t targ);
qboolean Lmd_IPs_ParseIP(char *str, IP_t ip);
char* Lmd_IPs_IPToString(IP_t ip);

char *Lmd_IPs_GetHostname(IP_t ip);
qboolean Lmd_HostnameIsValid(char *str);
