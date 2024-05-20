#include "lexer.h"

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>

typedef struct __attribute__((packed)) BPB {
    uint8_t BS_jmpBoot[3];      // Jump instruction to boot code
    char BS_OEMName[8];         // OEM Name in ASCII
    uint16_t BPB_BytsPerSec;    // Bytes per sector
    uint8_t BPB_SecPerClus;     // Sectors per cluster
    uint16_t BPB_RsvdSecCnt;    // Reserved sectors count
    uint8_t BPB_NumFATs;        // Number of FAT data structures
    uint16_t BPB_RootEntCnt;    // Number of directory entries in root (FAT12/16)
    uint16_t BPB_TotSec16;      // Total sectors (FAT12/16)
    uint8_t BPB_Media;          // Media type
    uint16_t BPB_FATSz16;       // FAT size in sectors (FAT12/16)
    uint16_t BPB_SecPerTrk;     // Sectors per track
    uint16_t BPB_NumHeads;      // Number of heads
    uint32_t BPB_HiddSec;       // Hidden sectors
    uint32_t BPB_TotSec32;      // Total sectors (FAT32)
    uint32_t BPB_FATSz32;       // FAT size in sectors (FAT32)
    uint16_t BPB_ExtFlags;      // Extended flags
    uint16_t BPB_FSVer;         // File system version
    uint32_t BPB_RootClus;      // Root directory cluster number
    uint16_t BPB_FSInfo;        // FSInfo structure sector number
    uint16_t BPB_BkBootSec;     // Backup boot sector
    uint8_t BS_DrvNum;          // Drive number
    uint8_t BS_Reserved1;       // Reserved (used by Windows NT)
    uint8_t BS_BootSig;         // Boot signature   
    uint32_t BS_VolID;          // Volume serial number
    char BS_VolLab[11];         // Volume label
    char BS_FilSysType[8];      // File system type
} bpb_t;

typedef struct __attribute__((packed)) directory_entry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    char padding_1[8]; // DIR_NTRes, DIR_CrtTimeTenth, DIR_CrtTime, DIR_CrtDate, 
                       // DIR_LstAccDate. Since these fields are not used in
                       // Project 3, defined as a placeholder.
    uint16_t DIR_FstClusHI;
    char padding_2[4]; // DIR_WrtTime, DIR_WrtDate
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} entry_t;

//**********************************************************************************************//

typedef struct openedfile {
    char* filename;
    char* flag;
    char* cwd;
    uint32_t cluster_offset;
    uint32_t file_size;
    uint32_t offset; // Current offset in the file for read/write operations
} openedfile;

typedef struct openedfile_node {
    openedfile file;
    struct openedfile_node* next;
} openedfile_node;

openedfile_node* head = NULL; // Head of the linked list of opened files
int filesOpened = 0; // Counter for the number of files currently opened

//**********************************************************************************************//

FILE *fat32_img;
bpb_t bpb;

#define MAX_PATH 260
#define SIZE_OF_PATH_LIST 20

char img_name[MAX_PATH];

char current_path[MAX_PATH];
uint32_t curr_root;
uint32_t previousCurrRoot[SIZE_OF_PATH_LIST];
int depth;

//for mkdir
uint32_t prev;


//prototypes (indents indicate helper functions)
int fileno(FILE*);
char* strdup(const char*);
ssize_t pread(int, void*, size_t, off_t );
void infoCommand();
void displayPrompt();
uint32_t convert_offset_to_clus_num_in_fat_region(uint32_t);
uint32_t convert_clus_num_to_offset_in_fat_region(uint32_t);
void lsCommand(const char*);
void changeDirectory(const char*, const char*);
    void rtrim(char*);
    void to_upper(char*);

void openFile(char* filename, char* flag, char* cwd, openedfile_node** head,
              uint32_t curr_cluster, int fd_for_img);
// int closeFile(char* filename, openedfile_node** head);
int closeFile(char* filename, openedfile_node** head);

