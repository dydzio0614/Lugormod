#include "g_local.h"

qboolean ParseField(char **sourcep, char *dest, char start, char stop) {
        if (start) {
                while (**sourcep && **sourcep != start && **sourcep != '}') {
                        (*sourcep)++;
                }
                if (**sourcep == '\0' || **sourcep == '}') {
                        dest[0] = '\0';
                        return qfalse;
                }
                (*sourcep)++;
        }
                
        int i_cv = 0;
        while (**sourcep && **sourcep != stop && (**sourcep != '}' || start))
        {
                if (**sourcep != '\n' && **sourcep != '\r' 
                    && **sourcep != '\t') {
                        dest[i_cv] = **sourcep;
                        i_cv++;
                }
                (*sourcep)++;
        }
        if (**sourcep == '\0' || **sourcep == '}') {
                //dest[0] = '\0';
                dest[i_cv] = '\0'; //Might aswell return what we got
                return qfalse;
        }
        
        dest[i_cv] = '\0';
        (*sourcep)++;
        return qtrue;
                
}

qboolean NextSection (char **sourcep){
        while (**sourcep && **sourcep != '{') {
                (*sourcep)++;
        }
        if (**sourcep) {
                (*sourcep)++;
                return qtrue;
        }
        return qfalse;
}
