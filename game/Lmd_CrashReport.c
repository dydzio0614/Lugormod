//[CrashLog]
#include "g_local.h"

//For debugging in GCC
//#define NO_LINUX_CRASHHANDLER

//#define LMD_USE_MINIDUMP



#if defined __linux__ && !defined GNUC
#ifndef NO_LINUX_CRASHHANDLER
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#define __USE_GNU
#include <link.h>
//RoboPhred: this header file includes __sighandler_t, but gcc fails to compile it.
//The ojp file I copied this file from had sys/ucontext.h from here.  The non sys/. variety seems a generic gnu c library
//also includes features.h

//Ahah, here is the issue, need to include this now.
#include <signal.h>

#include <ucontext.h>
//#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ == 1
#define GLIBC_21
#endif

extern char *strsignal (int __sig) __THROW;

//use sigaction instead.
__sighandler_t INTHandler (int signal, struct sigcontext ctx);
void CrashHandler(int signal, siginfo_t *siginfo, ucontext_t *ctx);
void (*OldHandler)(int signal);	
struct sigaction oldact[NSIG];

int segvloop = 0;
#endif

void installcrashhandler(void) {
#ifndef NO_LINUX_CRASHHANDLER
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	memset(&oldact, 0, sizeof(oldact));
	act.sa_sigaction = (void *)CrashHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &act, &oldact[SIGSEGV]); 
	sigaction(SIGILL, &act, &oldact[SIGILL]); 
	sigaction(SIGFPE, &act, &oldact[SIGFPE]); 
	sigaction(SIGBUS, &act, &oldact[SIGBUS]);
#endif
}

void restorecrashhandler(void) {
#ifndef NO_LINUX_CRASHHANDLER
	sigaction(SIGSEGV, &oldact[SIGSEGV], NULL); 
#endif
}

void installinthandler(void) {
#ifndef NO_LINUX_CRASHHANDLER
	// Trap Ctrl-C
	signal(SIGINT, (void *)INTHandler);
#endif
}

#ifndef NO_LINUX_CRASHHANDLER
void linux_siginfo(int signal, siginfo_t *siginfo) {
	G_LogPrintf("Signal: %s (%d)\n", strsignal(signal), signal); 
	G_LogPrintf("Siginfo: %p\n", siginfo);
	if(siginfo) {
		G_LogPrintf("Code: %d\n", siginfo->si_code);
		G_LogPrintf("Faulting Memory Ref/Instruction: %p\n",siginfo->si_addr);
	}
}

// tjw: i'm disabling etpub_dsnoinfo() because it depends on
//      glibc 2.3.3 only  (it also won't build on earlier glibc 2.3
//      versions).
//      We're only doing glibc 2.1.3 release builds anyway.
void linux_dsoinfo(void) {
	struct link_map *linkmap = NULL;
	ElfW(Ehdr)      * ehdr = (ElfW(Ehdr) *)0x08048000;
	ElfW(Phdr)      * phdr;
	ElfW(Dyn) *dyn;
	struct r_debug *rdebug = NULL;

	phdr = (ElfW(Phdr) *)((char *)ehdr + ehdr->e_phoff);

	while (phdr++<(ElfW(Phdr) *)((char *)phdr + (ehdr->e_phnum * sizeof(ElfW(Phdr)))))
		if (phdr->p_type == PT_DYNAMIC)
			break;

	for (dyn = (ElfW(Dyn) *)phdr->p_vaddr; dyn->d_tag != DT_NULL; dyn++)
		if (dyn->d_tag == DT_DEBUG) {
			rdebug = (void *)dyn->d_un.d_ptr;
			break;
		}

		linkmap = rdebug->r_map;

		//rewind to top item.
		while(linkmap->l_prev)
			linkmap=linkmap->l_prev;

		G_LogPrintf("DSO Information:\n");

		while(linkmap) {

			if(linkmap->l_addr) {

				if(strcmp(linkmap->l_name,"")==0) 
					G_LogPrintf("0x%08x\t(unknown)\n", linkmap->l_addr);
				else
					G_LogPrintf("0x%08x\t%s\n", linkmap->l_addr, linkmap->l_name);

			}			

			linkmap=linkmap->l_next;

		}
}