void lseekFile(const char* filename, int offset, openedfile_node** head);
void readFile(char* filename, int size, openedfile_node** head, int fd_for_img);

void writeFile(char* filename, char* data, openedfile_node** head, int fd_for_img);


//initializaiton information
void mount_fat32(const char *img_path) {
    strcpy(img_name, img_path);
    fat32_img = fopen(img_path, "r+");
    if (!fat32_img) {
        perror("Failed to open FAT32 image");
        exit(1);
    }

    // Read the BPB into the bpb_t structure
    if (fread(&bpb, sizeof(bpb_t), 1, fat32_img) != 1) {
        perror("Failed to read BPB");
        fclose(fat32_img);
        exit(1);
    }

    for(int i = 0; i < SIZE_OF_PATH_LIST; i++){
        previousCurrRoot[i] = 0x00;
    }

    strcpy(current_path, "/");
    curr_root = bpb.BPB_RootClus;
    depth = 0;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("Error, missing argument. Proper Usage: %s [FAT32 image path]\n", argv[0]);
        return 1;
    }

    mount_fat32(argv[1]);
    
    while(1){
        displayPrompt();
        tokenlist *tokens = shell();

        //sets the first value of the string to command for simplicity
        char *command = tokens->items[0]; 

        //validates non null list of tokens with at least one token
        if(tokens != NULL && tokens->size > 0){
            if(strcmp(command, "ls") == 0){                 //ls activation
                lsCommand(argv[1]);

            } else if(strcmp(command, "exit") == 0){        //exit activation
                free_tokens(tokens);
                break;

            } else if(strcmp(command, "cd") == 0){          //cd activation
                if(tokens->size > 1 && tokens->items[1] != NULL){
                    changeDirectory(tokens->items[1], argv[1]);
                } else{
                    printf("cd needs an additional argument\n");
                }

            } else if(strcmp(command, "info") == 0){         //info activation
                infoCommand();

            } else if (strcmp(command, "open") == 0) {       //open activation
                if (tokens->size < 3) {
                    printf("Usage: open <filename> <mode>\n");
                } else {
                    char* filename = tokens->items[1];
                    char* mode = tokens->items[2];
                    if (strcmp(mode, "-r") != 0 && strcmp(mode, "-w") != 0 && strcmp(mode, "-rw") 
                        != 0 && strcmp(mode, "-wr") != 0) {
                        printf("Invalid mode. Use -r, -w, -rw, or -wr.\n");
                    } else {
                        openFile(filename, mode, current_path, &head, curr_root, 
                            fileno(fat32_img));
                    }
                }
            } else if (strcmp(command, "close") == 0) {      //close activation
                if (tokens->size < 2) {
                    printf("Usage: close <filename>\n");
                } else {
                    char* filename = tokens->items[1];
                    int result = closeFile(filename, &head);
                    if (result == 1) {
                        printf("File closed: %s\n", filename);
                    } else {
                        printf("File not opened or does not exist: %s\n", filename);
                    }
                }
            } else if (strcmp(tokens->items[0], "lsof") == 0) { //lsof activation
                if (filesOpened == 0) {
                    printf("No files are currently opened.\n");
                } else {
                    printf("%-5s%-20s%-10s%-10s%-s\n", 
                        "Index", "File Name", "Mode", "Offset", "Path");
                    openedfile_node* current = head;
                    int index = 1;

                    while (current != NULL) {
                        printf("%-5d%-20s%-10s%-10d%s\n", 
                            index++, 
                            current->file.filename, 
                            current->file.flag,
                            current->file.offset, 
                            current->file.cwd);
                        current = current->next;
                    }
                }
            } else if (strcmp(command, "lseek") == 0) {   //lseek activation
                if (tokens->size < 3) {
                    printf("Usage: lseek <filename> <offset>\n");
                } else {
                    char* filename = tokens->items[1];
                    int offset = atoi(tokens->items[2]);  // Convert offset from string to integer
                    lseekFile(filename, offset, &head);
                }
            } else if (strcmp(command, "read") == 0) {    //read activation
                if (tokens->size < 3) {
                    printf("Usage: read <filename> <size>\n");
                } else {
                    char* filename = tokens->items[1];
                    int size = atoi(tokens->items[2]);  // Convert size from string to integer
                    if (size <= 0) {
                        printf("Error: Invalid size.\n");
                        continue;
                    }
                    readFile(filename, size, &head, fileno(fat32_img));
                }
            } else if (strcmp(command, "write") == 0) {    //write activation
                if (tokens->size < 3) {
                    printf("Usage: write <filename> \"<data>\"\n");
                } else {
                    char* filename = tokens->items[1];
                    char* data = tokens->items[2]; 
                    writeFile(filename, data, &head, fileno(fat32_img));
                }
            } else { //if tokens is null or tokens is less than 1, send notice and free memory
                printf("No valid command detected\n");
                free_tokens(tokens);
                continue;
            }

        }
        free_tokens(tokens);

    }
    fclose(fat32_img);
    return 0;
}


