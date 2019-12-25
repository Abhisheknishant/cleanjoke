/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

// qcommon.h -- definitions common between client and server, but not game.or ref modules

#include "qcommon/q_shared.h"
#include "sys/sys_public.h"

#define DEFAULT_RENDER_LIBRARY "rd-vanilla"

// msg.c

typedef struct msg_s {
	bool	allowoverflow;	// if false, do a Com_Error
	bool	overflowed;		// set to true if the buffer size failed (with allowoverflow set)
	bool	oob;			// set to true if the buffer size failed (with allowoverflow set)
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;				// for bitwise reads and writes
} msg_t;

void MSG_Init (msg_t *buf, byte *data, int length);
void MSG_InitOOB( msg_t *buf, byte *data, int length );
void MSG_Clear (msg_t *buf);
void MSG_WriteData (msg_t *buf, const void *data, int length);
void MSG_Bitstream( msg_t *buf );

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void MSG_WriteBits( msg_t *msg, int value, int bits );

void MSG_WriteChar (msg_t *sb, int c);
void MSG_WriteByte (msg_t *sb, int c);
void MSG_WriteShort (msg_t *sb, int c);
void MSG_WriteLong (msg_t *sb, int c);
void MSG_WriteFloat (msg_t *sb, float f);
void MSG_WriteString (msg_t *sb, const char *s);
void MSG_WriteBigString (msg_t *sb, const char *s);
void MSG_WriteAngle16 (msg_t *sb, float f);

void	MSG_BeginReading (msg_t *sb);
void	MSG_BeginReadingOOB(msg_t *sb);

int		MSG_ReadBits( msg_t *msg, int bits );

int		MSG_ReadChar (msg_t *sb);
int		MSG_ReadByte (msg_t *sb);
int		MSG_ReadShort (msg_t *sb);
int		MSG_ReadLong (msg_t *sb);
float	MSG_ReadFloat (msg_t *sb);
char	*MSG_ReadString (msg_t *sb);
char	*MSG_ReadBigString (msg_t *sb);
char	*MSG_ReadStringLine (msg_t *sb);
float	MSG_ReadAngle16 (msg_t *sb);
void	MSG_ReadData (msg_t *sb, void *buffer, int size);

void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );
void MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );

void MSG_WriteDeltaEntity( msg_t *msg, struct entityState_s *from, struct entityState_s *to
						   , bool force );
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to,
						 int number );

#ifdef _ONEBIT_COMBO
void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to, int *bitComboDelta, int *bitNumDelta );
#else
void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );
#endif
void MSG_ReadDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );

#ifndef FINAL_BUILD
void MSG_ReportChangeVectors_f( void );
#endif

// NET

#define NET_ENABLEV4		0x01

#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	MAX_SNAPSHOT_ENTITIES	256

#define	PORT_ANY			-1

#define	MAX_RELIABLE_COMMANDS	128			// max string commands buffered for restransmit

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

void		NET_Init( void );
void		NET_Shutdown( void );
void		NET_Restart_f( void );
void		NET_Config( bool enableNetworking );

void		NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t to);
void		NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, const char *format, ...);
void		NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );

bool	NET_CompareAdr (netadr_t a, netadr_t b);
bool	NET_CompareBaseAdrMask( netadr_t a, netadr_t b, int netmask );
bool	NET_CompareBaseAdr (netadr_t a, netadr_t b);
bool	NET_IsLocalAddress (netadr_t adr);
const char	*NET_AdrToString (netadr_t a);
bool	NET_StringToAdr ( const char *s, netadr_t *a);
bool	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message);
void		NET_Sleep(int msec);

void		Sys_SendPacket( int length, const void *data, netadr_t to );
//Does NOT parse port numbers, only base addresses.
bool	Sys_StringToAdr( const char *s, netadr_t *a );
bool	Sys_IsLANAddress (netadr_t adr);
void		Sys_ShowIP(void);

#define	MAX_MSGLEN				49152		// max length of a message, which may
											// be fragmented into multiple packets

//rww - 6/28/02 - Changed from 16384 to match sof2's. This does seem rather huge, but I guess it doesn't really hurt anything.

#define MAX_DOWNLOAD_WINDOW			8		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE		2048	// 2048 byte block chunks

/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct netchan_s {
	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			outgoingSequence;

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;
	byte		fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	bool	unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];
} netchan_t;

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

bool Netchan_Process( netchan_t *chan, msg_t *msg );

// PROTOCOL

#define	PROTOCOL_VERSION	26

#define	UPDATE_SERVER_NAME			"updatejk3.ravensoft.com"
#define MASTER_SERVER_NAME			"masterjk3.ravensoft.com"

