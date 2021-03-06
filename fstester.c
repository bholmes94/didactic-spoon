/*
 * This program is to manipulate the filesystem without the need for running the FUSE-based application. This
 * is primarily used for debugging or manually inserting data and directory entries inside the filesystem
 * for testing of the actual program. Additionally, this is used for testing algorithms for re-allocation,
 * deletion, and movement of files and data.
 * Layout:
 *			- 16 byte name
 *			- 11 byte start location
 *			- 11 byte end location
 *			- 3  byte offset *for remaining space of last block
 *
 * Command to run to make test filesystem: dd if=/dev/zero of=filesys bs=1 count=0 seek=4m
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h> 
#include <errno.h>

// global variables
int BLOCKSIZE = 512;	// blocksize of storage system
int ENTRYSIZE = 41;		// size of one entry in directory
int DATABEGIN = 512;	// beginning of user data at point 512 (513th space in array)
int ENTRIES = 0;		// number of entries within the directory
char block[512];		// array to store first block
struct entry *HEAD;		// head of array for the entries
FILE *FSPTR;			// global reference to file pointer

struct entry {
	char filename[16];
	int start, end, off;
	struct stat *info;
	struct entry *next;
};

static void init_dir(char *filesystem)
{
	//char entry[ENTRYSIZE];		// buffer for single entry
	int count, i, begin;
	char filename[17];
	char start[11];
	char end[11];
	char off[4];
	struct entry *prev;

	printf("[!] retreiving directory from %s\n", filesystem);

	// read dir from beginning into buffer
	FILE *dir = fopen(filesystem, "rw+");
	memset(&block, '\0', sizeof(block));
	fseek(dir, 0, SEEK_SET);			// seek to beginning of storage
	fread(block, BLOCKSIZE, 1, dir);	// read a block into memory
	count = block[0] - '0';				// convert to int
	
	if(count < 0) ENTRIES = 0;
	else ENTRIES = count;

	// print result for debugging
	printf("Number of files:\t%d\n", count);

	// reads directories, currently printing locations of dirs in the block
	for(i = 1; i < ENTRIES+1; i++) {

		if(i == 1) {
			prev = malloc(sizeof(struct entry));
			printf("[!] Entry %d location\t10\n", i);

			memcpy(filename, &block[10], 16);
			filename[16] = '\0';
			printf("[+] filename\t%s\n", filename);
			memcpy(prev->filename, &filename, 16);
			
			// copy start location from directory
			memcpy(start, &block[26], 11);
			start[10] = '\0';
			printf("[+] begin\t%s|%d\n", start, atoi(start));
			prev->start = atoi(start);

			// copy end location from directory
			memcpy(end, &block[37], 11);
			end[10] = '\0';
			printf("[+] end\t\t%s|%d\n", end, atoi(end));
			prev->end = atoi(end);

			// copy the offset of the last block from mem
			memcpy(off, &block[48], 3);
			off[3] = '\0';
			printf("[+] offset\t%s|%d\n", off, atoi(off));
			prev->off = atoi(off);

			HEAD = prev;

		}
		else {
			struct entry *tmp = malloc(sizeof(struct entry));

			begin = 10 + (41*(i-1));		// calculate the location of entry
			printf("[!] Entry %d location\t%d\n", i, begin);
			
			// copy filename from dir and into array for struct use
			memcpy(filename, &block[begin], 16);
			filename[15] = '\0';
			printf("[+] filename\t%s\n", filename);
			memcpy(tmp->filename, filename, 16);

			// copy start location from directory
			memcpy(start, &block[begin+16], 11);
			start[10] = '\0';
			printf("[+] begin\t%s|%d\n", start, atoi(start));
			tmp->start = atoi(start);

			// copy end location from directory
			memcpy(end, &block[begin+27], 11);
			end[10] = '\0';
			printf("[+] end\t\t%s|%d\n", end, atoi(end));
			tmp->end = atoi(end);

			// copy the offset of the last block from mem
			memcpy(off, &block[begin+38], 3);
			off[2] = '\0';
			printf("[+] offset\t%s|%d\n", off, atoi(off));
			tmp->off = atoi(end);

			prev->next = tmp;
			prev = tmp;
		}
	}

	fclose(dir);	// close directory to complete

	struct entry *print = HEAD;
	for(i=0; i < ENTRIES; i++) {
		printf("[-] number \t%d\tname\t%s\n", i, print->filename);
		print = print->next;
	}
}

/*
 * Function called to create a new entry inside of the directory which is already read
 * into memory and flush it to the drive. Also writes data to specified location. Right
 * now it is hard coded.
 * @param filesystem 	- location (file in Unix) to write to
 * @param filename		- file to write to filesystem
 *
 * Note: The buffers for start, end, and off are increased by one to accomodate the null
 * byte at the end of the string. Otherwise the program breaks. The program still will 
 * only write 11 bytes for start, end and 3 for the offset.
 */
