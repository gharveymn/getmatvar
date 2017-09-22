/* Specification. */
#include "mapping.h"

/* This implementation is only for native Windows systems.  */
#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__

# define WIN32_LEAN_AND_MEAN

# include <windows.h>


size_t getPageSize(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (size_t)system_info.dwPageSize;
}


size_t getAllocGran(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (size_t)system_info.dwAllocationGranularity;
}


int getNumProcessors(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (int)system_info.dwNumberOfProcessors;
}


#else
#include <unistd.h>

//these are the same on unix

size_t getPageSize(void)
{
	return sysconf(_SC_PAGE_SIZE);
}

size_t getAllocGran(void)
{
	return sysconf(_SC_PAGE_SIZE);
}

int getNumProcessors(void)
{
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
}


#endif


ByteOrder getByteOrder(void)
{
	short int number = 0x1;
	char* numPtr = (char*)&number;
	if(numPtr[0] == 1)
	{
		return LITTLE_ENDIAN;
	}
	else
	{
		return BIG_ENDIAN;
	}
}