#define JKHUB_MASTER_SERVER_NAME	"master.jkhub.org"
#define JKHUB_UPDATE_SERVER_NAME	"update.jkhub.org"

#define	PORT_MASTER			29060
#define	PORT_UPDATE			29061
#define	PORT_SERVER			29070	//...+9 more for multiple servers
#define	NUM_SERVER_PORTS	4		// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers

// the svc_strings[] array in cl_parse.c should mirror this
// server to client
enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,			// [short] [string] only in gamestate messages
	svc_baseline,				// only in gamestate messages
	svc_serverCommand,			// [string] to be executed by client game module
	svc_download,				// [short] size [size bytes]
	svc_snapshot,
	svc_setgame,
	svc_mapchange,
	svc_EOF
};

// client to server
enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF
};

// VIRTUAL MACHINE

typedef enum vmSlots_e {
	VM_GAME=0,
	VM_CGAME,
	VM_UI,
	MAX_VM
} vmSlots_t;

typedef struct vm_s {
	vmSlots_t	slot; // VM_GAME, VM_CGAME, VM_UI
    char		name[MAX_QPATH];
	void		*dllHandle;

	// fill the import/export tables
	void *		(*GetModuleAPI)( int apiVersion, ... );
} vm_t;

extern vm_t *currentVM;

class VMSwap {
private:
	VMSwap();
	vm_t *oldVM;
public:
	VMSwap( vm_t *newVM ) : oldVM( currentVM ) { currentVM = newVM; };
	~VMSwap() { if ( oldVM ) currentVM = oldVM; };
};

extern const char *vmStrs[MAX_VM];

void	 VM_Init( void );
vm_t	*VM_Create( vmSlots_t vmSlot );
void	 VM_Free( vm_t *vm );
void	 VM_Clear(void);
vm_t	*VM_Restart( vm_t *vm );
void VM_Shifted_Alloc( void **ptr, int size );
void VM_Shifted_Free( void **ptr );

// CMD
// Command text buffering and command execution
// Any number of commands can be added in a frame, from several different sources.
// Most commands come from either keybindings or console line input, but entire text files can be execed.

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText( const char *text );
// Adds command text at the end of the buffer, does NOT add a final \n

void Cbuf_ExecuteText( int exec_when, const char *text );
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function, or current args will be destroyed.

// Command execution takes a null terminated string, breaks it into tokens, then searches for a command or variable that matches the first token.

typedef void (*xcommand_t)( void );
typedef void (*completionCallback_t)( const char *s );

void	Cmd_Init (void);

void	Cmd_AddCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc=NULL );
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_clientCommand instead of executed locally

void	Cmd_RemoveCommand( const char *cmd_name );
void	Cmd_VM_RemoveCommand( const char *cmd_name, vmSlots_t vmslot );
typedef void (*completionFunc_t)( char *args, int argNum );

typedef struct cmdList_s
{
	const char *name;
	const char *description;
	xcommand_t func;
	completionFunc_t complete;
} cmdList_t;

void Cmd_AddCommandList( const cmdList_t *cmdList );
void Cmd_RemoveCommandList( const cmdList_t *cmdList );

void	Cmd_CommandCompletion( completionCallback_t callback );
// callback with each valid string
void Cmd_SetCommandCompletionFunc( const char *command, completionFunc_t complete );
void Cmd_CompleteArgument( const char *command, char *args, int argNum );
void Cmd_CompleteCfgName( char *args, int argNum );

int		Cmd_Argc (void);
char	*Cmd_Argv (int arg);
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength );
char	*Cmd_Args (void);
char	*Cmd_ArgsFrom( int arg );
void	Cmd_ArgsBuffer( char *buffer, int bufferLength );
void	Cmd_ArgsFromBuffer( int arg, char *buffer, int bufferLength );
char	*Cmd_Cmd (void);
void	Cmd_Args_Sanitize( size_t length = MAX_CVAR_VALUE_STRING, const char *strip = "\n\r;", const char *repl = "   " );
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

void	Cmd_TokenizeString( const char *text );
void	Cmd_TokenizeStringIgnoreQuotes( const char *text_in );
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString( const char *text );
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console

char *Cmd_DescriptionString( const char *cmd_name );

// FILESYSTEM
// No stdio calls should be used by any part of the game, because we need to deal with all sorts of directory and seperator char issues.

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF	0x01
#define FS_UI_REF		0x02
#define FS_CGAME_REF	0x04
#define FS_GAME_REF		0x08
// number of id paks that will never be autodownloaded from base
#define NUM_ID_PAKS		9