void create_entry(char *filesystem, char *filename)
{
	int location, i, size, blocks, total_size;
	struct entry *tmp;
	size_t bytes;		// number of bytes read from file
	char buf[512];		// buffer for finding free space
	char wrbuf[512];	// separate buffer for writing to fs
	char start[12];		// start block buffer
	char end[12];		// end block buffer
	char off[4];		// offset buffer for last block


	location = (ENTRIES * 41) + 10;			// calculates start location in dir
	FILE *fp = fopen(filename, "r");		// open requested file
	fseek(fp, 0L, SEEK_END);				// calculate size
	size = ftell(fp);						// get results
	fseek(fp, 0, SEEK_SET);					// sets to beginning again
	
	/* check number of blocks needed */
	if(size%BLOCKSIZE != 0) blocks = size/BLOCKSIZE + 1;
	else blocks = size/BLOCKSIZE;

	printf("[!] Size of file is\t%d\tblocks needed\t%d\n", size, blocks);

	
	// writes filename to directory
	for(i = 0; i < strlen(filename) + 1; i++) {
		block[location + i] = filename[i];
	}

	// finds location for number of calculated blocks
	FILE *fs = fopen("filesys", "r+");		// open filesys for calculations
	fseek(fs, 0L, SEEK_END);				// count size of entire system
	total_size = ftell(fs);					// get result
	fseek(fs, 512, SEEK_SET);				// make sure reset to beginning

	// print results. Debugging
	printf("[!] Total blocks in filesystem\t%d\n", total_size/BLOCKSIZE);

	/*
	 * new idea for finding space. Finds the last item in system and writes 
	 * to the next availible block after it. This assumes that the filesystem
	 * remains defragmented. Note that the last block is inclusive. So if 
	 * the end block is 4701, it uses some or all of 4701 and the write will
	 * need to start at block 4702.
	 *
	 * Below, we check and see if there are any entries on which we can base
	 * our next files position on. If there is not, we create a 'dummy' entry
	 * and assign it's end value (the only one used) to the end of the reser-
	 * ved space. The subsequent calculations will be based on this.
	 */

	tmp = HEAD;
	if(ENTRIES > 0) {
		for(i = 0; i < ENTRIES; i++) {
			if(i == ENTRIES - 1) {
				printf("[!] %s is the last file in the system. Ends at block %d\n", tmp->filename, tmp->end);
				break;
			}
			tmp = tmp->next;
		}
	} else { // creating 'dummy' struct
		tmp = malloc(sizeof(struct entry));
		tmp->end = 1;
	}

	// calculate start locations in bytes
	location = 10 + (ENTRIES * 41);
	printf("[+] start write at block %d and location %d\n[+] size of file (in blocks): %d\n"
						,tmp->end+1, (tmp->end * BLOCKSIZE) - 1, blocks); // -1 b/c pointer starts at 0
	printf("[+] directory write location %d\n[+] filesystem write start location %d\n", location, tmp->end * BLOCKSIZE);


	fseek(fs, tmp->end * BLOCKSIZE, SEEK_SET);

	// use existing buffer to write pieces to system
	while(0 < (bytes = fread(wrbuf, 1, sizeof(wrbuf), fp)))
		fwrite(wrbuf, 1, bytes, fs);

	/*
	 * NOTE: From here on out, any -1 is for exact location since the buffers start at 0, not 1. All calculations
	 * for the directory can just be done in blocks without issue. These start at 1. Block 1 corresponds to locations
	 * 0-511 (512 bytes) in filesystem. Remember this when reading from file! Offset buf can help.
	 */


	/* convert int to strings */
	sprintf(start, "%d", tmp->end+1);
	sprintf(end, "%d", (tmp->end + 1) + (blocks - 1));
	sprintf(off, "%d", size%BLOCKSIZE);				/* offset is just the remainder of the size/512 */


	/* 
	 * NOTE: all write functions are to memory, NOT the drive. there is a 
	 * separate function to flush the directory to the drive.
	 */
	
	// writes start block to directory
	for(i = 0; i < strlen(start) + 1; i++) {
		block[location + 16 + i] = start[i];
	}

	// writes end block to directory
	for(i = 0; i < strlen(end) + 1; i++) {
		block[location + 27 + i] = end[i];
	}

	// writes offset to directory
	for(i = 0; i < strlen(off); i++) {
		block[location + 38 + i] = off[i];
	}

	// update entry count, write to directory
	ENTRIES++;
	block[0] = ENTRIES + '0';

	// debugging print directory
	for(i = 10; i < BLOCKSIZE; i++) {
		printf("%c ", block[i]);
	}

	// flushes directory and writes to disk
	fseek(fs, 0, SEEK_SET);
	fwrite(block, sizeof(block), 1, fs);	/* rewrites the directory block */
	fclose(fs);
	fclose(fp);
}

/**
 * Mostly used to initialize filesystem and read the directories into memory. Also
 * parses command line arguments to interact and test system.
 */
int main(int argc, char *argv[])
{
	int opt;
	bool print;
	/* Help from http://stackoverflow
	.com/questions/9642732/parsing-command-line-arguments */
	// enum for filesystem mode
	enum { WRITE, LIST, DELETE } mode = -1;

	init_dir("filesys");		// hard coded for testing purposes
	FSPTR = fopen("filesys", "r+");

	while ((opt = getopt(argc, argv, "wldp")) != -1) {
		switch(opt) {
			case 'w': mode = WRITE; break;
			case 'l': mode = LIST; break;
			case 'd': mode = DELETE; break;
			case 'p': print = true; break;
			default:
				printf("Usage: %s [filesys] [-w] [filename]\n", argv[0]);
				printf("       %s [filesys] [-l] lists all structs\n", argv[0]);
				printf("       %s [filesys] [-p] prints buffer, no filename needed\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// indicate selected mode to user
	printf("[+] selected mode %d\n[+] Print? %d\n", mode, print);
	switch(mode) {
		case WRITE:
			// stupid thing is broken, had to manually assign filename
			printf("WRITE file\t%s\n\tbegin:\t%s\n\tend:\t%s\n", argv[3], argv[4], argv[5]); create_entry("filesys", argv[3]); break;
		case LIST:
			printf("LIST directories in filesystem\n"); break;
		case DELETE:
			printf("DELETE file\t%s\n", argv[1]); break;
		default:
			printf("[!] Only PRINT selected\n");
	}

	return 1;
}
