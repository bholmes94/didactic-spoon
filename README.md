# didactic-spoon

Current Large Problems:
	1) fstester management tool will write to a any zero even if it is in the middle of a file. So files can overwrite other files unintentionally. This is something that will need to be changed. Maybe the algorithm can check the next two bytes to make sure that they are both not null. Another option is checking that the 0 isn't already falling in between the location of an existing file. This second option is better.

	2) After a certain number of read calls, the program can no longer open the filesystem. This may be a security thing on macOS limiting the rate of which files are opened and closed. Will probably just open the filesystem and have a global var file pointer.