void linux_backtrace(ucontext_t *ctx) {

	// See <asm/sigcontext.h>	

	// ctx.eip contains the actual value of eip 
	// when the signal was generated.

	// ctx.cr2 contains the value of the cr2 register 
	// when the signal was generated.

	// the cr2 register on i386 contains the address 
	// that caused the page fault if there was one.

	int i;

	char **strings;
	void *array[1024];
	size_t size = (size_t)backtrace(array, 1024);

	//Set the actual calling address for accurate stack traces.
	//If we don't do this stack traces are less accurate.
#ifdef GLIBC_21
	G_LogPrintf("Stack frames: %Zd entries\n", size-1);
	array[1] = (void *)ctx->uc_mcontext.gregs[EIP];
#else
	G_LogPrintf("Stack frames: %zd entries\n", size-1);
	array[1] = (void *)ctx->uc_mcontext.gregs[REG_EIP];
#endif
	G_LogPrintf("Backtrace:\n");

	strings = (char **)backtrace_symbols(array, (int)size);

	//Start at one and climb up.
	//The first entry points back to this function.
	for (i = 1; i < (int)size; i++)
		G_LogPrintf("(%i) %s\n", i, strings[i]);

	free(strings);
}

__sighandler_t INTHandler(int signal, struct sigcontext ctx) {
	G_LogPrintf("------------------------------------------------\n");
	G_LogPrintf("Ctrl-C is not the proper way to kill the server.\n");
	G_LogPrintf("------------------------------------------------\n");
	return 0;
}

