#ifndef CERES_H
#define CERES_H

////////////////////////////////////////////////////////
// big-endian and little-endian

#define bswap16(x) ((unsigned short)( (((x)&0xffu)<<8u) | (((x)>>8u)&0xffu) ))

#define bswap32(x) (\
  ((x)&0xff000000u) >> 24 \
  |                       \
  ((x)&0x00ff0000u) >>  8 \
  |                       \
  ((x)&0x0000ff00u) <<  8 \
  |                       \
  ((x)&0x000000ffu) << 24 \
)

// these are compiler-defined, so we put this section BEFORE the includes

#if defined(_WIN32) || defined(_WIN64)
#define __BYTE_ORDER 1
#define __BIG_ENDIAN 0
#define __LITTLE_ENDIAN 1
#elif defined(__APPLE__) && defined(__MACH__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
# define cpu_to_le32(x) bswap32(x)
# define le32_to_cpu(x) bswap32(x)
# define cpu_to_le16(x) bswap16(x)
# define le16_to_cpu(x) bswap16(x)
# define cpu_to_be32(x) (x)
# define be32_to_cpu(x) (x)
# define cpu_to_be16(x) (x)
# define be16_to_cpu(x) (x)
#else
# define cpu_to_le32(x) (x)
# define le32_to_cpu(x) (x)
# define cpu_to_le16(x) (x)
# define le16_to_cpu(x) (x)
# define cpu_to_be32(x) bswap32(x)
# define be32_to_cpu(x) bswap32(x)
# define cpu_to_be16(x) bswap16(x)
# define be16_to_cpu(x) bswap16(x)
#endif

#define Log(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)



#define TRUE 1
#define FALSE 0
#include <stdint.h>
#include <usb.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define DEFAULT_CONFIGURATION	1
#define DEFAULT_INTERFACE	0
#define DEFAULT_ALT_INTERFACE	0
#define SATURNDOWNLOAD "/saturn_vids"
#define PV2_DOWNLOAD "/pv2_pics"

#define FIFILE  0
#define FIDIR   1
#define FIPART  2
#define FIROOT  3

#ifndef LIBUSB_PATH_MAX  //libusb may define it already
#define LIBUSB_PATH_MAX 4096
#endif

#define BUFSIZE 4096
#define LINUX_BUFSIZE 64

#define TIMEOUT			5000 
#define SHORT_TIMEOUT		500 
#define LONG_TIMEOUT		5000 
#define CAM_STIMEOUT		3000 
#define CAM_LTIMEOUT		14000 
#define STRINGSIZE		128 
#define CAMCORDER_VENDOR	0x167B 
#define CAMERA_VENDOR		0x0DCA
#define VENDOROLD		0x04C5 
#define READ_ENDPOINT		0x81 
#define WRITE_ENDPOINT		0x81
#define CAMERA_READ_ENDPOINT	0x81 
#define CAMERA_WRITE_ENDPOINT	0x01 
#define MAX_NUMBER_OF_FILES_IN_DIRECTORY	1000

// FAT code follows
// excerpt from FlushedSector fs@proglang.cjb.net header file. Last updated 2001-11-24 

#pragma pack (push, 1) // Force 1 byte structure packing


typedef unsigned char   byte;  /* 1 byte */
typedef unsigned short  word;  /* 2 bytes */
typedef unsigned int	dword; /* 4 bytes */


// Partition Table Entry, 16 bytes
typedef struct
{
	byte	State;
	byte	StartingHead;
	word	StartingSector:6;
	word	StartingCylinder:10;
	byte	Type;
	byte	EndingHead;
	word	EndingSector:6;
	word	EndingCylinder:10;
	dword	RelativeSectors;
	dword	NumberOfSectors;
} PartitionTable;


// Master Boot Record, 512 bytes
typedef struct
{
	byte			ExecutableCode[446];
	PartitionTable	Partiton[4];
	word			Signature; /*=AA55*/
} MasterBootRecord;


// FAT Boot Record, 512 bytes
typedef struct
{
	byte	JumpInstruction[3];
	byte	OEMID[8];
	word	BytesPerSector;
	byte	SectorsPerCluster;
	word	ReservedSectors;
	byte	FATs;
	word	RootEntries;
	word	SmallSectors;
	byte	Media;
	word	FATSize;
	word	TrackSize;
	word	Heads;
	dword	HiddenSectors;
	dword	LargeSectors;
	byte	DriveNumber;
	byte	CurrentHead;
	byte	Signature;
	dword	ID;
	byte	VolumeLabel[11];
	byte	SystemID[8];
	byte	LoadInstructions[448]; // 512-64
	word	BR_Signature; /*=AA55h*/
}  BootRecord;

// Time-field in DirEntry, 4 bytes
typedef struct
{
	word	Sec		:5;
	word	Min		:6;
	word	Hour	:5;
	word	Day		:5;
	word	Month	:4;
	word	Year	:7;
} FileTime;

// DirectoryEntry DOS Attributes in a, 1 byte
typedef struct
{
	byte	ReadOnly	:1;
	byte	Hidden		:1;
	byte	System		:1;

	byte	VolumeID	:1;
	byte	Directory	:1;
	byte	Archive		:1;
	byte	reserved	:2;
}  FileAttributes;

// Directory entry, 32 bytes
typedef struct
{
	char			Name[8];
	char			Ext[3];
	FileAttributes	Attributes;
	byte			reserved[8];
	word			EA_Index;
	FileTime		Time;
	word			EntryCluster;
	dword			Size;
} DirEntry;


MasterBootRecord	g_MBR;
BootRecord		g_Boot;
unsigned char		g_fat[3*512];
unsigned short		g_cluster[1024];


usb_dev_handle* m_p_handle;
struct usb_device* m_usb_device;

typedef struct file_info { //each one of these is at least 4000 bytes... :(
  char filename[STRINGSIZE];
  int filesize;
  char fullpath[STRINGSIZE];
  char dirpath[STRINGSIZE];
  int partition;
  int filetype;
  struct file_info* children[MAX_NUMBER_OF_FILES_IN_DIRECTORY]; //the culprit
  int number_of_children;
} file_info;

int Open (int *x);
int Close (void);
int Powerdown (void);
int Init (int *x);
int Unlock (int *x);
int GenerateFileList (void);
int DownloadAllMovies (int *x);
int pv2_bulk_only_mass_storage_reset();

#endif

