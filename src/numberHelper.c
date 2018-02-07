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


size_t getBytesAsNumber(byte* data_pointer, size_t num_bytes, ByteOrder endianness)
{
	size_t ret = 0;
	if(sizeof(size_t) < num_bytes)
	{
		if(__byte_order__ != endianness)
		{
			memcpy(&ret, data_pointer + (num_bytes - sizeof(size_t)), sizeof(size_t));
			reverseBytes((byte*)&ret, num_bytes);
		}
		else
		{
			memcpy(&ret, data_pointer, sizeof(size_t));
		}
	}
	else
	{
		memcpy(&ret, data_pointer, num_bytes);
		if(__byte_order__ != endianness)
		{
			reverseBytes((byte*)&ret, num_bytes);
		}
	}
	return ret;
}


size_t roundUp8(size_t numToRound)
{
	size_t remainder = numToRound%8;
	if(remainder == 0)
	{
		return numToRound;
	}
	
	return numToRound + 8 - remainder;
}