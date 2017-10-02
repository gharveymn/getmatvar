#include "headers/getDataObjects.h"


void reverseBytes(byte* data_pointer, size_t num_elems)
{
	byte* start, * end;
	
	for(start = data_pointer, end = start + num_elems - 1; start < end; ++start, --end)
	{
		byte swap = *start;
		*start = *end;
		*end = swap;
	}
	
}


uint64_t getBytesAsNumber(byte* data_pointer, size_t num_bytes, ByteOrder endianness)
{
	uint64_t ret = 0;
	memcpy(&ret, data_pointer, num_bytes);
	if(__byte_order__ != endianness)
	{
		reverseBytes((byte*)&ret, num_bytes);
	}
	return ret;
}


int roundUp(int numToRound)
{
	int remainder = numToRound%8;
	if(remainder == 0)
	{
		return numToRound;
	}
	
	return numToRound + 8 - remainder;
}