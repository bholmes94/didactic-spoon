# didactic-spoon | Overview

This is a collection of files which contain my capstone project. The general idea is to create a filesystem which is capable of transferring virtually any sized file between any operating system. Specifically, macOS, Linux and Windows. The entire project can be divided into two pieces; the actual filesystem structure and FUSE. Now since FUSE was designed to work with UNIX-like operating systems, creating a Windows version of this utility will require the use of the [Dokany](https://github.com/dokan-dev/dokany "Dokany") library which is similar to FUSE, excepted based around the Windows system calls.

There are a few large caveats to take into consideration with this filesystem. First off, the function of this filesystem is to transfer large files (4GB and up) between various systems with minor effort from the user's end. With this in mind, the filesystem relies on contiguous blocks of storage. So this scheme must be maintained, even between deletes. The problem here is if a small file, say 4KB, is inserted followed by a 3GB file then removed, that entire 3GB file plus any subsequent file(s), needs to be moved that tiny space to keep the contiguous scheme. This is VERY inefficient. I could ignore these small holes as just not worth the time but they would likely never be filled. 

There is currently an inefficiency which will soon be fixed on the macOS version where the program does not utilize the full buffer when performing reads and writes. As a result, more read/ write calls will need to be made in order to complete the file and the transfer times suffer. This is something that has been fixed on the Linux version and will not be present on the Windows version either.

# Included Files
Below are the files in the repository along with a brief description of each. It is worth noting that the device locations (i.e. /dev/sda) are hard-coded and will need to be changed accordingly before re-compiling and running. This is something that will be changed to a command line argument as re-compiling every single time a device changes is unnecessary. Another unhandled error is when the program is run without sufficient permissions to access the device. You can either run the program as root or change the permissions of the specific device prior to running the program. Otherwise, a segfault will be issued and you will need to manually unmount filesystem by using the following command `fusermount -u [mount path]`. After that you can re-run the program.

* myfs_linux.c:
A program for the Linux version of the filesystem. Instructions to compile and install are included below in the Installation section. This program relies on FUSE which allows the user to implement the included callbacks inside of this file. The buffer that is going to be used for the function re-arranging the directory will be small at first as it is not expected the user will be moving around files often, as they are quite large and wiping the drive would be easier. A fix should be coming to make use of a larger buffer.

* lffs_mac.c:
The macOS version of my filesystem. There are a few strange bugs that I had not run into before that will be addressed. This right now is essentially the Linux version with an explicit path to the fuse.h file that is installed with osxfuse. Documentation and info on this can be found [osxfuse]("https://osxfuse.github.io" "here"). It essentially allows programs and callbacks designed for FUSE to work with macOS as well. One of the primary issues here is that dragging and dropping a file into the drive doesn't seem to work. However using the `cp [filename] <mounted folder>` works without issue. Looking into this currently. This is using osxfuse 3.5.8. 

* fstester.c:
This program is used to insert data and test functionality of the filesystem without getting FUSE involved. No real bugs in here yet.

# Installation
## Linux
In order to run the application on Linux right now, you will need to have FUSE installed. For testing I have been using version 2.9.7. Additionally, you will need to know the name of the device you are using to transfer files within the /dev/ folder. This is something I plan to replace with something else soon. Once you have these two and the device filename is placed in the filesystem variable, you can compile the program using `gcc -Wall myfs_linux.c 'pkg-config fuse --cflags --libs' -o myfs_linux`. Running the program follows the template of: `./myfs_linux -s [mount folder]`. There are other arguments you can pass which are fed to FUSE such as `-d` for debugging 
or `-f` to run the application in the foreground. The `-s` argument is to run this in a single thread, which has been working fine. I have not tested it without this argument so far but feel free to do so.

## macOS
To install and run the program on macOS, you will first need to have a version of [osxfuse]("https://osxfuse.github.io" "osxfuse") installed. Once that is installed, you can compile the program using `gcc lffs_mac.c -I /usr/local/include/osxfuse/fuse.h -D_FILE_OFFSET_BITS=64 -v -L /usr/local/lib -losxfuse -o lffs_mac`. Note that the paths might have to be changed to suite the install locations. As far as I have read, these locations are default though. The compilation flags were helped by a few sites, I'll track them down and link them here.


## Windows
The first working version of the Windows filesystem is now up [windows version](https://github.com/bholmes94/WinLFFS "here"). You will need to visit the Dokany page and download the installer which should give you the files needed to run and test this program. This is availible [here](https://github.com/dokan-dev/dokany/wiki/Installation "here"). Note that I have been testing on an up to date Windows 10 Pro operating system. I have not had a chance to test on other versions. This program also does everything but actually write to the USB device. I have been holding off to ensure the calculations from the other programs on Linux and macOS work properly. This should be done very soon.
