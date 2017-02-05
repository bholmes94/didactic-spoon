# didactic-spoon | Overview

This is a collection of files which contain my capstone project. The general idea is to create a filesystem which is capable of transferring virtually any sized file between any operating system. Specifically, macOS, Linux and Windows. The entire project can be divided into two pieces; the actual filesystem structure and FUSE. Now since FUSE was designed to work with UNIX-like operating systems, creating a Windows version of this utility will require the use of wrappers and is thus still being worked on. 

There are a few large caveats to take into consideration with this filesystem. First off, the function of this filesystem is to transfer large files (4GB and up) between various systems with minor effort from the user's end. With this in mind, the filesystem relies on contiguous blocks of storage. So this scheme must be maintained, even between deletes (which are being worked on). The problem here is if there is a small file, say 4KB, is inserted followed by a 3GB file, then removed. That entire 3GB file plus any subsequent file, needs to be moved that tiny space to keep the contiguous scheme. This is VERY inefficient. I could ignore these small holes as just not worth the time but they would likely never be filled. 

Another efficiency issue is the way that the program currently handles reads and writes. To make things easier to port to Windows, which reads block-by-block, I have the file only fill the read/ write buffer with 512bytes when the max is substantially more, ~65k bytes by default I believe. In addition to requiring more read and write calls, the way I find the file within those needs work too. The program will search the entire array EVERY time Read() or Write() is called and continue from there. This combined with the small buffer causes much larger than needed transfer times. When things are working comfortably, I will increase the buffer size and use a global pointer to reduce this strain.

#Included Files
myfs_linux.c: 
This program is being worked on. The way the Read() and Write() calls are slightly different than in the macOS version. So the upload with a working version will be up shortly. There are also more files that are being passed to the Getattr() call which needs to be filtered out as the program thinks a user is adding a new file when they aren't.

myfs_regular.c:
This is the most complete program here. It has reading and writing. Although you will need to be sure to give the user permissions to use the file within the /dev/ directory corresponding to the USB drive to be used. Otherwise a segfault is issued for a permissions error. There is currently no way to remove a file and when you do add a file I have commented out the directory write so it is never written to the directory. Restarting will clear the added entry.

fstester.c:
This program is used to insert data and test functionality of the filesystem without getting FUSE involved. No real bugs in here yet.