//===========================================================//
//===================== FUNCTIONS LIST ======================//
//===========================================================//


void displayPrompt(){
     printf("%s%s> ", img_name, current_path);
}

void infoCommand(){
    fseek(fat32_img, 0, SEEK_END);
    long file_size = ftell(fat32_img);
    fseek(fat32_img, 0, SEEK_SET);

    printf("Bytes per Sector: %hu\n", bpb.BPB_BytsPerSec);
    printf("Sectors per Clutster: %hhu\n", bpb.BPB_SecPerClus);
    printf("Root Cluster: %u\n", bpb.BPB_RootClus);
    printf("Total Number of Clusters in Data Region: %u\n", ((bpb.BPB_TotSec32 - 
        (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz32))/bpb.BPB_SecPerClus));
    printf("Number of Entries in one fat: %u\n", ((bpb.BPB_FATSz32 * bpb.BPB_BytsPerSec) / 4 ));
    printf("Size of Image (bytes): %ld\n", file_size );
}

// the offset to the beginning of the file.
uint32_t convert_offset_to_clus_num_in_fat_region(uint32_t offset) {
    uint32_t fat_region_offset = offset;
    return (offset - fat_region_offset)/4;
}

uint32_t convert_clus_num_to_offset_in_fat_region(uint32_t clus_num) {
    uint32_t fat_region_offset = bpb.BPB_BytsPerSec * bpb.BPB_RsvdSecCnt;
    return fat_region_offset + clus_num * 4;
}

//**********************************************************************************************//

uint32_t get_first_data_sector() {
    return bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);
}

uint32_t get_first_sect_of_clus(uint32_t cluster_number) {
    return (((cluster_number - 2) * bpb.BPB_SecPerClus) + get_first_data_sector()) 
        * bpb.BPB_BytsPerSec;
}


//**********************************************************************************************//

void lsCommand(const char *img_path){

    //calculates the data region address
    uint32_t dataRegionAddr = bpb.BPB_BytsPerSec * 
        (bpb.BPB_RsvdSecCnt + (bpb.BPB_FATSz32 * bpb.BPB_NumFATs));

    //calculates the computer root cluster
    uint32_t computerRootClusterAddress = dataRegionAddr + (curr_root - 2) * 
        (bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec);

    //cluster numbers
    uint32_t max_clus_num = bpb.BPB_FATSz32 / bpb.BPB_SecPerClus;
    uint32_t curr_clus_num = bpb.BPB_RootClus;
    uint32_t min_clus_num = 2;
    uint32_t next_clus_num = 0;

    // Open fat32.img
    strcpy(img_name, img_path);
    int fd = open(img_path, O_RDONLY);
     if (fd < 0) {
        perror("open fat32.img failed");
    }

    
    while (curr_clus_num >= min_clus_num && curr_clus_num <= max_clus_num) {
    
        uint8_t sector[512];
        uint32_t offset = convert_clus_num_to_offset_in_fat_region(curr_clus_num);
        pread(fd, sector, 512, computerRootClusterAddress);
        pread(fd, &next_clus_num, sizeof(uint32_t), offset);
        for(int i = 0; i < bpb.BPB_BytsPerSec; i += sizeof(entry_t)) {
            entry_t *entry = (entry_t *)&sector[i];
            
            if(entry->DIR_Name[0] == 0x00){
                break;
            }

            if(entry->DIR_Name[0] != 0xE5 && entry->DIR_Attr != 0x0F){
                char name[12];
                memcpy(name, entry->DIR_Name, 11);
                name[11] = '\0';
                printf("%s\n", name);
            } 
        }
        curr_clus_num = next_clus_num;
    }
    //close file
    close(fd);
}