#define	MAX_FILE_HANDLES	64

#define DEFAULT_CFG "default.cfg"
#ifdef DEDICATED
#	define Q3CONFIG_CFG PRODUCT_NAME "_server.cfg"
#else
#	define Q3CONFIG_CFG PRODUCT_NAME ".cfg"
#endif

bool FS_Initialized();

void	FS_InitFilesystem (void);
void	FS_Shutdown( bool closemfp );

bool	FS_ConditionalRestart( int checksumFeed );
void	FS_Restart( int checksumFeed );
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

char	**FS_ListFiles( const char *directory, const char *extension, int *numfiles );
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void	FS_FreeFileList( char **fileList );
//rwwRMG - changed to fileList to not conflict with list type

void FS_Remove( const char *osPath );
void FS_HomeRemove( const char *homePath );

void FS_Rmdir( const char *osPath, bool recursive );
void FS_HomeRmdir( const char *homePath, bool recursive );

bool FS_FileExists( const char *file );

char   *FS_BuildOSPath( const char *base, const char *game, const char *qpath );
bool FS_CompareZipChecksum(const char *zipfile);

int		FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int		FS_GetModList(  char *listbuf, int bufsize );

fileHandle_t	FS_FOpenFileWrite( const char *qpath, bool safe=true );
// will properly create any needed paths and deal with seperater character issues

int		FS_filelength( fileHandle_t f );
fileHandle_t FS_SV_FOpenFileWrite( const char *filename );
int		FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp );
void	FS_SV_Rename( const char *from, const char *to, bool safe );
long		FS_FOpenFileRead( const char *qpath, fileHandle_t *file, bool uniqueFILE );
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int		FS_FileIsInPAK(const char *filename, int *pChecksum );
// returns 1 if a file is in the PAK file, otherwise -1

bool FS_FindPureDLL(const char *name);

int		FS_Write( const void *buffer, int len, fileHandle_t f );

int		FS_Read( void *buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

void	FS_FCloseFile( fileHandle_t f );
// note: you can't just fclose from another DLL, due to MS libc issues

long		FS_ReadFile( const char *qpath, void **buffer );
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void	FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

void	FS_FreeFile( void *buffer );
// frees the memory returned by FS_ReadFile

void	FS_WriteFile( const char *qpath, const void *buffer, int size );
// writes a complete file, creating any subdirectories needed

int		FS_filelength( fileHandle_t f );
// doesn't work for files that are opened from a pack file

int		FS_FTell( fileHandle_t f );
// where are we?

void	FS_Flush( fileHandle_t f );

void	FS_FilenameCompletion( const char *dir, const char *ext, bool stripExt, completionCallback_t callback, bool allowNonPureFilesOnDisk );

const char *FS_GetCurrentGameDir(bool emptybase=false);

#ifdef MACOS_X
bool FS_LoadMachOBundle( const char *name );
#endif

void 	QDECL FS_Printf( fileHandle_t f, const char *fmt, ... );
// like fprintf

int		FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

int		FS_Seek( fileHandle_t f, long offset, int origin );
// seek on a file

bool FS_FilenameCompare( const char *s1, const char *s2 );

const char *FS_LoadedPakNames( void );
const char *FS_LoadedPakChecksums( void );
const char *FS_LoadedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char *FS_ReferencedPakNames( void );
const char *FS_ReferencedPakChecksums( void );
const char *FS_ReferencedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded
// AND referenced pk3 files. Servers with sv_pure set will get this string
// back from clients for pure validation

void FS_ClearPakReferences( int flags );
// clears referenced booleans on loaded pk3s

void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames );
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames );
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

bool FS_CheckDirTraversal(const char *checkdir);
bool FS_idPak( char *pak, char *base );
bool FS_ComparePaks( char *neededpaks, int len, bool dlstring );
void FS_Rename( const char *from, const char *to );

bool FS_WriteToTemporaryFile( const void *data, size_t dataLength, char **tempFileName );

void FS_UpdateGamedir(void);


// Edit fields and command line history/completion

#define CONSOLE_PROMPT_CHAR ']'
#define	MAX_EDIT_LINE		256

