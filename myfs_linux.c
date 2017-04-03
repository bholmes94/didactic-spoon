#define FUSE_USE_VERSION 29

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
struct entry *ENT; 		// stores location of entry being worked on
FILE* FSPTR;
char *filename = "filesys";

/* struct to store each entry and info */
struct entry {
	char filename[16];
	int start, end, off;
	struct stat info;
	struct entry *next;
};

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
void create_entry(char *filename, int filesize)
{
	int location, i, size, total_size;
	double blocks;
	struct entry *tmp;
	struct entry *new;
	size_t bytes;		// number of bytes read from file
	char buf[512];		// buffer for finding free space
	char wrbuf[512];	// separate buffer for writing to fs
	char start[12];		// start block buffer
	char end[12];		// end block buffer
	char off[12];		// offset buffer for last block


	blocks = 0;
	location = 0;
	size = filesize;
	location = (ENTRIES * 41) + 10;			// calculates start location in dir
	
	/* check number of blocks needed */
	if(size%BLOCKSIZE != 0) blocks = size/BLOCKSIZE + 1;
	else blocks = size/BLOCKSIZE;

	printf("[!] Size of file is\t%d\tblocks needed\t%d\n", size, blocks);

	
	// writes filename to directory
	for(i = 0; i < strlen(filename) + 1; i++) {
		block[location + i] = filename[i];
	}

	// finds location for number of calculated blocks
	fseek(FSPTR, 0L, SEEK_END);
	total_size = ftell(FSPTR);					// get result
	fseek(FSPTR, 512, SEEK_SET);				// make sure reset to beginning

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

	if(ENTRIES > 0) {
		tmp = HEAD;
		for(i = 0; i < ENTRIES; i++) {
			if(i == ENTRIES - 1) {
				printf("[!] %s is the last file in the system. Ends at block %d\n", tmp->filename, tmp->end);
				new = malloc(sizeof(struct entry));
				memcpy(new->filename, filename, 16);
				tmp->next = new;
				new->info.st_size = size;
				break;
			}
			tmp = tmp->next;
		}
	} else { // creating 'dummy' struct and reassigning the HEAD
		tmp = malloc(sizeof(struct entry));
		HEAD = malloc(sizeof(struct entry));
		tmp->end = 1;
		memcpy(HEAD->filename, filename, 16);
		HEAD->info.st_size = size;
		HEAD->next = NULL;
		new = HEAD;
	}
	// calculate start locations in bytes
	location = 10 + (ENTRIES * 41);
	printf("[+] start write at block %d and location %d\n[+] size of file (in blocks): %d\n"
						,tmp->end+1, (tmp->end * BLOCKSIZE) - 1, blocks); // -1 b/c pointer starts at 0
	printf("[+] directory write location %d\n[+] filesystem write start location %d\n", location, tmp->end * BLOCKSIZE);

	fseek(FSPTR, tmp->end * BLOCKSIZE, SEEK_SET);


	/*
	 * NOTE: From here on out, any -1 is for exact location since the buffers start at 0, not 1. All calculations
	 * for the directory can just be done in blocks without issue. These start at 1. Block 1 corresponds to locations
	 * 0-511 (512 bytes) in filesystem. Remember this when reading from file! Offset buf can help.
	 */

	/* convert int to strings and assign values to new entry*/
	new->start = tmp->end+1;

	printf("*** After Start ***\n");
	new->end = (tmp->end + 1) + (blocks - 1);
	new->off = size%BLOCKSIZE;

	printf("*** After Assignments ***\n");

	sprintf(start, "%d", tmp->end+1);
	sprintf(end, "%d", new->end);
	sprintf(off, "%d", new->off);				/* offset is just the remainder of the size/512 */
	ENT = new;	/* store for future reference */

		printf("[!] File Info To Write:\n"
			"\tstart\t%s\n"
			"\tend\t%s\n"
			"\toff\t%s\n"
			,start, end, off);

	/* 
	 * NOTE: all write functions are to memory, NOT the drive. there is a 
	 * separate function to flush the directory to the drive.
	 */
	
	printf("*BEGINNING DIRECTORY WRITE*");
	// writes start block to directory
	for(i = 0; i < strlen(start) + 1; i++) {
		block[location + 16 + i] = start[i];
	}

	printf("\n\tStart Location Writen\n");

	// writes end block to directory
	for(i = 0; i < strlen(end) + 1; i++) {
		block[location + 27 + i] = end[i];
	}

	printf("\n\tEnd Location Writen\n");

	// writes offset to directory
	for(i = 0; i < strlen(off); i++) {
		block[location + 38 + i] = off[i];
	}

	printf("\n\tOffset Amnt Writen\n");

	// update entry count, write to directory
	ENTRIES++;
	block[0] = ENTRIES + '0';

	// debugging print directory
	for(i = 10; i < BLOCKSIZE; i++) {
		printf("%c ", block[i]);
	}

	tmp = HEAD;
	for(i=0;i < ENTRIES; i++) {
		printf("- %s\n", tmp->filename);
		if(ENTRIES > 1) tmp=tmp->next;
	}

	// flushes directory and writes to disk
	fseek(FSPTR, 0, SEEK_SET);
	fwrite(block, sizeof(block), 1, FSPTR);	/* rewrites the directory block */
}

