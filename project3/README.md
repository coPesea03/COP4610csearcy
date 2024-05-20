# FAT32 File System Project

The purpose of this project is to familiarize you with basic file-system design and implementation. You will need to understand various aspects of the FAT32 file system, such as cluster-based storage, FAT tables, sectors, and directory structure.

## Group Information

- **Group Number:** 20
- **Group Name:** Group 20
- **Group Members:**
  - Connor Searcy: cps21a@fsu.edu
  - Michael Clark: mtc21c@fsu.edu
  - Shirel Baumgartner: sb21l@fsu.edu

## IMPORTANT NOTE
The makefile creates the executable no matter what, but 
```bash
make run
```
only works if the image file is named fat32.img and issaved inside of the filesystem directory.There was no guideline on whether we needed an image file to be saved in the project. I have left the default fat32.img sitting in the filesystem/ directory for you.


## Division of Labor:

### Part 1: Mount the Image File

The user will need to mount the image file through command line arguments:

./filesys [FAT32 ISO] -> [1pt]

You should read the image file and implement the correct structure to store the format of FAT32 and navigate it.
You should close out the program and return an error message if the file does not exist.

Your terminal should look like this:

[NAME_OF_IMAGE]/[PATH_IN_IMAGE]/>

The following commands will need to be implemented:

info -> [3 pts]

    Parses the boot sector. Prints the field name and corresponding value for each entry, one per line 

exit -> [1 pt]

    Safely closes the program and frees up any allocated resources.

Completed by: Connor Searcy and Michael Clark

### Part 2: Navigation

You will need to create these commands that will allow the user to navigate the image.

cd [DIRNAME] -> [5 pts]

    Changes the current working directory to [DIRNAME]. ‘.’ and ‘..’ are valid directory names to cd. 

ls -> [5 pts]

    Print the name filed for the directories within the current working directory including the “.” And “..” directories.

Completed By: Connor Searcy and Michael Clark

### Part 3: Create

You will need to create commands that will allow the user to create files and directories.

mkdir [DIRNAME] -> [8 pts]

    Creates a new directory in the current working directory with the name [DIRNAME]. 

creat [FILENAME] -> [7 pts]

    Creates a file in the current working directory with a size of 0 bytes and with a name of [FILENAME]. The [FILENAME] is a valid file name, not the absolute path to a file.


Completed By: /

### Part 4: Read

You will need to create commands that will read from opened files. Create a structure that stores which files are opened.

open [FILENAME] [FLAGS] -> [4 pts]

    Opens a file named [FILENAME] in the current working directory. A file can only be read from or written to if it is opened first. You will need to maintain some data structure of opened files and add [FILENAME] to it when open is called. [FLAGS] is a flag and is only valid if it is one of the following (do not miss the ‘-‘ character for the flag):
        -r - read-only.
        -w - write-only.
        -rw - read and write. 
        -wr - write and read.
    Initialize the offset of the file at 0. Can be stored in the open file data structure along with other info.
    Print an error if the file is already opened, if the file does not exist, or an invalid mode is used.

close [FILENAME] -> [2 pts]

    Closes a file [FILENAME] in current working directory.
    Needs to remove the file entry from the open file data structure.
    Print an error if the file is not opened, or if the file does not exist in current working directory.

lsof -> [2 pts]

    Lists all opened files.
        Needs to list the index, file name, mode, offset, and path for every opened file. 
        If no files are opened, notify the user.

lseek [FILENAME] [OFFSET] -> [2 pts]

    Set the offset (in bytes) of file [FILENAME] in current working directory for further reading or writing.
        Store the value of [OFFSET] (in memory) and relate it to the file [FILENAME]. 
        Print an error if file is not opened or does not exist.
        Print an error if [OFFSET] is larger than the size of the file.

read [FILENAME] [SIZE] -> [10 pts]

    Read the data from a file in the current working directory with the name [FILENAME], and print it
    out.
        Start reading from the file’s stored offset and stop after reading [SIZE] bytes.
        If the offset + [SIZE] is larger than the size of the file, just read until end of file.
        Update the offset of the file to offset + [SIZE] (or to the size of the file if you reached the end of the file).
        Print an error if [FILENAME] does not exist, if [FILENAME] is a directory, or if the file is not opened for reading.

Completed By: Shirel Baumgartner

### Part 5: Update

You will need to implement the functionality that allows the user to write to a file.

write [FILENAME] [STRING] -> [10 pts]

    Writes to a file in the current working directory with the name [FILENAME].
        Start writing at the file’s offset and stop after writing [STRING].
        [STRING] is enclosed in “”. You do not need to wrong about “” in [STRING].
        If offset + size of [STRING] is larger than the size of the file, you will need to extend the length of the file to at least hold the data being written.
        Update the offset of the file to offset + size of [STRING].
        Print an error if [FILENAME] does not exist, if [FILENAME] is a directory, or if the file is not opened for writing.

Completed By: Shirel Baumgartner

### Part 6: Delete

You will need to implement the functionality that allows the user to delete a file/directory.

rm [FILENAME] -> [5 pts]

    Deletes the file named [FILENAME] from the current working directory.
        This means removing the entry in the directory as well as reclaiming the actual file data. 
        Print an error if [FILENAME] does not exist or if the file is a directory or if it is opened.

rmdir [DIRNAME] -> [5 pts]

    Remove an empty directory by the name of [DIRNAME] from the current working directory.
        This command alone can only be used on an empty directory (if a directory only contains ‘.’ and ‘..’, it is an empty directory).
        Make sure to remove the entry from the current working directory and to remove the data [DIRNAME] points to.
        Print an error if the [DIRNAME] does not exist, if [DIRNAME] is not a directory, or [DIRNAME] is not an empty directory or if a file is opened in that directory.

Completed By: /

### Extra Credit

For extra credit you can implement:

rm -r [DIRNAME] -> [5 pts]

    The -r option adds the ability to rm to delete a directory and its content (e.g., files, subdirectories) recursively.
    You can assume no files are opened in this directory or child directories. 

Completed By: /

## File Listing:

```plaintext
project-3-os-group-20/
├── filesystem/
│   ├── include/
│   │   ├── lexer.h
│   ├── src/
│   │   ├── lexer.c
│   │   ├── fat32.c
|   ├── fat32.img
│   ├── Makefile
├── README.md
```
## How to Compile & Execute:

```bash
git clone https://github.com/FSU-COP4610-S24/project-3-os-group-20.git
```
Compile the program on linprog using the provided Makefile. To execute, run the executable named " filesys" created by the makefile inside of bin/. If you choose to manually run the executable by navigating to bin/ you will have to pass in an image file to be opened. 

This command will BUILD AND RUN the executable located at: /bin/filesys with the image stored in filesystem/. 
```bash
make run
```
This command requires the image to be named fat32.img.

## Requirements: 
### Compiler: 
gcc compiler 
### Dependencies: 
The makefile makes the executable no matter what, but 
```bash
make run
```
only works if the image file is named fat32.img and is saved inside of the filesystem directory. There was no guideline on whether we needed an image file to be saved in the project. I have left the default fat32.img sitting in the filesystem/ directory for you.

## Bugs

1. 
2. 
3. 

## Extra Credit

Extra Credit 1: 

## Considerations

The makefile makes the executable no matter what, but 
```bash
make run
```
only works if the image file is named fat32.img and issaved inside of the filesystem directory.
There was no guideline on whether we needed an image file to be saved in the project. I have left the default fat32.img sitting in the filesystem/ directory for you.