typedef struct field_s {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

void Field_Clear( field_t *edit );
void Field_AutoComplete( field_t *edit );
void Field_CompleteKeyname( void );
void Field_CompleteFilename( const char *dir, const char *ext, bool stripExt, bool allowNonPureFilesOnDisk );
void Field_CompleteCommand( char *cmd, bool doCommands, bool doCvars );

// MISC

#define RoundUp(N, M) ((N) + ((unsigned int)(M)) - (((unsigned int)(N)) % ((unsigned int)(M))))
#define RoundDown(N, M) ((N) - (((unsigned int)(N)) % ((unsigned int)(M))))

char		*CopyString( const char *in );
void		Info_Print( const char *s );

void		Com_BeginRedirect (char *buffer, int buffersize, void (*flush)(char *));
void		Com_EndRedirect( void );
#if defined( _GAME ) || defined( _CGAME ) || defined( UI_BUILD )
	extern NORETURN_PTR void (*Com_Error)( int level, const char *fmt, ... );
	extern void (*Com_Printf)( const char *fmt, ... );
#else
	void NORETURN QDECL Com_Error( int level, const char *fmt, ... );
	void QDECL Com_Printf( const char *fmt, ... );
#endif
void 		QDECL Com_DPrintf( const char *fmt, ... );
void		QDECL Com_OPrintf( const char *fmt, ...); // Outputs to the VC / Windows Debug window (only in debug compile)
void 		NORETURN Com_Quit_f( void );
int			Com_EventLoop( void );
int			Com_Milliseconds( void );	// will be journaled properly
uint32_t	Com_BlockChecksum( const void *buffer, int length );
char		*Com_MD5File(const char *filename, int length, const char *prefix, int prefix_len);
int      Com_HashKey(char *string, int maxlen);
int			Com_Filter(char *filter, char *name, int casesensitive);
int			Com_FilterPath(char *filter, char *name, int casesensitive);
int			Com_RealTime(qtime_t *qtime);
bool	Com_SafeMode( void );
void		Com_RunAndTimeServerPacket(netadr_t *evFrom, msg_t *buf);

void		Com_StartupVariable( const char *match );
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

// com_speeds times
extern	int		time_game;
extern	int		time_frontend;
extern	int		time_backend;		// renderer backend time

extern	int		com_frameTime;

extern	bool	com_errorEntered;

extern	fileHandle_t	com_logfile;
extern	fileHandle_t	com_journalFile;
extern	fileHandle_t	com_journalDataFile;

/*
typedef enum {
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;
*/

/*

low memory
	server vm
	server clipmap
mark
	renderer initialization (shaders, etc)
	UI vm
	cgame vm
	renderer map
	renderer models
free
	temp file loading
high memory

*/

#if defined(_DEBUG)
	#define DEBUG_ZONE_ALLOCS
#endif

/*
#ifdef DEBUG_ZONE_ALLOCS
	#define Z_TagMalloc(size, tag)			Z_TagMallocDebug(size, tag, #size, __FILE__, __LINE__)
	#define Z_Malloc(size)					Z_MallocDebug(size, #size, __FILE__, __LINE__)
	#define S_Malloc(size)					S_MallocDebug(size, #size, __FILE__, __LINE__)
	void *Z_TagMallocDebug( int size, int tag, char *label, char *file, int line );	// NOT 0 filled memory
	void *Z_MallocDebug( int size, char *label, char *file, int line );			// returns 0 filled memory
	void *S_MallocDebug( int size, char *label, char *file, int line );			// returns 0 filled memory
#else
	void *Z_TagMalloc( int size, int tag );	// NOT 0 filled memory
	void *Z_Malloc( int size );			// returns 0 filled memory
	void *S_Malloc( int size );			// NOT 0 filled memory only for small allocations
#endif
void Z_Free( void *ptr );
void Z_FreeTags( int tag );
int Z_AvailableMemory( void );
void Z_LogHeap( void );
*/

// later on I'll re-implement __FILE__, __LINE__ etc, but for now...

#ifdef DEBUG_ZONE_ALLOCS
void *Z_Malloc  ( int iSize, memtag_t eTag, bool bZeroit = false, int iAlign = 4);	// return memory NOT zero-filled by default
void *S_Malloc	( int iSize );					// NOT 0 filled memory only for small allocations
#else
void *Z_Malloc  ( int iSize, memtag_t eTag, bool bZeroit = false, int iAlign = 4);	// return memory NOT zero-filled by default
void *S_Malloc	( int iSize );					// NOT 0 filled memory only for small allocations
#endif
void  Z_MorphMallocTag( void *pvBuffer, memtag_t eDesiredTag );
void  Z_Validate( void );
int   Z_MemSize	( memtag_t eTag );
void  Z_TagFree	( memtag_t eTag );
void  Z_Free	( void *ptr );
int	  Z_Size	( void *pvAddress);
void Com_InitZoneMemory(void);
void Com_InitZoneMemoryVars(void);
void Com_InitHunkMemory(void);
void Com_ShutdownZoneMemory(void);
void Com_ShutdownHunkMemory(void);

void Hunk_Clear( void );
void Hunk_ClearToMark( void );
void Hunk_SetMark( void );
bool Hunk_CheckMark( void );
void Hunk_ClearTempMemory( void );
void *Hunk_AllocateTempMemory( int size );
void Hunk_FreeTempMemory( void *buf );
int	Hunk_MemoryRemaining( void );
void Hunk_Log( void);
void Hunk_Trash( void );
bool Com_TheHunkMarkHasBeenMade(void);

void Com_TouchMemory( void );

// commandLine should not include the executable name (argv[0])
void Com_Init( char *commandLine );
void Com_Frame( void );
void Com_Shutdown( void );

// CLIENT / SERVER SYSTEMS

// client interface
void CL_InitKeyCommands( void );
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init( void );
void CL_Disconnect( bool showMainMenu );
void CL_Shutdown( void );
void CL_Frame( int msec );
bool CL_GameCommand( void );
void CL_KeyEvent (int key, bool down, unsigned time);

void CL_CharEvent( int key );
// char events are for field typing, not game control

void CL_MouseEvent( int dx, int dy, int time );

void CL_JoystickEvent( int axis, int value, int time );

void CL_PacketEvent( netadr_t from, msg_t *msg );

void CL_ConsolePrint( const char *text );

void CL_MapLoading( void );
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void	CL_ForwardCommandToServer( const char *string );
// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_FlushMemory( void );
// dump all memory on an error

void CL_StartHunkUsers( void );
// start all the client stuff using the hunk

bool CL_ConnectedToRemoteServer( void );
// returns true if connected to a server

void Key_KeynameCompletion ( void(*callback)( const char *s ) );
// for keyname autocompletion

void Key_WriteBindings( fileHandle_t f );
// for writing the config files

void S_ClearSoundBuffer( void );
// call before filesystem access

// AVI files have the start of pixel lines 4 byte-aligned
#define AVI_LINE_PADDING 4

// server interface
void SV_Init( void );
void SV_Shutdown( char *finalmsg );
void SV_Frame( int msec );
void SV_PacketEvent( netadr_t from, msg_t *msg );
int SV_FrameMsec( void );
bool SV_GameCommand( void );
void SV_ShutdownGameProgs( void );

// UI interface
bool UI_GameCommand( void );

/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */

#define NYT HMAX					/* NYT = Not Yet Transmitted */
#define INTERNAL_NODE (HMAX+1)

typedef struct nodetype {
	struct	nodetype *left, *right, *parent; /* tree structure */
	struct	nodetype *next, *prev; /* doubly-linked list */
	struct	nodetype **head; /* highest ranked node in block */
	int		weight;
	int		symbol;
} node_t;

#define HMAX 256 /* Maximum symbol */

typedef struct huff_s {
	int			blocNode;
	int			blocPtrs;

	node_t*		tree;
	node_t*		lhead;
	node_t*		ltail;
	node_t*		loc[HMAX+1];
	node_t**	freelist;

	node_t		nodeList[768];
	node_t*		nodePtrs[768];
} huff_t;

typedef struct huffman_s {
	huff_t		compressor;
	huff_t		decompressor;
} huffman_t;

void	Huff_Compress(msg_t *buf, int offset);
void	Huff_Decompress(msg_t *buf, int offset);
void	Huff_Init(huffman_t *huff);
void	Huff_addRef(huff_t* huff, byte ch);
int		Huff_Receive (node_t *node, int *ch, byte *fin);
void	Huff_transmit (huff_t *huff, int ch, byte *fout);
void	Huff_offsetReceive (node_t *node, int *ch, byte *fin, int *offset);
void	Huff_offsetTransmit (huff_t *huff, int ch, byte *fout, int *offset);
void	Huff_putBit( int bit, byte *fout, int *offset);
int		Huff_getBit( byte *fout, int *offset);

void MSG_shutdownHuffman();

extern huffman_t clientHuffTables;

#define	SV_ENCODE_START		4
#define SV_DECODE_START		12
#define	CL_ENCODE_START		12
#define CL_DECODE_START		4

inline int Round(float value)
{
	return((int)floorf(value + 0.5f));
}

// Persistent data store API
bool PD_Store ( const char *name, const void *data, size_t size );
const void *PD_Load ( const char *name, size_t *size );

uint32_t ConvertUTF8ToUTF32( char *utf8CurrentChar, char **utf8NextChar );
void BotDrawDebugPolygons(void (*drawPoly)(int color, int numPoints, float *points), int value);

#include "sys/sys_public.h"
