







//Ufo: discarded
#ifdef _PATCHER
#include "g_local.h"
 #ifdef _WIN32
 static const char *(*Cmd_Argv)( int arg ) = (const char *( * )(int))0x40F490;
 #else
 static const char *(*Cmd_Argv)( int arg ) = (const char *( * )(int))0x812C264;
 #endif

//
// SNIPPLET FROM JKA GALAXIES SOURCE CODE
//

#include "../disasm/disasm.h"


#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
	#include <string.h>
	#include <stdlib.h>
	typedef unsigned char byte;
	#define _stricmp strcasecmp
#endif

typedef struct {
	int addr;
	int size;
	char origbytes[24];
} PatchData_t;

typedef enum {
	PATCH_JUMP,
	PATCH_CALL,
} PatchType_e;


int CanPatch() {
	if (trap_Cvar_VariableIntegerValue("lmd_disablepatcher") != 0) {
		return 0;
	}

	char ver[1024] = {0};
	trap_Cvar_VariableStringBuffer("version", ver, sizeof(ver));
	//if (strncmp((char *)0x4A88AC, "(internal)JAmp:", 15) != 0)
	if (strncmp(ver, "(internal)JAmp:", 15) != 0) {
		return 0;
	}

	return 1;
}

// ==================================================
// UnlockMemory (WIN32 & Linux compatible)
// --------------------------------------------------
// Makes the memory at address writable for at least
// size bytes.
// Returns 1 if successful, returns 0 on failure.
// ==================================================
static int UnlockMemory(int address, int size) {
	int ret;
	int dummy;
#ifdef _WIN32
	ret = VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, (PDWORD)&dummy);
	return (ret != 0);
