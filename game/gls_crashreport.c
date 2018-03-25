////////////////////////////////
//							  
// JKA Galaxies crash handler 
//
// In the somewhat unlikely event of a crash
// this code will create a thorough crash log so the cause can be determined
//
////////////////////////////////


#define	GAMEVERSION	"JKA Galaxies v0.1"

int	bCrashing = 0;

#include "../disasm/disasm.h"
#include <time.h>
#include "..\qcommon\disablewarnings.h"

#ifdef _WIN32
	#include <windows.h>
#else
	// TODO: Linux includes
#endif

#include "gls_enginefuncs.h"

static const char *JKG_Crash_GetCrashlogName() {
	static char Buff[1024];
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(Buff, 1024,"LMD-Crashlog_%d-%m-%Y_%H-%M-%S.log",timeinfo);
	return Buff;
}

static void JKG_FS_WriteString(const char *msg, fileHandle_t f) {
	FS_Write(msg, strlen(msg), f);
}
static cvar_t *fs_basepath;
static cvar_t *fs_game;

void UpdateCvars() {
	// Since we need these cvars, we'll just cast em right here
	// Saves us some calls ;)
#ifdef _WIN32
	fs_basepath = *(cvar_t **)0x4FF460;
	fs_game = *(cvar_t **)0x4FF464;
#else
	fs_basepath = *(cvar_t **)0x8386AA0;
	fs_game = *(cvar_t **)0x838AAEC;
#endif
}

#ifdef _WIN32
#include <dbghelp.h>
#include <tlhelp32.h>
#include <psapi.h>
// Windows version of the crash handler
LPTOP_LEVEL_EXCEPTION_FILTER oldHandler = 0;

static void JKG_Crash_AddOSData(fileHandle_t f) {

	//vs2005 header files dont have PRODUCT_*
#if 0 
	char buff[1024] = {0};

	OSVERSIONINFOEXA osvi;
	
	memset(&osvi,0,sizeof(OSVERSIONINFOEXA));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	GetVersionExA((LPOSVERSIONINFOA)&osvi);

	switch(osvi.dwPlatformId) {
		case VER_PLATFORM_WIN32s:
			strncat(buff, "Windows 32s", 1024);	
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
				strncat(buff, "Windows 95", 1024);
				if (osvi.szCSDVersion[0] == 'B' || osvi.szCSDVersion[0] == 'C') 
					strncat(buff, " OSR2", 1024);
			} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
				strncat(buff, "Windows 98", 1024);
				if (osvi.szCSDVersion[0] == 'A') 
					strncat(buff, " SE", 1024);
			} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
				strncat(buff, "Windows ME", 1024);
			} else {
				strncat(buff, "Unknown", 1024);
			}
			break;
		case VER_PLATFORM_WIN32_NT:
			if (osvi.dwMajorVersion <=4) {
				strncat(buff, "Windows NT", 1024);
			} else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
				strncat(buff, "Windows 2000", 1024);
			} else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
				strncat(buff, "Windows XP", 1024);
				if (osvi.wSuiteMask & VER_SUITE_PERSONAL) {
					strncat(buff, " Home", 1024);
				} else {
					strncat(buff, " Professional", 1024);
				}
			} else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2) {
				strncat(buff, "Windows 2003", 1024);
			} else if (osvi.dwMajorVersion == 6) {
				strncat(buff, "Windows Vista", 1024);
				switch (osvi.wProductType) {
					case PRODUCT_ULTIMATE:
					   strncat(buff, " Ultimate Edition", 1024);
					   break;
					case PRODUCT_HOME_PREMIUM:
					   strncat(buff, " Home Premium Edition", 1024);
					   break;
					case PRODUCT_HOME_BASIC:
					   strncat(buff, " Home Basic Edition", 1024);
					   break;
					case PRODUCT_ENTERPRISE:
					   strncat(buff, " Enterprise Edition", 1024);
					   break;
					case PRODUCT_BUSINESS:
					   strncat(buff, " Business Edition", 1024);
					   break;
					case PRODUCT_STARTER:
					   strncat(buff, " Starter Edition", 1024);
					   break;
					case PRODUCT_CLUSTER_SERVER:
					   strncat(buff, " Cluster Server Edition", 1024);
					   break;
					case PRODUCT_DATACENTER_SERVER:
					   strncat(buff, " Datacenter Edition", 1024);
					   break;
					case PRODUCT_DATACENTER_SERVER_CORE:
					   strncat(buff, " Datacenter Edition (core installation)", 1024);
					   break;
					case PRODUCT_ENTERPRISE_SERVER:
					   strncat(buff, " Enterprise Edition", 1024);
					   break;
					case PRODUCT_ENTERPRISE_SERVER_CORE:
					   strncat(buff, " Enterprise Edition (core installation)", 1024);
					   break;
					case PRODUCT_ENTERPRISE_SERVER_IA64:
					   strncat(buff, " Enterprise Edition for Itanium-based Systems", 1024);
					   break;
					case PRODUCT_SMALLBUSINESS_SERVER:
					   strncat(buff, " Small Business Server", 1024);
					   break;
					case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
					   strncat(buff, " Small Business Server Premium Edition", 1024);
					   break;
					case PRODUCT_STANDARD_SERVER:
					   strncat(buff, " Standard Edition", 1024);
					   break;
					case PRODUCT_STANDARD_SERVER_CORE:
					   strncat(buff, " Standard Edition (core installation)", 1024);
					   break;
					case PRODUCT_WEB_SERVER:
					   strncat(buff, " Web Server Edition", 1024);
					   break;
				}
			}
			break;
		default:
			break;
	}

	JKG_FS_WriteString(va("Operating system: %s (%i.%i.%i, SP %i.%i, Type %i)\n", &buff[0], osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.wServicePackMajor, osvi.wServicePackMinor, osvi.wProductType), f);