void G_ShutdownGame( int restart );
void CrashHandler(int signal, siginfo_t *siginfo, ucontext_t *ctx) {

	//we are real cautious here.
	restorecrashhandler();

	if(signal == SIGSEGV)
		segvloop++;

	if(segvloop < 2) {
		//RoboPhred
		fileHandle_t oldf = level.logFile;
		char version[MAX_STRING_CHARS];
		trap_Cvar_VariableStringBuffer("Lugormod_Version", version, sizeof(version));
		trap_FS_FOpenFile("CrashLog.txt", &level.logFile, FS_APPEND);
		G_LogPrintf("-8<------- Crash Information ------->8-\n");
		G_LogPrintf(" Please forward to the "GAMEVERSION" developer. \n");
		G_LogPrintf("---------------------------------------\n");
		G_LogPrintf("Version: %s Linux\n", GAMEVERSION);
		G_LogPrintf("Lugormod version: %s\n", version);
		G_LogPrintf("Map: %s\n", level.rawmapname);
		linux_siginfo(signal, siginfo);
		linux_dsoinfo();
		linux_backtrace(ctx);
		G_LogPrintf("-8<--------------------------------->8-\n\n");
		G_LogPrintf("Attempting to clean up.\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = oldf;
		G_ShutdownGame(0);
		//pass control to the default handler.
		if(signal == SIGSEGV) {
			OldHandler = (void *)oldact[SIGSEGV].sa_sigaction;
			(*OldHandler)(signal);
		} else {
			exit(1);
		}
	} else {
		//end this madness we are looping.
		//RoboPhred: G_Error was before the handler revert, Bad
		OldHandler = (void *)oldact[SIGSEGV].sa_sigaction;
		(*OldHandler)(signal);
		G_Error("Recursive segfault. Bailing out.");
	}

	return;

}
#endif

#elif defined WIN32
#include <windows.h>
#include <process.h>
#include <imagehlp.h>

#ifdef LMD_USE_MINIDUMP
#include <time.h>
#include "../WDT/dbghelp.h"
#endif

#ifdef LMD_USE_MINIDUMP

void CreateMiniDump( LPEXCEPTION_POINTERS e, char *filepath){
	//From http://www.codeproject.com/KB/applications/minidump.aspx?fid=127267&df=90&mpp=25&noise=3&sort=Position&view=Quick&select=990308#xx990308xx
	//Changes from http://blogs.msdn.com/joshpoley/archive/2008/05/19/prolific-usage-of-minidumpwritedump-automating-crash-dump-analysis-part-0.aspx
	MINIDUMP_EXCEPTION_INFORMATION minidump_exception;
	MINIDUMP_EXCEPTION_INFORMATION *p = NULL;
	if(e) {
		minidump_exception.ThreadId = GetCurrentThreadId();	
		minidump_exception.ExceptionPointers = e;	
		minidump_exception.ClientPointers = FALSE;
		p = &minidump_exception;
	}
	HANDLE hProcess = GetCurrentProcess();	
	DWORD ProcessId = GetProcessId(hProcess);
	SYSTEMTIME sysTime = {0};
    GetSystemTime(&sysTime);
	char *path = va("%s\\%04u-%02u-%02u_%02u-%02u-%02u.dmp", filepath, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	HANDLE hFile = CreateFile(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile) {
		// interesting options:  MiniDumpNormal, MiniDumpWithDataSegs, MiniDumpWithFullMemory
		MiniDumpWriteDump(hProcess, ProcessId, hFile, (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs), p, NULL, NULL);
	}
}

#endif


HMODULE	imagehlp = NULL;

typedef BOOL (WINAPI *PFNSYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL (WINAPI *PFNSYMCLEANUP)(HANDLE);
typedef PGET_MODULE_BASE_ROUTINE PFNSYMGETMODULEBASE;
typedef BOOL (WINAPI *PFNSTACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
typedef BOOL (WINAPI *PFNSYMGETSYMFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (WINAPI *PFNSYMENUMERATEMODULES)(HANDLE, PSYM_ENUMMODULES_CALLBACK, PVOID); 
typedef PFUNCTION_TABLE_ACCESS_ROUTINE PFNSYMFUNCTIONTABLEACCESS;

PFNSYMINITIALIZE pfnSymInitialize = NULL;
PFNSYMCLEANUP pfnSymCleanup = NULL;
PFNSYMGETMODULEBASE pfnSymGetModuleBase = NULL;
PFNSTACKWALK pfnStackWalk = NULL;
PFNSYMGETSYMFROMADDR pfnSymGetSymFromAddr = NULL;
PFNSYMENUMERATEMODULES pfnSymEnumerateModules = NULL;
PFNSYMFUNCTIONTABLEACCESS pfnSymFunctionTableAccess = NULL;

/*
Visual C 7 Users, place the PDB file generated with the build into your ensimod 
directory otherwise your stack traces will be useless.

Visual C 6 Users, shouldn't need the PDB file since the DLL will contain COFF symbols.

Watch this space for mingw.
*/

BOOL CALLBACK EnumModules( LPSTR ModuleName, DWORD BaseOfDll, PVOID UserContext ) {
	G_LogPrintf("0x%08x\t%s\n", BaseOfDll, ModuleName);
	return TRUE;
}

char *ExceptionName(DWORD exceptioncode){
	switch (exceptioncode){
			case EXCEPTION_ACCESS_VIOLATION: return "Access violation"; break;
			case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment"; break;
			case EXCEPTION_BREAKPOINT: return "Breakpoint"; break;
			case EXCEPTION_SINGLE_STEP: return "Single step"; break;
			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded"; break;
			case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float denormal operand"; break;
			case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float divide by zero"; break;
			case EXCEPTION_FLT_INEXACT_RESULT: return "Float inexact result"; break;
			case EXCEPTION_FLT_INVALID_OPERATION: return "Float invalid operation"; break;
			case EXCEPTION_FLT_OVERFLOW: return "Float overflow"; break;
			case EXCEPTION_FLT_STACK_CHECK: return "Float stack check"; break;
			case EXCEPTION_FLT_UNDERFLOW: return "Float underflow"; break;
			case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer divide by zero"; break;
			case EXCEPTION_INT_OVERFLOW: return "Integer overflow"; break;
			case EXCEPTION_PRIV_INSTRUCTION: return "Privileged instruction"; break;
			case EXCEPTION_IN_PAGE_ERROR: return "In page error"; break;
			case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal instruction"; break;
			case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception"; break;
			case EXCEPTION_STACK_OVERFLOW: return "Stack overflow"; break;
			case EXCEPTION_INVALID_DISPOSITION: return "Invalid disposition"; break;
			case EXCEPTION_GUARD_PAGE: return "Guard page"; break;
			case EXCEPTION_INVALID_HANDLE: return "Invalid handle"; break;
			default: break;
	}
	return "Unknown exception";
}

void win32_exceptioninfo(LPEXCEPTION_POINTERS e) {
	G_LogPrintf("Exception: %s (0x%08x)\n", ExceptionName(e->ExceptionRecord->ExceptionCode), e->ExceptionRecord->ExceptionCode); 
	G_LogPrintf("Exception Address: 0x%08x\n", e->ExceptionRecord->ExceptionAddress);
}

void win32_dllinfo(void) {
	G_LogPrintf("DLL Information:\n");
	pfnSymEnumerateModules(GetCurrentProcess( ), (PSYM_ENUMMODULES_CALLBACK)EnumModules, NULL);
}

void win32_backtrace(LPEXCEPTION_POINTERS e) {
	PIMAGEHLP_SYMBOL pSym;
	STACKFRAME sf;
	HANDLE process, thread;
	DWORD dwModBase, Disp;
	BOOL more = FALSE;
	int cnt = 0;
	char modname[MAX_PATH] = "";

	pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

	ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = e->ContextRecord->Eip;
	sf.AddrStack.Offset = e->ContextRecord->Esp;
	sf.AddrFrame.Offset = e->ContextRecord->Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	process = GetCurrentProcess();
	thread = GetCurrentThread();

	G_LogPrintf("Backtrace:\n");

	while(1) {

		more = pfnStackWalk(
			IMAGE_FILE_MACHINE_I386,
			process,
			thread,
			&sf,
			e->ContextRecord,
			NULL,
			pfnSymFunctionTableAccess,
			pfnSymGetModuleBase,
			NULL
			);

		if(!more || sf.AddrFrame.Offset == 0) {
			break;
		}

		dwModBase = pfnSymGetModuleBase(process, sf.AddrPC.Offset);

		if(dwModBase) {
			GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
		} else {
			wsprintf(modname, "Unknown");
		}

		pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		pSym->MaxNameLength = MAX_PATH;

		if(pfnSymGetSymFromAddr(process, sf.AddrPC.Offset, &Disp, pSym))
			G_LogPrintf("(%d) %s(%s+%#0x) [0x%08x]\n", cnt, modname, pSym->Name, Disp, sf.AddrPC.Offset);
		else 
			G_LogPrintf("(%d) %s [0x%08x]\n", cnt, modname, sf.AddrPC.Offset);

		cnt++;
	}

	GlobalFree(pSym);
}



//RoboPhred:

#if 0
void CreateMiniDump(LPEXCEPTION_POINTERS e){
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	char szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			_tcscpy( pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szDumpPath[_MAX_PATH];
			char szScratch [_MAX_PATH];

			// work out a good place for the dump file
			if (!GetTempPath( _MAX_PATH, szDumpPath ))
				_tcscpy( szDumpPath, "c:\\temp\\" );

			_tcscat( szDumpPath, m_szAppName );
			_tcscat( szDumpPath, ".dmp" );

			// ask the user if they want to save a dump file
			if (::MessageBox( NULL, "Something bad happened in your program, would you like to save a diagnostic file?", m_szAppName, MB_YESNO )==IDYES)
			{
				// create the file
				HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
					if (bOK)
					{
						sprintf( szScratch, "Saved dump file to '%s'", szDumpPath );
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
					}
					else
					{
						sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					::CloseHandle(hFile);
				}
				else
				{
					sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	if (szResult)
		::MessageBox( NULL, szResult, m_szAppName, MB_OK );

	return retval;
}
#endif

#if 0
//http://www.debuginfo.com/articles/effminidumps2.html#sampleapplications

///////////////////////////////////////////////////////////////////////////////
// This function determines whether we need data sections of the given module 
//

bool IsDataSectionNeeded( const WCHAR* pModuleName ) 
{
	// Check parameters 

	if( pModuleName == 0 ) 
	{
		return false; 
	}


	// Extract the module name 

	WCHAR szFileName[_MAX_FNAME] = L""; 

	_wsplitpath( pModuleName, NULL, NULL, szFileName, NULL ); 


	// Compare the name with the list of known names and decide 

	if( wcsicmp( szFileName, L"jampded" ) == 0 || wcsicmp( szFileName, L"jampgamex86" ) == 0  ||
		wcsicmp( szFileName, L"ntdll" ) == 0)
		return true; 

	// Complete 

	return false; 

}

///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback 
//

BOOL CALLBACK MyMiniDumpCallback(
								 PVOID                            pParam, 
								 const PMINIDUMP_CALLBACK_INPUT   pInput, 
								 PMINIDUMP_CALLBACK_OUTPUT        pOutput 
								 ) 
{
	BOOL bRet = FALSE; 


	// Check parameters 

	if( pInput == 0 ) 
		return FALSE; 

	if( pOutput == 0 ) 
		return FALSE; 


	// Process the callbacks 

	switch( pInput->CallbackType ) 
	{
	case IncludeModuleCallback: 
		{
			// Include the module into the dump 
			bRet = TRUE; 
		}
		break; 

	case IncludeThreadCallback: 
		{
			// Include the thread into the dump 
			bRet = TRUE; 
		}
		break; 

	case ModuleCallback: 
		{
			// Are data sections available for this module ? 

			if( pOutput->ModuleWriteFlags & ModuleWriteDataSeg ) 
			{
				// Yes, they are, but do we need them? 

				if( !IsDataSectionNeeded( pInput->Module.FullPath ) ) 
				{
					pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg); 
				}
			}

			bRet = TRUE; 
		}
		break; 

	case ThreadCallback: 
		{
			// Include all thread information into the minidump 
			bRet = TRUE;  
		}
		break; 

	case ThreadExCallback: 
		{
			// Include this information 
			bRet = TRUE;  
		}
		break; 

	case MemoryCallback: 
		{
			// We do not include any information here -> return FALSE 
			bRet = FALSE; 
		}
		break; 

		//case CancelCallback: 
		//	break; 
	}

	return bRet; 

}

void CreateMiniDump( LPEXCEPTION_POINTERS e ){
	// Open the file 
	char basepath[MAX_STRING_CHARS], gamepath[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof(basepath));
	trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof(gamepath));
	HANDLE hFile = CreateFile(va("%s\\%s\\MidiDump.dmp", basepath, gamepath), GENERIC_READ | GENERIC_WRITE, 
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 

	if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) ) {
		// Create the minidump 

		MINIDUMP_EXCEPTION_INFORMATION mdei; 

		mdei.ThreadId           = GetCurrentThreadId(); 
		mdei.ExceptionPointers  = e; 
		mdei.ClientPointers     = FALSE; 

		MINIDUMP_CALLBACK_INFORMATION mci; 

		//mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback; 
		//mci.CallbackParam       = 0; 

		MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(
			//MiniDumpWithPrivateReadWriteMemory
			MiniDumpWithDataSegs
			//| MiniDumpWithHandleData
			//| MiniDumpWithFullMemory 
			//| MiniDumpWithIndirectlyReferencedMemory
			| MiniDumpWithProcessThreadData

			//this is giving me empty dmp files, or at least hudge but useless ones.
			//| MiniDumpScanMemory //filter out data with MyMiniDumpCallback

			//MiniDumpWithUnloadedModules
			); 

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (e != 0) ? &mdei : 0, 0, NULL /*&mci*/);


		// Close the file 

		CloseHandle( hFile ); 

	}
}

#endif








void G_ShutdownGame( int restart );
LONG CALLBACK win32_exception_handler(LPEXCEPTION_POINTERS e) {
	char basepath[MAX_STRING_CHARS];
	char gamepath[MAX_STRING_CHARS];
	char version[MAX_STRING_CHARS];
	//RoboPhred
	fileHandle_t old;
	old = level.logFile;
	trap_FS_FOpenFile("CrashLog.txt", &level.logFile, FS_APPEND);
	trap_Cvar_VariableStringBuffer("Lugormod_Version", version, sizeof(version));
	//search path for symbols...
	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof(basepath));
	trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof(gamepath));
	strcpy(basepath, va("%s\\%s", basepath, gamepath));
	pfnSymInitialize(GetCurrentProcess(), basepath, TRUE);
	G_LogPrintf("-8<----- Crash Information ----->8-\n");
	G_LogPrintf(" Please forward to the developer. \n");
	G_LogPrintf("-----------------------------------\n");
	G_LogPrintf("Version: %s Win32\n", GAMEVERSION);
	G_LogPrintf("Lugormod version: %s\n", version);
	G_LogPrintf("Map: %s\n",level.rawmapname);
	win32_exceptioninfo(e);
	win32_dllinfo();
	win32_backtrace(e);

#ifdef LMD_USE_MINIDUMP
	CreateMiniDump(e, basepath);
#endif

	G_LogPrintf("-8<--------------------------------->8-\n\n");
	G_LogPrintf("Attempting to clean up.\n");
	trap_FS_FCloseFile(level.logFile);
	level.logFile = old;
	G_ShutdownGame(0);
	pfnSymCleanup(GetCurrentProcess());
	return 1;
}

void win32_initialize_handler(void) {

	imagehlp = LoadLibrary("IMAGEHLP.DLL");
	if(!imagehlp) {
		G_LogPrintf("imagehlp.dll unavailable\n");
		return;
	}

	pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress(imagehlp, "SymInitialize");
	pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress(imagehlp, "SymCleanup");
	pfnSymGetModuleBase = (PFNSYMGETMODULEBASE) GetProcAddress(imagehlp, "SymGetModuleBase");
	pfnStackWalk = (PFNSTACKWALK) GetProcAddress(imagehlp, "StackWalk");
	pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) GetProcAddress(imagehlp, "SymGetSymFromAddr");
	pfnSymEnumerateModules = (PFNSYMENUMERATEMODULES) GetProcAddress(imagehlp, "SymEnumerateModules");
	pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS) GetProcAddress(imagehlp, "SymFunctionTableAccess");

	if(
		!pfnSymInitialize ||
		!pfnSymCleanup ||
		!pfnSymGetModuleBase ||
		!pfnStackWalk ||
		!pfnSymGetSymFromAddr ||
		!pfnSymEnumerateModules ||
		!pfnSymFunctionTableAccess
		) {
			FreeLibrary(imagehlp);
			G_LogPrintf("imagehlp.dll missing exports.\n");
			return;
	}

	// Install exception handler
	SetUnhandledExceptionFilter(win32_exception_handler);
}

void win32_deinitialize_handler(void) {
	// Deinstall exception handler
	SetUnhandledExceptionFilter(NULL);
	pfnSymInitialize = NULL;
	pfnSymCleanup = NULL;
	pfnSymGetModuleBase = NULL;
	pfnStackWalk = NULL;
	pfnSymGetSymFromAddr = NULL;
	pfnSymEnumerateModules = NULL;
	pfnSymFunctionTableAccess = NULL;
	FreeLibrary(imagehlp);
}

#else //other platforms
//FIXME: Get rich, buy a mac, figure out how to do this on OSX.
#endif

void EnableCoreDumps() {
#if defined __linux__

#elif defined WIN32

#else

#endif
}

void DisableCoreDump() {
#if defined __linux__

#elif defined WIN32

#else

#endif
}

void EnableStackTrace() {
#if defined __linux__ && !defined GNUC
	installcrashhandler();
#elif defined WIN32		
	win32_initialize_handler();
#else

#endif
}

void DisableStackTrace() {
#if defined __linux__ && !defined GNUC
	restorecrashhandler();
#elif defined WIN32
	win32_deinitialize_handler();
#else

#endif
}
//[/CrashLog]