void rtrim(char *str) {
    int n = strlen(str);
    while (n > 0 && isspace((unsigned char)str[n - 1])) {
        n--;
    }
    str[n] = '\0';
}

void to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

void changeDirectory(const char *dirName, const char *img_path) {
    //calculates the data region address
    uint32_t dataRegionAddr = bpb.BPB_BytsPerSec * 
        (bpb.BPB_RsvdSecCnt + (bpb.BPB_FATSz32 * bpb.BPB_NumFATs));

    //calculates the computer root cluster
    uint32_t computerRootClusterAddress = dataRegionAddr + (curr_root - 2) * 
        (bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec);

    //cluster numbers
    uint32_t max_clus_num = bpb.BPB_FATSz32 / bpb.BPB_SecPerClus;
    uint32_t curr_clus_num = curr_root;
    uint32_t min_clus_num = 2;
    uint32_t next_clus_num = 0;

    // Open fat32.img
    strcpy(img_name, img_path);
    int fd = open(img_path, O_RDONLY);
     if (fd < 0) {
        perror("open fat32.img failed");
    }
    
    bool isFound = false;
    while (curr_clus_num >= min_clus_num && curr_clus_num <= max_clus_num) {
        uint8_t sector[512];
        uint32_t offset = convert_clus_num_to_offset_in_fat_region(curr_clus_num);
        pread(fd, sector, 512, computerRootClusterAddress);
        pread(fd, &next_clus_num, sizeof(uint32_t), offset);
        char directoryName[11];
        strcpy(directoryName, dirName);

        for (int i = 0; i < bpb.BPB_BytsPerSec; i += sizeof(entry_t)) {
            entry_t *entry = (entry_t *)&sector[i];
            
            char formattedName[12];  // Buffer to store a null-terminated version of DIR_Name
            strncpy(formattedName, entry->DIR_Name, 11);  // Copy the first 11 characters
            formattedName[11] = '\0';  // Null-terminate the string
            rtrim(formattedName);  // Remove trailing spaces
            to_upper(formattedName);  // Convert to upper case if necessary

            char directoryNameUpper[12];  
            strncpy(directoryNameUpper, directoryName, 11);
            directoryNameUpper[11] = '\0';
            rtrim(directoryNameUpper);  // Trim right spaces
            to_upper(directoryNameUpper);  // Convert to upper case to match formattedName
            
            if (strcmp(directoryNameUpper, ".") == 0){
                break;
            } if(strcmp(directoryNameUpper, "..") == 0){
                if(curr_root != bpb.BPB_RootClus){
                    char *lastSlash = strrchr(current_path, '/');
                    if (lastSlash != current_path) {
                        *lastSlash = '\0';  // Cut the path at the last slash
                    } else {
                        *(lastSlash + 1) = '\0';  // Only root remains
                    }

                depth--;
                curr_root = previousCurrRoot[depth];
                previousCurrRoot[depth + 1] = 0;
                break;
                }
            } else if (entry->DIR_Attr == 0x10) {  // Check if it is a directory
                if (strcmp(formattedName, directoryNameUpper) == 0) {  // Compare cleaned names
                    previousCurrRoot[depth] = curr_root;
                    curr_root = ((uint32_t)entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;     
                    depth++;

                    if(previousCurrRoot[depth - 1] != bpb.BPB_RootClus){
                        strcat(current_path, "/");
                    }

                    strcat(current_path, directoryName);  
                    isFound = true;
                    break;
                } 
            } else if(entry->DIR_Attr != 0x0F && entry->DIR_Attr != 0x02 && 
                entry->DIR_Attr != 0x04 && entry->DIR_Name[0] == 0xE5){
               printf("'%s' is not a directory\n", entry->DIR_Name);
            }
        }

        if(isFound){
            break;
        }
        
        curr_clus_num = next_clus_num;
    }
    //close file
    close(fd);     
 } 

//**********************************************************************************************//

void openFile(char* filename, char* mode, char* cwd, openedfile_node** headRef, 
    uint32_t curr_cluster, int fd_for_img) {
    if (filesOpened >= 10) {
        printf("Maximum file open limit reached.\n");
        return;
    }

    char fat_filename[12];
    snprintf(fat_filename, sizeof(fat_filename), "%-11s", filename);
    for (int i = 0; i < 11; i++) {
        fat_filename[i] = toupper(fat_filename[i]);
    }

    entry_t entry;
    bool file_found = false;
    int i = 0;
    uint32_t pos;
    while (i * sizeof(entry) < bpb.BPB_BytsPerSec) {
        pos = get_first_sect_of_clus(curr_cluster) + i * sizeof(entry);
        pread(fd_for_img, &entry, sizeof(entry), pos);

        if (entry.DIR_Name[0] == 0x00) break;  // End of directory entries
        if (entry.DIR_Name[0] == 0xE5) continue;  // Skip deleted entries

        if (strncmp(entry.DIR_Name, fat_filename, 11) == 0) {
            file_found = true;
            break;
        }
        i++;
    }

    if (!file_found) {
        printf("File '%s' does not exist in the current directory.\n", filename);
        return;
    }

    openedfile_node* new_node = (openedfile_node*)malloc(sizeof(openedfile_node));
    if (new_node == NULL) {
        printf("Memory allocation failed for the new file node.\n");
        return;
    }

    new_node->file.filename = strdup(fat_filename);
    new_node->file.flag = strdup(mode);
    new_node->file.cwd = strdup(cwd);
    new_node->file.cluster_offset = get_first_sect_of_clus(entry.DIR_FstClusLO);
    new_node->file.file_size = entry.DIR_FileSize;
    new_node->file.offset = 0;
    new_node->next = *headRef;
    *headRef = new_node;  // Update the global head to the new node

    filesOpened++;
    printf("File '%s' opened with mode '%s'.\n", filename, mode);
}


int closeFile(char* filename, openedfile_node** head) {
    char fat_filename[12];
    snprintf(fat_filename, sizeof(fat_filename), "%-11s", filename);
    for (int i = 0; i < 11; i++) {
        fat_filename[i] = toupper(fat_filename[i]);
    }

    openedfile_node* current = *head;
    openedfile_node* previous = NULL;

    while (current != NULL) {
        if (strncmp(current->file.filename, fat_filename, 11) == 0) {
            if (previous == NULL) {
                *head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current->file.filename);
            free(current->file.flag);
            free(current->file.cwd);
            free(current);
            filesOpened--;
            printf("File '%s' closed successfully.\n", filename);
            return 1;
        }
        previous = current;
        current = current->next;
    }
    printf("Error: File '%s' not found or not opened.\n", filename);
    return -1;
}


void lseekFile(const char* filename, int offset, openedfile_node** head) {
    char fat_filename[12];
    snprintf(fat_filename, sizeof(fat_filename), "%-11s", filename);
    for (int i = 0; i < 11; i++) {
        fat_filename[i] = toupper(fat_filename[i]);
    }

    openedfile_node* current = *head;
    while (current != NULL) {
        if (strncmp(current->file.filename, fat_filename, 11) == 0) {
            if (offset < 0 || offset > current->file.file_size) {
                printf("Error: Offset (%d) is out of bounds. File size is %d.\n", offset, 
                    current->file.file_size);
                return;
            }
            current->file.offset = offset;
            printf("Offset set to %d for file '%s'.\n", offset, filename);
            return;
        }
        current = current->next;
    }
    printf("Error: File '%s' not found or not opened.\n", filename);
}

void readFile(char* filename, int size, openedfile_node** head, int fd_for_img) {
    char fat_filename[12];
    snprintf(fat_filename, sizeof(fat_filename), "%-11s", filename);
    for (int i = 0; i < 11; i++) {
        fat_filename[i] = toupper(fat_filename[i]);
    }

    openedfile_node* current = *head;
    while (current != NULL) {
        if (strncmp(current->file.filename, fat_filename, 11) == 0) {
            if (strstr(current->file.flag, "r") == NULL) {
                printf("Error: File '%s' is not opened for reading.\n", filename);
                return;
            }

            if (current->file.offset >= current->file.file_size) {
                printf("Error: Current offset is at or beyond the end of file.");
                printf(" Reset offset or reopen file.\n");
                return; // Early exit if offset is at or beyond the end
            }

            uint32_t remaining = current->file.file_size - current->file.offset;
            uint32_t effective_size = (size > remaining) ? remaining : size;

            char* buffer = (char*)malloc(effective_size + 1);
            if (!buffer) {
                printf("Error: Memory allocation failed.\n");
                return;
            }

            lseek(fd_for_img, current->file.cluster_offset + current->file.offset, SEEK_SET);
            int bytes_read = read(fd_for_img, buffer, effective_size);
            buffer[bytes_read] = '\0';  // Null-terminate the buffer

            printf("Read from file '%s': %s\n", filename, buffer);

            current->file.offset += bytes_read;  // Update the file offset
            free(buffer);
            return;
        }
        current = current->next;
    }
    printf("Error: File '%s' not found or not opened.\n", filename);
}

void writeFile(char* filename, char* data, openedfile_node** head, int fd_for_img) {
    char fat_filename[12];
    snprintf(fat_filename, sizeof(fat_filename), "%-11s", filename);
    for (int i = 0; i < 11; i++) {
        fat_filename[i] = toupper(fat_filename[i]);
    }

    fprintf(stderr, "%d", fd_for_img);
    char tmp[10];
    pread(fd_for_img, tmp, 10, 0);
    fprintf(stderr, "read [%s] from %d\n", tmp, fd_for_img);

    openedfile_node* current = *head;
    while (current != NULL) {
        if (strncmp(current->file.filename, fat_filename, 11) == 0) {
            if (strstr(current->file.flag, "w") == NULL && strstr(current->file.flag, "rw")
                 == NULL && strstr(current->file.flag, "wr") == NULL) {
                printf("Error: File '%s' is not opened for writing.\n", filename);
                return;
            }

            // Validate file descriptor
            if (fd_for_img < 0) {
                fprintf(stderr, "%d", fd_for_img);
                printf("Error: Bad file descriptor.\n");
                return;
            }

            uint32_t start_pos = current->file.cluster_offset + current->file.offset;
            uint32_t data_len = strlen(data);

            if (current->file.offset + data_len > current->file.file_size) {
                // Optionally expand the file here if that behavior is desired
                current->file.file_size = current->file.offset + data_len;
            }

            lseek(fd_for_img, start_pos, SEEK_SET);
            fprintf(stderr, "lseek fd: %d pos: %d\n", fd_for_img, start_pos);

            int written_bytes = write(fd_for_img, data, data_len);
            fprintf(stderr, "write %d size data [%s] to fd %d\n", data_len, data, fd_for_img);
            if (written_bytes < 0) {
                perror("Failed to write data");
                return;
            }

            current->file.offset += written_bytes;
            printf("Written to file '%s': %s\n", filename, data);
            return;
        }
        current = current->next;
    }
    printf("Error: File '%s' not found or not opened for writing.\n", filename);
}