#endif
}

static int GetModuleNamePtr(void* ptr, char *buffFile, char *buffName, void ** ModuleBase, int * ModuleSize) {
	MODULEENTRY32 M;
	HANDLE	hSnapshot;
	
	// BUFFERS MUST BE (AT LEAST) 260 BYTES!!
	if (buffFile) buffFile[0]=0;
	if (buffName) buffName[0]=0;
	if (ModuleBase) *ModuleBase = 0;
	if (ModuleSize) *ModuleSize = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	
	if ((hSnapshot != INVALID_HANDLE_VALUE) && Module32First(hSnapshot, &M)) {
		do {
			if (ptr > (void *)M.modBaseAddr && ptr <= (void *)(M.modBaseAddr+M.modBaseSize)) {
				if (buffFile) strncpy(buffFile, M.szExePath, MAX_PATH);
				if (buffName) strncpy(buffName, M.szModule, 256);
				if (ModuleBase) *ModuleBase = M.modBaseAddr;
				if (ModuleSize) *ModuleSize = M.modBaseSize;
				CloseHandle(hSnapshot);
				return 1;
			}
		} while (Module32Next(hSnapshot, &M));
	}

	CloseHandle(hSnapshot);
	// No matches found
	return 0;
}

static const char *GetExceptionCodeDescription(int ExceptionCode) {
	switch (ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			return " (Access Violation)";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return " (Array Bounds Exceeded)";
		case EXCEPTION_BREAKPOINT:
			return " (Breakpoint Encountered)";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return " (Datatype Misallignment)";
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return " (Denormal Operand (Floating point operation))";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return " (Division By Zero (Floating point operation))";
		case EXCEPTION_FLT_INEXACT_RESULT:
			return " (Inexact Result (Floating point operation))";
		case EXCEPTION_FLT_INVALID_OPERATION:
			return " (Invalid Operation (Floating point operation))";
		case EXCEPTION_FLT_OVERFLOW:
			return " (Overflow (Floating point operation))";
		case EXCEPTION_FLT_STACK_CHECK:
			return " (Stack Overflow/Underflow (Floating point operation))";
		case EXCEPTION_FLT_UNDERFLOW:
			return " (Underflow (Floating point operation))";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return " (Illegal Instruction)";
		case EXCEPTION_IN_PAGE_ERROR:
			return " (In Page Error)";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return " (Division By Zero (Integer operation))";
		case EXCEPTION_INT_OVERFLOW:
			return " (Overflow (Integer operation))";
		case EXCEPTION_INVALID_DISPOSITION:
			return " (Invalid Disposition)";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return " (Non-Continuable Exception)";
		case EXCEPTION_PRIV_INSTRUCTION:
			return " (Privileged Instruction)";
		case EXCEPTION_SINGLE_STEP:
			return " (Debugger Single Step)";
		case EXCEPTION_STACK_OVERFLOW:
			return " (Stack Overflow)";
		default:
			return "";
	}
}

