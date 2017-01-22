#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
// global variables
int BLOCKSIZE = 512;	// blocksize of storage system
int ENTRYSIZE = 37;		// size of one entry in directory
int DATABEGIN = 512;	// beginning of user data at point 512 (513th space in array)
int ENTRIES = 0;		// number of entries within the directory
char block[512];		// array to store first block
struct entry *HEAD;		// head of array for the entries

/* struct to store each entry and info */
struct entry {
	char filename[16];
	int start, end, off;
	struct stat info;
	struct entry *next;
};

static int file_lookup(char *filename, struct stat *stbuf)
{
	struct entry *tmp;	// pointer to struct for searching
	char cpy[16];		// to deal with string copying
	int i;				// for loop later on
	tmp = HEAD;			// points tmp to head

	strcpy(cpy, filename);
	memmove(cpy, cpy + 1, strlen(cpy));

	printf("[+] not root dir filename:\t%s\n", cpy);

	// loop through all entries searching for a match
	for(i = 0; i < ENTRIES; i++) {
		if(strcmp(cpy, tmp->filename) == 0){
			printf("[!] Match found!\n");
			// grabs stored attributes
			if(stbuf != NULL) { 
				stbuf->st_nlink = 1;
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_size = tmp->info.st_size;
			}
			return 1;
		}
		tmp = tmp->next;
	}

	return 0;
}

/**
 * Function deals with creating the directory and storing it in memory
 * for reference during execution
 * @param filesystem - the name of the filesystem to read from
 */
static void init_dir(char *filesystem)
{
	//char entry[ENTRYSIZE];		// buffer for single entry
	int count, i, begin;
	char filename[17];
	char start[12];
	char end[12];
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

			struct stat stbuf;
		 	stbuf.st_ino = i+10;
		 	stbuf.st_mode = S_IFREG | 0444;
		 	stbuf.st_nlink = 10;
		 	stbuf.st_size = (((prev->end - prev->start)) * 512) + prev->off;
		 	prev->info = stbuf;

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


			struct stat st;
		 	st.st_ino = i+10;
		 	st.st_mode = S_IFREG | 0444;
		 	st.st_nlink = 10;
		 	st.st_size = (((atoi(end) - atoi(start))) * 512) + atoi(off);
		 	tmp->info = st;
		 	printf("[+] Size of file:\t%zu | Calculated:\t%zu\n", st.st_size, (((atoi(end) - atoi(start))) * 512) + atoi(off));
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

static int myfs_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	printf("[!] GETATTR called\tpath:\t%s\n", path);

	// check if root or not, if not find in directory
	if(strcmp(path, "/") == 0) {
		printf("[+] root dir\n");
		stbuf->st_ino = 2;
		stbuf->st_mode = S_IFDIR | 0755;
		return 0;
	} else if(file_lookup(path, stbuf) == 1) {	//TODO: Breaks looking up some files, fix this!
		printf("[+] resetting attributes\n");
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		//stbuf->st_size = HEAD->info.st_size;
		return 0;
	} else {
		printf("[+] Nothing found\n");
		return -ENOENT;
	}
}

static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	struct entry *tmp;
	tmp = HEAD;
	(void) fi;
	printf("[!] READDIR called\tpath:\t%s\n", path);

	// check to make sure in right directory
	if(strcmp(path, "/") != 0) return -ENOENT;
	else {
		// place back directories
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		// loop through the files in dir in memory
		int i;
		for(i = 0; i < ENTRIES; i++) {
			filler(buf, tmp->filename, NULL, 0);
			tmp = tmp->next;
		}
	}
	return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[!] OPEN called\tpath:\t%s\n", path);
	return 0;
}

/*
 * Reads files into a buffer and returns the number of bytes that are written. To ensure somewhat
 * easy compatability with Windows, this function will read 512 bytes (1 block) at a time into a
 * buffer before passing it onto the FUSE buffer. Can implement multiple block transfers later on.
 */
static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	size_t total;			// var to store written byte number
	char cpy[16];			// buffer for copy of path
	FILE *fs;				// file pointer to the actual data
	char data[513];			// buffer of one block to read from file
	struct entry *tmp;		// pointer to head of directory list
	int i, start, end;		// for loop stuff & location storage
	
	/* copy string & get rid of '/' */
	strcpy(cpy, path);
	memmove(cpy, cpy + 1, strlen(cpy));

	/* testing finding the file to change */
	tmp = HEAD;
	for(i = 0; i < ENTRIES; i++) {		// loop through all entries, find matching filename
		if(strcmp(cpy, tmp->filename) == 0) printf("[+] File found\n");
	}

	/* mostly testing stuff here... */
	printf("[!] READ called\tpath:\t%s\n\t-start: %d\n\t-end: %d\n", path, tmp->start, tmp->end);
	printf("[!] buffer size:\t%d\toffset:\t%d\n", size, offset);
	if(512 >= size) {
		fs = fopen("filesys", "r+");
		//printf("[+] next chunk too large. Remaining size of buf:\t%zu\tcur total:\t%zu\tadding:\t%zu\n", size, total, size);
		start = ((tmp->start - 1) * BLOCKSIZE) + offset;
		fseek(fs, start, SEEK_SET);
		fread(data, size, 1, fs);
		printf("\n[+] start location:\t%d\n", start);
		memcpy(buf, data, size);
		total = size;
		close(fs);
	}
	else {
		/* for now: open, read x into data buffer, repeat */
		fs = fopen("filesys", "r+");
		/* if fs is open, set to proper location and read */
		if(fs) {
			//printf("[+] Read from %d to %d\n", ((tmp->start - 1) * BLOCKSIZE) + offset-512, ((tmp->end - 1) * BLOCKSIZE) + tmp->off);
			start = ((tmp->start - 1) * BLOCKSIZE) + offset;
			//end = ((tmp->end - 1) * BLOCKSIZE) + tmp->off;
			printf("[+] Seek set to\t%d\n", ((tmp->start - 1) * BLOCKSIZE) + offset);
			fseek(fs, start, SEEK_SET);
			memset(data, 0, sizeof(data));
			fread(data, 512, 1, fs);
			memcpy(buf, data, 512);
			data[512] = '\0';
			total = 512;
			fclose(fs);
		} else {
			printf("[-] ERROR: file pointer not pointing to a filesystem\n");
		}
	}

	return total; // Returns number of bytes read, will call again with new buffer if not reached size yet! Uses 8byte buffer by default
	// offset approaches size
}

static int myfs_write()
{
	printf("[!] WRITE called\n");
	return 0;
}

static int myfs_create()
{
	printf("[!] CREATE called\n");
	return 0;
}

/* mapping of FUSE functions to my functions */
static struct fuse_operations myfs_ops = {
	.getattr	= myfs_getattr,
	.readdir	= myfs_readdir,
	.open 		= myfs_open,
	.read 		= myfs_read,
	.write		= myfs_write,
	.create		= myfs_create,
};

int main(int argc, char *argv[])
{
	init_dir("filesys");
	return fuse_main(argc, argv, &myfs_ops, NULL);	// FUSE taking over here
}