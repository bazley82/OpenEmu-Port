
#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

//primitive types
typedef signed char				INT8;
typedef unsigned char			UINT8;
typedef signed short			INT16;
typedef unsigned short			UINT16;
typedef signed int				INT32;
typedef unsigned int			UINT32;

// There is no ANSI standard for 64 bit integers :(
#ifdef _MSC_VER
typedef __int64					INT64;  // MS VC++
typedef unsigned __int64		UINT64;  // MS VC++
#else
typedef signed long long int	INT64;  // C99 C++11
typedef unsigned long long int	UINT64;	// C99 C++11
#endif

typedef char					CHAR;

#if !defined(BOOL)
#if defined(__MACH__)
#if defined(__arm64__)
#include <stdbool.h>
typedef _Bool                   BOOL;
#else
typedef signed char				BOOL;
#endif
#else
typedef int						BOOL;
#endif
#endif

#if !defined(TRUE)
#define TRUE	(1)
#endif

#if !defined(FALSE)
#define FALSE	(0)
#endif

#if !defined(NULL)
#if defined(__cplusplus)
#define NULL (0)
#else
#define NULL ((void *)0)
#endif
#endif

#define SWAP32BIT(swap)			((((swap) << 24) & 0xFF000000) | \
								 (((swap) <<  8) & 0x00FF0000) | \
								 (((swap) >>  8) & 0x0000FF00) | \
								 (((swap) >> 24) & 0x000000FF))

#if !defined(__LITTLE_ENDIAN__) && defined(_WIN32)
#define __LITTLE_ENDIAN__		(1)
#endif

#if defined(__LITTLE_ENDIAN__)
#define FOURCHAR(x) SWAP32BIT((UINT32)(x))
#else
#define FOURCHAR(x) (x)
#endif

#if defined(__GNUC__)
#define TYPEDEF_STRUCT_PACK(_x_) typedef struct __attribute__((__packed__)) _x_
#define PACKED(_x_) _x_ __attribute__((__packed__))
#elif defined(_MSC_VER)
#define TYPEDEF_STRUCT_PACK(_x_) __pragma(pack(1)) typedef struct _x_ __pragma(pack())
#define PACKED(_x_) __pragma(pack(push,1)) _x_ __pragma(pack(pop))
#else
#define TYPEDEF_STRUCT_PACK(_x_) _x_
#define PACKED(_x_) _x_
#warning pack macro is not supported on this compiler
#endif

#if defined( __MACH__ )
#include <string.h>

typedef unsigned char			UCHAR;
typedef unsigned short			USHORT;

#ifndef FLOAT
typedef float					FLOAT;
#endif

#ifndef GUID
typedef struct
{
	unsigned int			data1;
	unsigned short			data2;
	unsigned short			data3;
	unsigned char			data4[8];
} GUID;
#endif

#ifndef ZeroMemory
#define ZeroMemory(ptr,size) memset(ptr, 0, size)
#endif

#ifndef strcmpi
#define strcmpi strcasecmp
#endif

#ifndef MAX_PATH
#include <sys/syslimits.h>
#define MAX_PATH PATH_MAX
#endif

#endif /* __MACH__ */

#ifdef __cplusplus
}
#endif

#endif //TYPES_H

