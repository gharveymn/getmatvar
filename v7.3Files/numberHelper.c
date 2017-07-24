#include "mapping.h"

uint64_t getBytesAsNumber(char* chunk_start, int num_bytes)
{
	uint64_t ret = 0;
	int n = 0;
	uint8_t byte = 0;
	uint64_t temp = 0;
	while (n < num_bytes)
	{
		byte = *(chunk_start + n);
		temp = byte;
		temp = temp << (8*n);
		ret += temp;
		n++;
	}
	return ret;
}
double convertHexToFloatingPoint(double hex)
{
	double ret;
	int sign = 0;
	if (((uint64_t)hex & (uint64_t)pow(2, 63)) > 0)
	{
		sign = 1;
	}
	uint64_t exponent = (uint64_t)hex >> 52;
	exponent = exponent & 1023;

	uint64_t temp = (uint64_t)(pow(2, 52) - 1);
	uint64_t fraction = (uint64_t)hex & temp;

	ret = pow(-1, sign)*pow(2, exponent - 1023);
	double sum = 1;

	long b_i;

	for (int i = 1; i <=52; i++)
	{
		temp = (uint64_t)pow(2, 52 - i);
		b_i = fraction & temp;
		b_i = b_i >> (52-i);
		sum += b_i*pow(2, -i);
	}

	return ret*sum;
}
int roundUp(int numToRound)
{
	int remainder = numToRound % 8;
    if (remainder == 0)
        return numToRound;

    return numToRound + 8 - remainder;
}