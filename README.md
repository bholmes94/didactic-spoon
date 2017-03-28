# didactic-spoon | Overview

This is a collection of files which contain my capstone project. The general idea is to create a filesystem which is capable of transferring virtually any sized file between any operating system. Specifically, macOS, Linux and Windows. The entire project can be divided into two pieces; the actual filesystem structure and FUSE. Now since FUSE was designed to work with UNIX-like operating systems, creating a Windows version of this utility will require the use of wrappers and is thus still being worked on. 

There are a few large caveats to take into consideration with this filesystem. First off, the function of this filesystem is to transfer large files (4GB and up) between various systems with minor effort from the user's end. With this in mind, the filesystem relies on contiguous blocks of storage. So this scheme must be maintained, even between deletes (which are being worked on). The problem here is if there is a small file, say 4KB, is inserted followed by a 3GB file, then removed. That entire 3GB file plus any subsequent file, needs to be moved that tiny space to keep the contiguous scheme. This is VERY inefficient. I could ignore these small holes as just not worth the time but they would likely never be filled. 

There is currently an inefficiency which will soon be fixed on the macOS version where the program does not utilize the full buffer when performing reads and writes. As a result, more read/ write calls will need to be made in order to complete the file and the transfer times suffer. This is something that has been fixed on the Linux version and will not be present on the Windows version either. A fix is coming shortly for the macOS version.

# Included Files
Below are the files in the repository along with a brief description of each. It is worth noting that the device locations (i.e. /dev/sda) are hard-coded and will need to be changed accordingly before running. Another unhandled error is when the program is run without sufficient permissions to access the device. You can either run the program as root or change the permissions of the specific device prior to running the program. Otherwise, a segfault will be issued and you will need to manually unmount filesystem.

* myfs_linux.c:
A program for the Linux version of the filesystem. Slightly different read/ write calls than with the macOS version of FUSE. Deleting almost completely implemented.

* myfs_regular.c:
This program (soon to be renamed) is the macOS version. Still needs to be caught up to the Linux version but this should not be a challenge as most of the code is comepletely portable between the two systems.

* fstester.c:
This program is used to insert data and test functionality of the filesystem without getting FUSE involved. No real bugs in here yet.

* WIN_LFFS.c:
Not here yet, will be soon. The Windows version!