static void JKG_Crash_AddCrashInfo(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	char buffFile[MAX_PATH] = {0};
	char buffName[MAX_PATH] = {0};
	unsigned int ModuleBase;
	PEXCEPTION_RECORD ER = EI->ExceptionRecord;
	GetModuleFileNameA(NULL, buffFile, MAX_PATH);
	JKG_FS_WriteString(va("Process: %s\n", buffFile),f);
	if (GetModuleNamePtr(ER->ExceptionAddress,buffFile,buffName,(void **)&ModuleBase,NULL)) {
		JKG_FS_WriteString(va("Exception in module: %s\n", buffFile), f);
	} else {
		JKG_FS_WriteString("Exception in module: Unknown\n", f);
	}
	JKG_FS_WriteString(va("Exception Address: 0x%08X (%s+0x%X)\n", ER->ExceptionAddress, buffName, (unsigned int)ER->ExceptionAddress - ModuleBase), f);
	JKG_FS_WriteString(va("Exception Code: 0x%08X%s\n", ER->ExceptionCode, GetExceptionCodeDescription(ER->ExceptionCode)), f);
	if (ER->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ER->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) { // Access violation, show read/write address
		switch (ER->ExceptionInformation[0]) {
			case 0:
				JKG_FS_WriteString(va("Attempted to read data at: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			case 1:
				JKG_FS_WriteString(va("Attempted to write data to: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			case 2:
				JKG_FS_WriteString(va("DEP exception caused attempting to execute: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			default:
				break;
		}
	}

}

static void JKG_Crash_AddRegisterDump(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	PCONTEXT CR = EI->ContextRecord;
	PEXCEPTION_RECORD ER = EI->ExceptionRecord;
	JKG_FS_WriteString("General Purpose & Control Registers:\n", f);
	JKG_FS_WriteString(va("EAX: 0x%08X, EBX: 0x%08X, ECX: 0x%08X, EDX: 0x%08X\n", CR->Eax, CR->Ebx, CR->Ecx, CR->Edx),f);
	JKG_FS_WriteString(va("EDI: 0x%08X, ESI: 0x%08X, ESP: 0x%08X, EBP: 0x%08X\n", CR->Edi, CR->Esi, CR->Esp, CR->Ebp),f);
	JKG_FS_WriteString(va("EIP: 0x%08X\n\n", CR->Eip),f);
	JKG_FS_WriteString("Segment Registers:\n", f);
	JKG_FS_WriteString(va("CS: 0x%08X, DS: 0x%08X, ES: 0x%08X\n", CR->SegCs, CR->SegDs, CR->SegEs), f);
	JKG_FS_WriteString(va("FS: 0x%08X, GS: 0x%08X, SS: 0x%08X\n\n", CR->SegFs, CR->SegGs, CR->SegSs), f);
}

static BOOL CALLBACK JKG_Crash_EnumModules( LPSTR ModuleName, DWORD BaseOfDll, PVOID UserContext ) {
	char Path[MAX_PATH] = {0};
	GetModuleFileName((HMODULE)BaseOfDll, Path, MAX_PATH);
	JKG_FS_WriteString(va("0x%08X - %s - %s\n", BaseOfDll, ModuleName, Path), (fileHandle_t)UserContext);
	return TRUE;
}

static void JKG_Crash_ListModules(fileHandle_t f) {
	SymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)JKG_Crash_EnumModules, (PVOID)f );
}

static void JKG_Crash_DisAsm(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	int addr;
	int sz;
	int disp;
	t_disasm da;
	t_disasm dasym;
	int dmod;
	int i;
	int showsource = 0;
	int lastsourceaddr = 0;
	char modname[260];
	PIMAGEHLP_SYMBOL sym = (PIMAGEHLP_SYMBOL )malloc(1024);
	IMAGEHLP_LINE line;
	MEMORY_BASIC_INFORMATION mem = {0};
	memset(sym, 0, 1024);
	memset(&line, 0, sizeof(IMAGEHLP_LINE));
	sym->MaxNameLength = 800;
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

	if (IsBadReadPtr(EI->ExceptionRecord->ExceptionAddress,16)) {
		JKG_FS_WriteString("ERROR: Exception address invalid, cannot create assembly dump\n\n",f);
		return;
	}
	dmod = SymGetModuleBase(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress);
	if (dmod) {
		GetModuleBaseName(GetCurrentProcess(), (HMODULE)dmod, modname, 260);
	} else {
		strcpy(modname,"Unknown");
	}

	if (SymGetSymFromAddr(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress, (PDWORD)&disp, sym)) {
		// We got a symbol, display info
		JKG_FS_WriteString(va("Crash location located at 0x%08X: %s::%s(+0x%X) [Func at 0x%08X]\n",EI->ExceptionRecord->ExceptionAddress, modname,sym->Name, disp, sym->Address), f);
		// Try to find a source file
		if (SymGetLineFromAddr(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress, (PDWORD)&disp, &line)) {
			if (disp) {
				JKG_FS_WriteString(va("Source code: %s:%i(+0x%X)\n\n", line.FileName, line.LineNumber, disp), f);
			} else {
				JKG_FS_WriteString(va("Source code: %s:%i\n\n", line.FileName, line.LineNumber), f);
			}
			showsource = 1;
		} else {
			JKG_FS_WriteString("No source code information available\n\n", f);
			showsource = 0;
		}
	} else {
		// We don't have a symbol..
		JKG_FS_WriteString(va("Crash location located at 0x%08X: No symbol information available\n\n", EI->ExceptionRecord->ExceptionAddress), f);
	}
	VirtualQuery(EI->ExceptionRecord->ExceptionAddress, &mem, sizeof(MEMORY_BASIC_INFORMATION));
	// Do a 21 instruction disasm, 10 back and 10 forward

	addr = Disassembleback((char *)mem.BaseAddress, (ulong)mem.BaseAddress, (ulong)mem.RegionSize, (ulong)EI->ExceptionRecord->ExceptionAddress, 10);
	JKG_FS_WriteString("^^^^^^^^^^\n", f);
	for(i=0; i<21; i++) {
		sz = Disasm((char *)addr, 16, addr, &da, DISASM_CODE);
		if (sz < 1) {
			JKG_FS_WriteString(va("ERROR: Could not disassemble code at 0x%08X, aborting...\n", addr), f);
			return;
		}
		if (addr == (int)EI->ExceptionRecord->ExceptionAddress) {
			JKG_FS_WriteString("\n=============================================\n", f);
		}
		// Check if this is a new sourcecode line
		if (showsource) {
			if (SymGetLineFromAddr(GetCurrentProcess(), (DWORD)addr, (PDWORD)&disp, &line)) {
				if (line.Address != lastsourceaddr) {
					lastsourceaddr = line.Address;
					if (disp) {
						JKG_FS_WriteString(va("\n--- %s:%i(+0x%X) ---\n\n", line.FileName, line.LineNumber, disp), f);
					} else {
						JKG_FS_WriteString(va("\n--- %s:%i ---\n\n", line.FileName, line.LineNumber), f);
					}
				}
			}
		}
		
		JKG_FS_WriteString(va("0x%08X - %-30s",da.ip, da.result), f);
		if ((da.cmdtype == C_CAL || da.cmdtype == C_JMP || da.cmdtype == C_JMC) && da.jmpconst) {
			// Its a call or jump, see if we got a symbol for it
			// BUT FIRST ;P
			// Since debug compiles employ a call table, we'll disassemble it first
			// if its a jump, we use that address, otherwise, we'll use this one
			if (Disasm((char *)da.jmpconst, 16, da.jmpconst, &dasym, DISASM_CODE) > 0) {
				if (dasym.cmdtype == C_JMP && dasym.jmpconst) {
					// Its a call table
					if (SymGetSymFromAddr(GetCurrentProcess(), dasym.jmpconst, (PDWORD)&disp, sym)) {
						// We got a symbol for it!
						if (disp) {
							JKG_FS_WriteString(va(" (%s+0x%X)", sym->Name, disp), f);
						} else {
							JKG_FS_WriteString(va(" (%s)", sym->Name), f);
						}
					}
				} else {
					// Its not a call table
					if (SymGetSymFromAddr(GetCurrentProcess(), da.jmpconst, (PDWORD)&disp, sym)) {
						// We got a symbol for it!
						if (disp) {
							JKG_FS_WriteString(va(" (%s+0x%X)", sym->Name, disp), f);
						} else {
							JKG_FS_WriteString(va(" (%s)", sym->Name), f);
						}
					}
				}
			}
		}
		if (da.comment[0]) {
			JKG_FS_WriteString(va(" (%s)",da.comment), f);
		}
		if (addr == (int)EI->ExceptionRecord->ExceptionAddress) {
			JKG_FS_WriteString(" <-- Exception\n=============================================\n", f);
		}
		addr+=sz;
		JKG_FS_WriteString("\n", f);
	}
	JKG_FS_WriteString("vvvvvvvvvv\n\n", f);
	free(sym);
	
}

static void JKG_Crash_BackTrace(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	HANDLE proc, thread;
	int frameok;
	PIMAGEHLP_SYMBOL sym = (PIMAGEHLP_SYMBOL)malloc(1024);
	IMAGEHLP_LINE line;
	STACKFRAME sf;
	char ModName[260];
	int dmod;
	int disp;
	int gotsource;
	int sourcedisp;
	CONTEXT ctx = *EI->ContextRecord;	// Copy of the context, since it may be changed
	
	memset(&sf, 0, sizeof(STACKFRAME));
	memset(&line, 0, sizeof(IMAGEHLP_LINE));
	line.SizeOfStruct=sizeof(IMAGEHLP_LINE);
	sf.AddrPC.Offset = EI->ContextRecord->Eip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = EI->ContextRecord->Esp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = EI->ContextRecord->Ebp;
	sf.AddrFrame.Mode = AddrModeFlat;
	memset(sym, 0, 1024);
	sym->MaxNameLength = 800;
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	
	proc = GetCurrentProcess();
	thread = GetCurrentThread();
	while(1) {
		frameok = StackWalk(IMAGE_FILE_MACHINE_I386, proc, thread,&sf,&ctx,NULL,SymFunctionTableAccess,SymGetModuleBase,NULL);
		if (!frameok || !sf.AddrFrame.Offset) {
			break;
		}
		dmod = SymGetModuleBase(proc,sf.AddrPC.Offset);
		if (!dmod) {
			strcpy(ModName,"Unknown");
		} else {
			GetModuleBaseName(proc,(HMODULE)dmod, ModName, 260);
		}
		
		if (SymGetLineFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, (PDWORD)&sourcedisp, &line)) {
			gotsource = 1;
		} else {
			gotsource = 0;
		}

		if (SymGetSymFromAddr(proc,sf.AddrPC.Offset, (PDWORD)&disp, sym)) {
			if (gotsource) {
				JKG_FS_WriteString(va("%s::%s(+0x%X) [0x%08X] - (%s:%i)\n", ModName, sym->Name, disp, sf.AddrPC.Offset, line.FileName, line.LineNumber), f);
			} else {
				JKG_FS_WriteString(va("%s::%s(+0x%X) [0x%08X]\n", ModName, sym->Name, disp, sf.AddrPC.Offset), f);
			}
		} else {
			if (gotsource) {
				// Not likely...
				JKG_FS_WriteString(va("%s [0x%08X] - (%s:%i)\n", ModName, sf.AddrPC.Offset, line.FileName, line.LineNumber), f);
			} else {
				JKG_FS_WriteString(va("%s [0x%08X]\n", ModName, sf.AddrPC.Offset), f);
			}
		}
	}
	free(sym);
	JKG_FS_WriteString("\n",f);
}

static void InitSymbolPath( char * SymbolPath, const char* ModPath )
{
	char Path[1024];

	SymbolPath[0] = 0;	// Clear the buffer
	// Creating the default path
	// ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	strcpy( SymbolPath, "." );

	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
	}

	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariableA( "SYSTEMROOT", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
		strcat( SymbolPath, ";" );

		// SYSTEMROOT\System32
		strcat( SymbolPath, Path );
		strcat( SymbolPath, "\\System32" );
	}

   // Add path of gamedata/JKG
	if ( ModPath != NULL )
		if ( ModPath[0] != '\0' )
		{
			strcat( SymbolPath, ";" );
			strcat( SymbolPath, ModPath );
		}
}
void G_ShutdownGame( int restart );
static void (* Sys_Quit)(void) = (void (*) (void))0x410CB0;
static LONG WINAPI UnhandledExceptionHandler (struct _EXCEPTION_POINTERS *EI /*ExceptionInfo*/) {
	// Alright, we got an exception here, create a crash log and let the program grind to a halt :P
	char SymPath[4096] = {0};
	char basepath[260] = {0};
	char fspath[260] = {0};
	const char *filename = JKG_Crash_GetCrashlogName();
	fileHandle_t f;
	bCrashing = 1;
	InitSymbolPath(SymPath, NULL);
	SymInitialize(GetCurrentProcess(), SymPath, TRUE);
	Com_Printf("------------------------------------------------------------\n\nServer crashed. Creating crash log %s...\n", filename);
	FS_FOpenFileByMode(filename, &f, FS_WRITE);
	JKG_FS_WriteString("========================================\n"
		               "         JKA Galaxies Crash Log\n"
					   "========================================\n", f);
	JKG_FS_WriteString(va("Version: %s (Windows)\n", GAMEVERSION), f);
	JKG_FS_WriteString(va("Side: Server-side\n"), f);
	JKG_FS_WriteString(va("Build Date/Time: %s %s\n", __DATE__, __TIME__), f);
	
	JKG_Crash_AddOSData(f);
	JKG_FS_WriteString("Crash type: Exception\n\n"
					   "----------------------------------------\n"
					   "          Exception Information\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddCrashInfo(EI, f);
	JKG_FS_WriteString("\n"
					   "----------------------------------------\n"
					   "              Register Dump\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddRegisterDump(EI, f);
	JKG_FS_WriteString("----------------------------------------\n"
					   "               Module List\n"
					   "----------------------------------------\n", f);
	JKG_Crash_ListModules(f);
	JKG_FS_WriteString("\n----------------------------------------\n"
					   "          Disassembly/Source code\n"
					   "----------------------------------------\n", f);
	JKG_Crash_DisAsm(EI, f);

	JKG_FS_WriteString("----------------------------------------\n"
					   "                Backtrace\n"
					   "----------------------------------------\n", f);
	JKG_Crash_BackTrace(EI, f);
	JKG_FS_WriteString("========================================\n"
					   "             End of crash log\n"
					   "========================================\n", f);
	FS_FCloseFile(f);
	SymCleanup(GetCurrentProcess());
	Com_Printf("Crash report finished, attempting to shut down...\n");
	Sys_Quit();	// This will call G_ShutdownGame and then shutdown the engine as well
	// Generally speaking, we'll never get here, as Sys_Quit will terminate the process
	return 1;
}

void ActivateCrashHandler() {
	oldHandler = SetUnhandledExceptionFilter(UnhandledExceptionHandler);
}

void DeactivateCrashHandler() {
	if (!oldHandler) return;
	SetUnhandledExceptionFilter(oldHandler);
}



#endif

