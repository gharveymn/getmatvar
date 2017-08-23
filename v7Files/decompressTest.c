#include <stdio.h>
#include "zlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

#define HEADER_SIZE 128
#define COMP_FACTOR 10

uint32_t numFromStr(char* str);

int main(int argc, char* argv[])
{
	char filename[50];
	strcpy(filename, "my_struct.mat");
	char spec[2];
	
	int bytesToRead, readErr, comprErr, numElem, eofAddr;
	char *dataBuffer, *uncompr;
	uLongf uncomprLen;

	FILE *fp, *uncomprfp;

	fp = fopen(filename, "rb");
	//get eof address
	fseek(fp, 0, SEEK_END);
	eofAddr = ftell(fp);
	rewind(fp);
	printf("Beginning of file address: 0x%x\n", ftell(fp));
	if (!fp)
	{
		perror("Could not open compressed data file.");
		exit(1);
	}
	//get eof address
	fseek(fp, 0, SEEK_END);
	eofAddr = ftell(fp);
	rewind(fp);

	//skip header
	fseek(fp, HEADER_SIZE, SEEK_SET);

	numElem = 0;
	while (ftell(fp) < eofAddr)
	{
		printf("Current file address: 0x%x\n", ftell(fp));
		//get number of bytes in data element
		fseek(fp, 4, SEEK_CUR);
		char dataSize[4];
		if ((readErr = fread(dataSize, 1, 4, fp)) != 4)
		{
			fputs("Failed to read size of compressed data\n", stderr);
			printf("Bytes read:%d\n", readErr);
		}

		bytesToRead = numFromStr(dataSize);
		printf("Bytes to read: 0x%x, %d\n", bytesToRead, bytesToRead);

		//read compressed data element into dataBuffer
		dataBuffer = (char *)malloc(bytesToRead);
		if ((readErr = fread(dataBuffer, 1, bytesToRead, fp)) != bytesToRead)
		{
			fputs("Failed to read compressed data\n", stderr);
			printf("Bytes read:%d\n", readErr);
		}

		//uncompress compressed data element
		uncomprLen = COMP_FACTOR*bytesToRead;
		uncompr = (char *)malloc(uncomprLen);

		comprErr = uncompress(uncompr, &uncomprLen, dataBuffer, bytesToRead);
	    CHECK_ERR(comprErr, "uncompress");

	    //write uncompressed data element to file
	    sprintf(spec, "%d", ++numElem);
	    filename[0] = '\0';
	    strcpy(filename, "UncompressedDataElement");
	    strcat(filename, spec);
	    strcat(filename, ".txt");
	    uncomprfp = fopen(filename, "w");
	    if (fwrite(uncompr, 1, uncomprLen, uncomprfp) != uncomprLen)
	    {
	    	fputs("Write failed.", stderr);
	    }
	    fclose(uncomprfp);
	    free(dataBuffer);
	    free(uncompr);
	}

    fclose(fp);
    
}
uint32_t numFromStr(char* str)
{
	uint32_t ret = str[0] & 255;
	
	ret += (str[1] << 8) & 65535;
	ret += (str[2] << 16) & 16777215;
	ret += (str[3] << 24);

	return ret;

	//return str[0] + (str[1] << 8) + (str[2] << 16) + (str[3] << 24);
}