#else
	// Linux is a bit more tricky
	int page1, page2;
	page1 = address & ~( getpagesize() - 1);
	page2 = (address+size) & ~( getpagesize() - 1);
	if( page1 == page2 ) {
		ret = mprotect((char *)page1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
	} else {
		ret = mprotect((char *)page1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
		if (ret) return 0;
		ret = mprotect((char *)page2, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
		return (ret == 0);
	}
#endif
}

// ==================================================
// LockMemory (WIN32 & Linux compatible)
// --------------------------------------------------
// Makes the memory at address read-only for at least
// size bytes.
// Returns 1 if successful, returns 0 on failure.
// ==================================================
static int LockMemory(int address, int size) {
	int ret;
#ifdef _WIN32
	ret = VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READ, NULL);
	return (ret != 0);
#else
	// Linux is a bit more tricky
	int page1, page2;
	page1 = address & ~( getpagesize() - 1);
	page2 = (address+size) & ~( getpagesize() - 1);
	if( page1 == page2 ) {
		ret = mprotect((char *)page1, getpagesize(), PROT_READ | PROT_EXEC);
	} else {
		ret = mprotect((char *)page1, getpagesize(), PROT_READ | PROT_EXEC);
		if (ret) return 0;
		ret = mprotect((char *)page2, getpagesize(), PROT_READ | PROT_EXEC);
		return (ret == 0);
	}
#endif
}

// ==================================================
// JKG_PlacePatch (WIN32 & Linux compatible)
// --------------------------------------------------
// Patches the code at address to make it go towards
// destination.
// The instruction used is either JMP or CALL, 
// depending on the type specified.
//
// Before the code is modified, the code page is
// unlocked. If you wish to stop it from being locked
// again, specify 1 for nolock
//
// This function returns a malloced PatchData_t.
// To remove the patch, call JKG_RemovePatch. This
// will also free the PatchData_t.
// ==================================================

static PatchData_t *JKG_PlacePatch( int type, unsigned int address, unsigned int destination ) {
	PatchData_t *patch = (PatchData_t *)malloc(sizeof(PatchData_t));
	t_disasm dasm;
	int addr = address;
	int sz = 0;
	int opsz;
	// Disassemble the code and determine the size of the code we have to replace
	while (sz < 5) {
		opsz = Disasm((char*)addr, 16, addr, &dasm, DISASM_CODE);
		if (opsz == 0) {
			return NULL;	// Should never happen
		}
		sz += opsz;
		addr += opsz;
	}
	if (sz == 0 || sz > 24) {
		// This really shouldnt ever happen, in the worst case scenario,
		// the block is 20 bytes (4 + 16), so if we hit 24, something went wrong
		return NULL;
	}
	patch->addr = address;
	patch->size = sz;
	memcpy(patch->origbytes, (const void *)address, sz);
	UnlockMemory(address, sz); // Make the memory writable
	*(unsigned char *)address = type == PATCH_JUMP ? 0xE9 : 0xE8;
	*(unsigned int *)(address+1) = destination - (address + 5);
	memset((void *)(address+5),0x90,sz-5);	// Nop the rest
	LockMemory(address, sz);
	return patch;
}

static void JKG_RemovePatch(PatchData_t **patch) {
	if (!*patch)
		return;
	UnlockMemory((*patch)->addr, (*patch)->size);
	memcpy((void *)(*patch)->addr, (*patch)->origbytes, (*patch)->size);
	LockMemory((*patch)->addr, (*patch)->size);
	*patch = 0;
}

// ==================================================
// Shell System for Hooks (By BobaFett & Deathspike)
// --------------------------------------------------
// This system, that I (BobaFett) call:
// The Shell System, allows for hooks to be created
// on both windows and linux with exactly the same code
// (OS specific asm changes not included)
//
// The system works as follows:
// Since compilers have the tendancy to add prologue
// and epilogue code to functions, we put our asm
// inside a safe 'shell'
// The shell is defined by:
//
// void *MyHook()
// {
//		__JKG_StartHook;			<-- Shell
//		{
//			// Hook code here		<-- Contents of shell
//		}
//		__JKG_EndHook;				<-- Shell
// }
//
// This code should be placed in a function returning
// a void *, as shown above.
// When called, it will return the pointer to the
// shell's contents, which can then be used to place
// hooks (ie. jumps).
//
// Note that the shell's contents (the hook in question)
// are not executed!
//
// For the actual asm, 3 defines are available:
// __asm1__ for zero/single operand opcodes	(push 10	)
// __asm2__ for dual operand opcodes		(mov eax, 1	)
// __asmL__ for labels						(mylabel:	)
//
// To compile this code on linux, you require the
// following in the gcc command line:
//  -masm=intel
// 
// NOTE: The hook's execution flow must NEVER get to
//       the shell layer! Always ensure the code is
//       ended with a jump or a return!
//
// ==================================================

#include "jkg_asmdefines.h"

// =================================================
// Hook 2:
// Custom anti-q3infoboom patch
// -------------------------------------------------
// Because Luigi Auriemma's 'fix' has side-effects
// such as userinfo being cut off from the connect
// packet. JKG implements a custom protection
// against this issue.
//
// This will also undo the fix from Luigi
//
// The hook is placed in SV_ConnectionlessPacket
// The patch is inside MSG_ReadStringLine
// =================================================
//
#ifdef __linux__
// Define linux symbols
#define _IBFIX_MSGPATCH 0x807803D
#define _IBFIX_PATCHPOS 0x8056E23
#define _IBFIX_QSTRICMP 0x807F434
#else
// Define windows symbols
#define _IBFIX_MSGPATCH 0x418B2C
#define _IBFIX_PATCHPOS 0x443F7F
#define _IBFIX_QSTRICMP 0x41B8A0
#endif

static PatchData_t *pIBFix;

void JKG_CheckConnectionlessPacket(const char *cmd);
static void *_Hook_InfoBoomFix()
{
	__JKG_StartHook;
	{
		__asm1__(	pushad									);	// Secure registers
		__asm1__(	push	ebx								);	// Secure registers
		__asm1__(	call	JKG_CheckConnectionlessPacket	);
		__asm2__(	add		esp, 4							);
		__asm1__(	popad									);
		__asm1__(	push	_IBFIX_QSTRICMP					);
		__asm1__(	ret										);
	}
	__JKG_EndHook;
}

static void JKG_CheckConnectionlessPacket(const char *cmd) {
	char *s;

	if (!_stricmp(cmd,"getstatus") || !_stricmp(cmd,"getinfo")) {
		// We got a risky function here, get arg 1 and do a cutoff if needed
		s = (char *)Cmd_Argv(1);
		if (strlen(s) > 32) {
			// POSSIBLE TODO: Add a check for malicious use and take action (ie. ban)
			s[32] = 0;	// 32 chars should be more than enough for the challenge number
		}
	} else if (!_stricmp(cmd,"connect")) {
		s = (char *)Cmd_Argv(1);
		//RoboPhred: was 980, changed to 511
		if (strlen(s) > 511) {
			s[511] = 0;
		}
	}
}

void JKG_PatchEngine() {
	
	if (!CanPatch()) {
		Com_Printf("Skipping engine patch: Disabled or engine not supported.");
		return;
	}

	Com_Printf(" ------- Installing Engine Patches -------- \n");

	pIBFix = JKG_PlacePatch(PATCH_CALL, _IBFIX_PATCHPOS, (unsigned int)_Hook_InfoBoomFix()); // We'll be overwriting a call here
	if (!pIBFix) {
		Com_Printf("Warning: Failed to place hook 2: Q3infoboom fix\n");
	}

	 ///////////////////////////////
	// Patch 2: Revert the patch of Luigi (in case its patched)
	///////////////////////////////
	UnlockMemory(_IBFIX_MSGPATCH,1);
	*(unsigned int *)_IBFIX_MSGPATCH = (unsigned int)0x3FF;
	LockMemory(_IBFIX_MSGPATCH,1);


	//RoboPhred: Snippet from boba: cause crash on internal G_Error for crashlog and auto-restart purposes
	UnlockMemory(0x00410AC1, 4);
	*(unsigned int *)0x00410AC1 = 0;
	LockMemory(0x00410AC1, 4);



	Com_Printf("Finished\n");
}

void JKG_UnpatchEngine() {
	if (CanPatch()) {
		JKG_RemovePatch(&pIBFix);
	}
}
#endif //_PATCHER