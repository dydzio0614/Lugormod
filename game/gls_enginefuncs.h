/////////////////////////////////
//
//  Engine function definitions
//        - Header -
//
/////////////////////////////////

void (*Com_Printf)( const char *fmt, ... );
void (*Com_DPrintf)( const char *fmt, ... );
const char *(*Cmd_Argv)( int arg );
int Cmd_Argc();
char	* __cdecl va( char *format, ... );

typedef int fileHandle_t;
typedef enum {qfalse, qtrue}	qboolean;

typedef struct cvar_s {
	char		*name;
	char		*string;
	char		*resetString;		// cvar_restart will reset to this value
	char		*latchedString;		// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;			// set each time the cvar is changed
	int			modificationCount;	// incremented each time the cvar is changed
	float		value;				// atof( string )
	int			integer;			// atoi( string )
	struct cvar_s *next;
	struct cvar_s *hashNext;
} cvar_t;

typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

int	(*FS_FOpenFileByMode)( const char *qpath, fileHandle_t *f, fsMode_t mode );
void (*FS_FCloseFile)( fileHandle_t f );
int	(*FS_Write)	( const void *buffer, int len, fileHandle_t f );