/**
 * Function takes a pointer to the struct containing the updated file info. It
 * then takes them, converts them to a string, and prints them to the directory
 * similar to the way create_entry does it.
 *
 * @param struct entry *tmp		-	pointer to the directory entry
 * @param int Number 			-	position in array [for calculations]
 */
static void update_dir(struct entry *tmp, int index)
{
	char end[11];
	char off[3];
	int location, i;

	printf("[+] update dir of %lf. Number %d\n", tmp->filename, index);
	sprintf(end, "%d", tmp->end);
	sprintf(off, "%d", tmp->off);

	location = (index * 41) + 10;

	// writes end block to directory
	for(i = 0; i < strlen(end) + 1; i++) {
		block[location + 27 + i] = end[i];
	}

	// writes offset to directory
	for(i = 0; i < strlen(off); i++) {
		block[location + 38 + i] = off[i];
	}
	
	/* seek to beginning, write updated dir */
	fseek(FSPTR, 0, SEEK_SET);
	fwrite(block, sizeof(block), 1, FSPTR);
}

static int file_lookup(char *filename, struct stat *stbuf)
{
	struct entry *tmp;	// pointer to struct for searching
	char cpy[16];		// to deal with string copying
	int i;				// for loop later on
	tmp = HEAD;			// points tmp to head
	
	/* VERY IMPORTANT TO CHECK SIZE! */
	if(strlen(filename) > 16) return 0;
	
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
				stbuf->st_mode = S_IFREG | 0777;
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
	printf("Number of files:\t%lf\n", count);

	// reads directories, currently printing locations of dirs in the block
	for(i = 1; i < ENTRIES+1; i++) {

		if(i == 1) {
			prev = malloc(sizeof(struct entry));
			printf("[!] Entry %lf location\t10\n", i);

			memcpy(filename, &block[10], 16);
			filename[16] = '\0';
			printf("[+] filename\t%s\n", filename);
			memcpy(prev->filename, &filename, 16);
			
			// copy start location from directory
			memcpy(start, &block[26], 11);
			start[11] = '\0';
			printf("[+] begin\t%s|%lf\n", start, atoi(start));
			prev->start = atoi(start);

			// copy end location from directory
			memcpy(end, &block[37], 11);
			end[11] = '\0';
			printf("[+] end\t\t%s|%lf\n", end, atoi(end));
			prev->end = atoi(end);

			// copy the offset of the last block from mem
			memcpy(off, &block[48], 3);
			off[3] = '\0';
			printf("[+] offset\t%s|%lf\n", off, atoi(off));
			prev->off = atoi(off);

			HEAD = prev;

			struct stat stbuf;
		 	stbuf.st_ino = i+10;
		 	stbuf.st_mode = S_IFREG | 0777;
		 	stbuf.st_nlink = 10;
		 	stbuf.st_size = (((prev->end - prev->start)) * 512) + prev->off;
		 	prev->info = stbuf;

		}
		else {
			struct entry *tmp = malloc(sizeof(struct entry));

			begin = 10 + (41*(i-1));		// calculate the location of entry
			printf("[!] Entry %lf location\t%lf\n", i, begin);
			
			// copy filename from dir and into array for struct use
			memcpy(filename, &block[begin], 16);
			filename[16] = '\0';
			printf("[+] filename\t%s\n", filename);
			memcpy(tmp->filename, filename, 16);

			// copy start location from directory
			memcpy(start, &block[begin+16], 11);
			start[11] = '\0';
			printf("[+] begin\t%s|%lf\n", start, atoi(start));
			tmp->start = atoi(start);

			// copy end location from directory
			memcpy(end, &block[begin+27], 11);
			end[11] = '\0';
			printf("[+] end\t\t%s|%lf\n", end, atoi(end));
			tmp->end = atoi(end);

			// copy the offset of the last block from mem
			memcpy(off, &block[begin+38], 3);
			off[3] = '\0';
			printf("[+] offset\t%s|%d\n", off, atoi(off));
			tmp->off = atoi(off);


			struct stat st;
		 	st.st_ino = i+10;
		 	st.st_mode = S_IFREG | 0777;
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
		printf("[-] number \t%lf\tname\t%s\n", i, print->filename);
		print = print->next;
	}
}

/**
 * Implemented function from fuse.h header file. Called when the OS needs to know information
 * (attributes) regarding a file or directory. The file path is passed through the path var 
 * while the pointer to the stat struct (a UNIX standard struct) is passed in the stbuf var.
 * Right now the system only accepts files and not directories, so if the path consists of 
 * the root dir "/" it returns those attributes to the stat. Otherwise it checks if the file
 * requested is in the filesystem. Finally, if it is not one of the hidden file containing a 
 * '.' at the beginning, it can be considered a file to create and is given the proper attrib.
 *
 * @param path 		- path to the file
 * @param stbuf		- pointer to the stat struct
 */
static int myfs_getattr(const char *path, struct stat *stbuf)
{
	char cpy[16]; 			// buffer for removing first char 
	memset(stbuf, 0, sizeof(struct stat));
	printf("[!] GETATTR called\tpath: %s\tname length: %lf\n", path, strlen(path));

	/* can do this neater in future... stops from breaking */
	if(strlen(path) > 15) return -ENOENT;
	/* checks if OS requesting root directory info */
	if(strcmp(path, "/") == 0) {
		printf("[+] root dir\n");
		stbuf->st_ino = 2;
		stbuf->st_mode = S_IFDIR | 0755;
		return 0;
	} if(strcmp(path, "/autorun.inf") == 0|| strcmp(path, "/AACS") == 0|| strcmp(path, "/BDSVM") == 0 || strcmp(path, "/BDMV") == 0) {
		return -ENOENT;
	} if(file_lookup(path, stbuf) == 1) {	//TODO: Breaks looking up some files, fix this!
		printf("[+] resetting attributes\n");
		stbuf->st_mode = S_IFREG | 0777;		// leaving this for now, will get from stbuf in future 
		stbuf->st_nlink = 1;
		return 0;
	} else if(path[1] != '.') {
		printf("[+] Potential new file path:\t%s\n", path);
		stbuf->st_mode = S_IFREG | 0777;
		return 0;
	} else {
		printf("[+] Nothing found. Second char: %c\n", path[1]); // debugging to ensure we can ignore paths with a '.'
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
			printf("What's going on?\n");
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
	char data[131072];		// buffer to read from file
	struct entry *tmp;		// pointer to head of directory list
	int i, end;				// for loop stuff & location storage
	double start, rsize;
	
	/* copy string & get rid of '/' */
	strcpy(cpy, path);
	memmove(cpy, cpy + 1, strlen(cpy));

	/* testing finding the file to change */
	tmp = HEAD;
	for(i = 0; i < ENTRIES; i++) {		// loop through all entries, find matching filename
		if(strcmp(cpy, tmp->filename) == 0) {
			break;
		} 
		else tmp = tmp->next;
	}

	/* mostly testing stuff here... */
	printf("[!] READ called\tpath:\t%s\n\t-start: %lf\n\t-end: %lf\n", path, tmp->start, tmp->end);
	if(tmp->info.st_size - offset > size) {
		start = ((tmp->start - 1) * BLOCKSIZE) + offset;
		fseek(FSPTR, start, SEEK_SET);
		fread(data, size, 1, FSPTR);
		memcpy(buf, data, size);
		total = size;
	} else { // We can fill buffer now
		printf("[!] filesize smaller than buffer\n");
		start = ((tmp->start - 1) * BLOCKSIZE) + offset;
		/* 
		 * subtract read from total size for amount to read,
		 * read that amount into the new large buffer and then
		 * write it to the FUSE buffer.
		 */
		rsize = tmp->info.st_size - offset;
		fseek(FSPTR, start, SEEK_SET);
		fread(data, rsize, 1, FSPTR);
		memcpy(buf, data, rsize);
		total = rsize;
	}
	return total; // Returns number of bytes read, will call again with new buffer if not reached size yet! Uses 8byte buffer by default
	// offset approaches size
}

/*
 * For this case, write will assume that items are only being copied into the filesysten and not edited. Therefore,
 * we can assume that the size is 0 until this is called. In which case, the new filesize will be the size which is
 * passed into the function as a parameter. 
 */
static int myfs_write(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	char cpy[16];
	struct entry *tmp;
	double i, addblock;
	printf("[!] WRITE called on path: %s\tsize: %zu\toffset:%zu\n", path, size, offset);
	tmp = HEAD;
	/* 
	 * TODO:
	 * Function needs to:
	 *		1) check if file exists (it should from open())
	 *		2) update the size within the stat struct
	 *		3) calculate blocks
	 *		4) update entry struct with size info
	 */
	
	if(file_lookup(path, NULL) == 0) {
		strcpy(cpy, path);
		memmove(cpy, cpy + 1, strlen(cpy));
		create_entry(cpy, size); /* Breaks here now! */
		printf("New Created! Size: %lld\toffset: %lld\n", size, offset);

		/* going to want to remove this/ find alternative. Inefficient! */
		tmp = HEAD; // IMPORTANT: Reassigns for when HEAD had to be created as the first file
		printf("\n\n\nFILENAME\t%s\n\n\n", tmp->filename);
		for(i = 0; i < ENTRIES; i++) {
			if(strcmp(cpy, tmp->filename) == 0) {
				printf("[+] %lf added. New size %lf\n", size, tmp->info.st_size);
				double start = (tmp->start * BLOCKSIZE) - 512;
				printf("[+] Write at location %lf beginning at %lf\n", start + offset, start);
				fseek(FSPTR, start+offset, SEEK_SET);
				fwrite(buf, 1, size, FSPTR);
			}
			tmp = tmp->next;
		}
	} else {
		/* going to want to remove this/ find alternative. Inefficient! */
		for(i = 0; i < ENTRIES; i++) {
			strcpy(cpy, path);
			memmove(cpy, cpy + 1, strlen(cpy));
			printf("[+] Looking for entries matching %s\n", cpy);
			/*IMPORTANT!: This needs to update the blocks used by the file*/
			if(strcmp(cpy, tmp->filename) == 0) {
				tmp->info.st_size += size;
				printf("[+] %lf added. New size %lf\n", size, tmp->info.st_size);
				/* check if we need another block added for offset */
				addblock = size/BLOCKSIZE;
				if(size % BLOCKSIZE > 0){ 
					addblock++;
					tmp->off = size%BLOCKSIZE;
				}
				tmp->end += addblock;
				printf("[+] need to add %lf blocks to end\n", addblock);

				/* find block location to write and write given bytes */
				double start = (tmp->start * BLOCKSIZE) - 512;
				printf("[+] Write at location %d beginning at %d\n", start + offset, start);
				fseek(FSPTR, start+offset, SEEK_SET);
				fwrite(buf, 1, size, FSPTR);

				/* update size */
				update_dir(tmp, i);

			}
			tmp = tmp->next;
		}
	}

	printf("[-] Need to update sizes for %s\n", ENT->filename);
	printf("\t-start %d\n\t-end %d\n", ENT->start, ENT->end);

	return size;
}

static int myfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[!] CREATE called\n");
	return 0;
}

static int myfs_truncate(const char *path, off_t offset)
{
	printf("[!] TRUNCATE called on path:\t%s\toffset:\t%zu\n", path, offset);
	return 0;
}

/*
 * Function called to delete files. Takes the pathname. Note that this function is not
 * completely implemented and does everything short of rewriting to the filesystem. It
 * will calculate new locations, etc though.
 */
static int myfs_unlink(const char *path)
{
	char cpy[16];
	struct entry *tmp;
	int i;
	double moveLoc, size;
	
	/* copy string & get rid of '/' */
	strcpy(cpy, path);
	memmove(cpy, cpy + 1, strlen(cpy));

	/* debugging nonsense */
	printf("[!] unlink is called %s\n", cpy);
	if(ENTRIES == 0) return 0;

	/* finding the file*/
	tmp = HEAD;
	i = 0;
	while(i < ENTRIES) {
		if(strcmp(cpy, tmp->filename) == 0) {
			printf("[!] file to remove found!\n");
			break;
		}

		/*
		 * we want to end here if the file is not in the
		 * filesystem or else we risk deleting the last 
		 * file. Check here and return if necessary.
		 */
		if(i == ENTRIES - 1) {
			printf("[!] Last file, checking\n");
			if(strcmp(cpy, tmp->filename) != 0) {
				printf("[-] No such file in system\n");
				return 0;
			}
		} else {
			tmp = tmp->next;
			i++;
			printf("CRAP\n");
		}
	}

	printf("[+] Continuing file removal\n");

	/* move rest of system up if there are others after the deleted entry */
	if(tmp->next != NULL) {
		/* calculate space displaced and free struct */
		//printf("[!] Location of first block is %d\n[!] Reclaiming %d blocks\n", tmp->start, tmp->end - tmp->start);
		//moveLoc = ((tmp->start - 1) * 512) + 10;
		//printf("[+] Moving all files up to position %lf\n", moveLoc);
		size = (tmp->next->end - tmp->next->start) + 1;
		//printf("[+] Size of moved file is %f blocks\n[+] New end is %f\n", size, tmp->start + size - 1);
		
		/* update structs */
		tmp->next->end = tmp->start + size - 1;   // sub one b/c it's inclusive
		tmp->next->start = tmp->start;
		printf("[!] filename\t%s\n\tbegin block\t%f\n\tend block\t%f\n", tmp->next->filename, tmp->next->start, tmp->next->end);
		tmp = tmp->next;

		/* calculate where data is moved to for next directory */
		printf("[+] New locations for file data\n\tbegin\t%f\n\tend\t%f\n",
			(tmp->start - 1) * 512, ((tmp->end - 1) * 512) + tmp->off - 1);


		/* change locations of rest of files in dir */
		while(tmp->next != NULL) {
			double diff = tmp->next->end - tmp->next->start;
			tmp->next->start = tmp->end + 1; 
			tmp->next->end = tmp->next->start + diff;
			printf("[!] Calculating Next\n\tbegin:\t%f\n\tend:\t%f\n", tmp->next->start, tmp->next->end);
			tmp = tmp->next;
		}
	} else {
		/* deals with the case wehre last added is removed */
		printf("[!] The last entry is the one removed\n");
	}

	return 0;
}

static int myfs_chown()
{
	return 0;
}

static int myfs_utimens()
{
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
	.unlink		= myfs_unlink,
	.truncate 	= myfs_truncate,
	.chown 		= myfs_chown,
	.utimens	= myfs_utimens,
};

int main(int argc, char *argv[])
{
	FSPTR = fopen(filename, "r+");
	init_dir(filename);
	return fuse_main(argc, argv, &myfs_ops, NULL);	// FUSE taking over here
}
