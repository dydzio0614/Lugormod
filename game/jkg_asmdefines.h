// Cross-platform (Linux & Windows) Asm defines

#ifdef _WIN32
	#define __asm1__( a )		__asm a
	#define __asm2__( a, b )	__asm a, b
	#define __asmL__( a )		__asm a
// Data defines
	#define __asmD__1( b1 )																			__asm _emit b1
	#define __asmD__2( b1, b2 )																		__asm _emit b1 __asm _emit b2
	#define __asmD__3( b1, b2, b3 )																	__asm _emit b1 __asm _emit b2 __asm _emit b3
	#define __asmD__4( b1, b2, b3, b4 )																__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4
	#define __asmD__5( b1, b2, b3, b4, b5 )															__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5
	#define __asmD__6( b1, b2, b3, b4, b5, b6 )														__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6
	#define __asmD__7( b1, b2, b3, b4, b5, b6, b7 )													__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b7
	#define __asmD__8( b1, b2, b3, b4, b5, b6, b7, b8 )												__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8
	#define __asmD__9( b1, b2, b3, b4, b5, b6, b7, b8, b9 )											__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9
	#define __asmD__10( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10 )									__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10
	#define __asmD__11( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11 )								__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11
	#define __asmD__12( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12 )							__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11 __asm _emit b12
	#define __asmD__13( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13 )					__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11 __asm _emit b12 __asm _emit b13
	#define __asmD__14( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14 )				__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11 __asm _emit b12 __asm _emit b13 __asm _emit b14
	#define __asmD__15( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 )			__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11 __asm _emit b12 __asm _emit b13 __asm _emit b14 __asm _emit b15
	#define __asmD__16( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16 )		__asm _emit b1 __asm _emit b2 __asm _emit b3 __asm _emit b4 __asm _emit b5 __asm _emit b6 __asm _emit b6 __asm _emit b8 __asm _emit b9 __asm _emit b10 __asm _emit b11 __asm _emit b12 __asm _emit b13 __asm _emit b14 __asm _emit b15 __asm _emit b16
// End of data defines
	#define __asmD__( size )	__asmD__##size
	#define __asmB__	__asmD__1

	#define __JKG_StartHook		__asm lea eax, [__hookstart] \
								__asm jmp __hookend \
								__asm __hookstart:
	#define __JKG_EndHook		__asm __hookend:

#else 
	#define __asm1__( a )		__asm__( #a "\n" );
	#define __asm2__( a, b )	__asm__( #a ", " #b "\n" );
	#define __asmL__( a )       __asm__( ".att_syntax\n" ); \
								__asm__( #a ":\n" ); \
								__asm__( ".intel_syntax noprefix\n" );
// Data defines
	#define __asmD__1( b1 )																			__asm__( ".byte " #b1 "\n" );
	#define __asmD__2( b1, b2 )																		__asm__( ".byte " #b1 ", " #b2 "\n" );
	#define __asmD__3( b1, b2, b3 )																	__asm__( ".byte " #b1 ", " #b2 ", " #b3 "\n" );
	#define __asmD__4( b1, b2, b3, b4 )																__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 "\n" );
	#define __asmD__5( b1, b2, b3, b4, b5 )															__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 "\n" );
	#define __asmD__6( b1, b2, b3, b4, b5, b6 )														__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 "\n" );
	#define __asmD__7( b1, b2, b3, b4, b5, b6, b7 )													__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 "\n" );
	#define __asmD__8( b1, b2, b3, b4, b5, b6, b7, b8 )												__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 "\n" );
	#define __asmD__9( b1, b2, b3, b4, b5, b6, b7, b8, b9 )											__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 "\n" )
	#define __asmD__10( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10 )									__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 "\n" )
	#define __asmD__11( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11 )								__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 "\n" )
	#define __asmD__12( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12 )							__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 ", " #b12 "\n" )
	#define __asmD__13( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13 )					__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 ", " #b12 ", " #b13 "\n" )
	#define __asmD__14( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14 )				__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 ", " #b12 ", " #b13 ", " #b14 "\n" )
	#define __asmD__15( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 )			__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 ", " #b12 ", " #b13 ", " #b14 ", " #b15 "\n" )
	#define __asmD__16( b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16 )		__asm__( ".byte " #b1 ", " #b2 ", " #b3, ", " #b4 ", " #b5 ", " #b6 ", " #b7 ", " #b8 ", " #b9 ", " #b10 ", " #b11 ", " #b12 ", " #b13 ", " #b14 ", " #b15 ", " #b16 "\n" )
// End of data defines
	#define __asmB__			__asmD__1
	#define __asmD__( size )	__asmD__##size

	#define __JKG_StartHook		__asm__( ".intel_syntax noprefix\n" ); \
								__asm__("lea eax, [__hookstart]\n"); \
								__asm__("jmp __hookend\n"); \
								__asm__(".att_syntax\n"); \
								__asm__("__hookstart:\n"); \
								__asm__(".intel_syntax noprefix\n");
	
	#define __JKG_EndHook		__asm__(".att_syntax\n"); \
								__asm__("__hookend:\n"); \
								__asm__(".intel_syntax noprefix\n");
#endif