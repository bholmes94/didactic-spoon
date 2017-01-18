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

struct entry {
	char filename[16];
	int start, end, off;
	struct stat *info;
	struct entry *next;
};

static void init_dir(char *filesystem)
{
	//char entry[ENTRYSIZE];		// buffer for single entry
	int count, i, begin, j;
	char filename[16];
	char start[11];
	char end[11];
	char off[3];
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
			start[11] = '\0';
			printf("[+] begin\t%s|%d\n", start, atoi(start));
			prev->start = atoi(start);

			// copy end location from directory
			memcpy(end, &block[37], 11);
			end[11] = '\0';
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
			filename[16] = '\0';
			printf("[+] filename\t%s\n", filename);
			memcpy(tmp->filename, filename, 16);

			// copy start location from directory
			memcpy(start, &block[begin+16], 11);
			start[11] = '\0';
			printf("[+] begin\t%s|%d\n", start, atoi(start));

			// copy end location from directory
			memcpy(end, &block[begin+27], 11);
			end[11] = '\0';
			printf("[+] end\t\t%s|%d\n", end, atoi(end));

			// copy the offset of the last block from mem
			memcpy(off, &block[begin+38], 3);
			off[3] = '\0';
			printf("[+] offset\t%s|%d\n", off, atoi(off));

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

void create_entry(char *filesystem, char *filename, char *begin, char *end)
{
	int location, i;

	location = (ENTRIES * 41) + 10;

	// writes filename to directory
	for(i = 0; i < strlen(filename) + 1; i++) {
		block[location + i] = filename[i];
	}

	// writes begin block to directory
	for(i = 0; i < strlen(begin) + 1; i++) {
		block[location + 16 + i] = begin[i];
	}

	// writes end block to directory
	for(i = 0; i < strlen(end) + 1; i++) {
		block[location + 27 + i] = end[i];
	}

	// update entry count, write to directory
	ENTRIES++;
	block[0] = ENTRIES + '0';

	// debugging print directory
	for(i = 0; i < 512; i++) {
		if(block[i] == '\0') printf("x ");
		else printf("%c", block[i]);
	}

	// flushes directory and writes to disk
	FILE *dir = fopen(filesystem, "w+");
	printf("Result: %d\n", dir);
	fseek(dir, 0, SEEK_SET);
	fwrite(block, sizeof(block), 1, dir);
	fclose(dir);
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

	while ((opt = getopt(argc, argv, "wldp")) != -1) {
		switch(opt) {
			case 'w': mode = WRITE; break;
			case 'l': mode = LIST; break;
			case 'd': mode = DELETE; break;
			case 'p': print = true; break;
			default:
				printf("Usage: %s [filesys] [-w] [filename] [begin] [end]\n", argv[0]);
				printf("       %s [filesys] [-l] lists all structs\n", argv[0]);
				printf("       %s [filesys] [-p] prints buffer, no filename needed\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// indicate selected mode to user
	//printf("[+] selected mode %d\n[+] Print? %d\n", mode, print);
	switch(mode) {
		case WRITE:
			// stupid thing is broken, had to manually assign filename
			printf("WRITE file\t%s\n\tbegin:\t%s\n\tend:\t%s\n", argv[3], argv[4], argv[5]); create_entry("filesys", argv[3], argv[4], argv[5]); break;
		case LIST:
			printf("LIST directories in filesystem\n"); break;
		case DELETE:
			printf("DELETE file\t%s\n", argv[1]); break;
		default:
			printf("[!] Only PRINT selected\n");
	}

	return 1;
}