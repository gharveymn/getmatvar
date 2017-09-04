#include <stdio.h>
#include "zlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

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
#define COMP_FACTOR 100

uint32_t numFromStr(char* str);

int main(int argc, char* argv[])
{
	char filename[50];
	strcpy(filename, "my_struct1.mat");
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
	if (!fp)
	{
		perror("Could not open compressed data file.");
		exit(1);
	}
	strcpy(filename, "UncompressedDataElement.txt");
	uncomprfp = fopen(filename, "w");

	fseek(fp, 0x2228, SEEK_SET);

	numElem = 0;
	
	printf("Current file address: 0x%x\n", ftell(fp));
	bytesToRead = 0xF5;

	//read compressed data element into dataBuffer
	dataBuffer = (char *)malloc(bytesToRead*sizeof(char));
	if ((readErr = fread(dataBuffer, 1, bytesToRead, fp)) != bytesToRead)
	{
		fputs("Failed to read compressed data\n", stderr);
		printf("Bytes read:%d\n", readErr);
	}

	uncompr = (char *)malloc(COMP_FACTOR*bytesToRead*sizeof(char));

	int ret;
	unsigned have;
	z_stream strm;
	unsigned char out[COMP_FACTOR*0xF5];

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
	{
		printf("ret = %d\n", ret);
		exit(EXIT_FAILURE);
	}

	do
	{
		strm.avail_in = bytesToRead;
		strm.next_in = dataBuffer;
		
		do
		{
			strm.avail_out = COMP_FACTOR*bytesToRead;
			strm.next_out = uncompr;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = COMP_FACTOR*bytesToRead - strm.avail_out;
			if (fwrite(uncompr, 1, have, uncomprfp) != have || ferror(uncomprfp))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	
	/*z_stream infstream;
	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	infstream.avail_in = (uInt)bytesToRead;
	infstream.next_in = (Bytef *)dataBuffer;
	infstream.avail_out = (uInt)(COMP_FACTOR*bytesToRead);
	infstream.next_out = (Bytef *)uncompr;
	
	inflateInit(&infstream);
	inflate(&infstream, Z_FINISH);
	inflateEnd(&infstream);*/
	fclose(uncomprfp);
	free(dataBuffer);
	free(uncompr);

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