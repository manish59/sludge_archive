#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>

#include "sludge.h"

#define DBGOUT(msg)	do{FILE* log = fopen("/tmp/sludge.log", "a");fwrite((void *)msg, strlen(msg), 1, log);fclose(log);}while(0)
//#define DBGOUT(msg)	{}
#define ERROR(msg)	do{char msgbuf[PATH_MAX]; sprintf(msgbuf, "ERROR %s:%s\n", msg, strerror(errno)); DBGOUT(msgbuf);}while(0)
//#define ERROR(msg)  {}


const char *switches[] = {"-a", "-c", "-e", "-d", "-l", "-r", "-h"};
const char *helpstring[] = {"Append File(s)", "Create Archive", "Extract File(s)", "Delete File(s)", "List Files", "Read File(s)", "Howto Use"};

#define SLUGE_APPEND	0
#define SLUGE_CREATE	1
#define SLUGE_EXTRACT	2
#define SLUGE_DELETE	3
#define SLUGE_LIST	4
#define SLUGE_READ	5
#define SLUGE_HELP	6

#define SLUGE_FCNT	7

// detect function switch and return its ID
int get_function(char * sw)
{
    if (!sw) return SLUGE_HELP;
    int i;
    for( i = 0; i < SLUGE_FCNT; i++)
    {
	if (strcmp(switches[i], sw) == 0) return i;
    }
    return SLUGE_HELP;
}

// print how to use
void help_message(char * name)
{
    printf("USAGE: %s [function_switch] [archive_name] [file_name]\n", name);
    printf("\t Available command switches \n");
    int i;
    for (i = 0; i < SLUGE_FCNT; i++)
    {
	printf("\t%s:%s\n", switches[i], helpstring[i]);
    }
}

// get file size
int get_filesize(char * file_name)
{
    int size = 0;
    FILE *file;
    file = fopen(file_name, "r");
    if (!file) 
    {
	ERROR("get file size failed");
	exit(0);
    }
    else
    {
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fclose(file);
    }
    return size;
}

// initialize filesystem header buffer
FSHDR *init_fshdr()
{
    FSHDR *fshdr = (FSHDR *)calloc(1, FSHEADERSZ);
    if (!fshdr)
    {
        ERROR("File system header creation failed");
        exit(0);
    }
    return fshdr;
}

// load file system header from archive file
int load_fshdr(char * arch_name, FSHDR * fshdr)
{
    FILE *arch_file = fopen(arch_name, "rb");
    
    if (!arch_file)
    {
	ERROR("Archive file open failed");
	exit(0);
    }
    else
    {
	fread((void*)fshdr, FSHEADERSZ, 1, arch_file);
	fclose(arch_file);
    }
    
    return fshdr->total_file_count;
}

// creat a new archieve file with file lists
void create(char * arch_name, char *file_lists[], int count)
{
    printf("create %s\n", arch_name);

    FILE *archive_file = fopen(arch_name, "wb");
    if (!archive_file)
    {
	ERROR("Archive File Creation failed");
	exit(0);
    }
    else
    {
	// create file system header
	FSHDR *fshdr = init_fshdr();
	fwrite((void *)fshdr, FSHEADERSZ, 1, archive_file);
	fclose(archive_file);
	free(fshdr);
    }
    int i;
    for(i = 0; i < count; i++) append(arch_name, file_lists[i]);
}

// extract file(s) from the archive
void extract(char *arch_name, char * file_name)
{
    if (file_name) printf("extract %s from %s\n", file_name, arch_name);
    else printf("extract all from %s\n", arch_name);
    
    // load file system header
    FSHDR * fshdr = init_fshdr();
    int file_count = load_fshdr(arch_name, fshdr);
    
    if (file_count == 0) // empty archive file
    {
	DBGOUT(arch_name);
	DBGOUT(" is initialized\n");
    }
    else
    {
	// archive file open and goto data offset
	FILE * arch_file = fopen(arch_name, "rb"); // here we expect arch file will be opened
	int i;
	for ( i = 0; i < file_count; i++)
	{
	    // search and extract file(s)
	    char * cur_file_name = fshdr->file_name[i];
	    if (strlen(cur_file_name) == 0) continue; // deleted file

	    int file_size = fshdr->file_size[i];
	    int file_offset = fshdr->file_offset[i];
	
	    int individual = 0; 
	    if (file_name )	// individual file extraction
	    {
		if (strcmp(file_name, cur_file_name) != 0) continue;
		else individual = 1;
	    }
	    // file extraction
	    FILE *extract_file = fopen(cur_file_name, "wb");
	
	    if (!extract_file)	// file open failed
	    {
		DBGOUT(cur_file_name);
		DBGOUT(" cannot be created\n");
		continue;
	    }
	    else if (file_size != 0) // if file is not emptied
	    {
		fseek(arch_file, FSHEADERSZ + file_offset, SEEK_SET);
		int j;
		for ( j = 0; j < file_size; j++)
	        {
		    char ch;
		    fread((void *)&ch, 1, 1, arch_file);
		    fwrite((void *)&ch, 1, 1, extract_file);
		}
	    }
	    fclose(extract_file);
	    if (individual) break;
	}
	fclose(arch_file);
    }
    free(fshdr);
}

// delete file(s) from the archive
void delete(char *arch_name, char *file_name)
{
    if (file_name) printf("delete file %s from %s\n", file_name, arch_name);
    else printf("delete all from %s\n", arch_name);

    int total_file_size = get_filesize(arch_name);
    
    if (total_file_size == 0) // empty file, invalid file
    {
	DBGOUT(arch_name);
	DBGOUT(" is zero byte, please select another archive file\n");
	return;
    }
    
    // allocate buffer
    void *arch_filebuf = malloc(total_file_size);
    if (!arch_filebuf)
    {
	ERROR("memory lacks");
	exit(0);
    }
    
    // load file contents
    FILE * arch_file = fopen(arch_name, "rb");
    fread(arch_filebuf, total_file_size, 1, arch_file);
    fclose(arch_file);
    
    // load file system header
    FSHDR * fshdr = (FSHDR*)arch_filebuf;
    int file_count = fshdr->total_file_count;

    if (file_count == 0) 	// empty archive file
    {
	DBGOUT(arch_name);
	DBGOUT(" is already emptied.\n");
    }
    else
    {    
        for (int i = 0; i < file_count; i++)
	{
	    char *cur_file_name = fshdr->file_name[i];
	    if (strlen(cur_file_name) == 0) continue; // skip over deleted file
	    
	    int individual = 0;
	    if (file_name)		// individual deleting
	    {
		if (strcmp(fshdr->file_name[i], file_name) != 0) continue;
		else individual =1;
	    }
	    //logical file deletion, only clears file name field
	    fshdr->real_file_count--;
	    memset(cur_file_name, 0, MAX_FILE_NAME);
	    if (individual == 1) break;
	}
    }
    // rewrite filesystem header
    arch_file = fopen(arch_name, "wb");
    fwrite(arch_filebuf, total_file_size, 1, arch_file);
    fclose(arch_file);
    free(arch_filebuf);
}

// read file(s) from the archive
void readfile(char *arch_name, char * file_name)
{
    if (file_name) printf("read %s from %s\n", file_name, arch_name);
    else printf("read all from %s\n", arch_name);
    
    // load file system header
    FSHDR * fshdr = init_fshdr();
    int file_count = load_fshdr(arch_name, fshdr);
    
    if (file_count == 0)// empty archive file
    {
	DBGOUT(arch_name);
	DBGOUT(" is initialized\n");
    }
    else
    {
	FILE * arch_file = fopen(arch_name, "rb");
	for (int i = 0; i < file_count; i++)
	{
	    char * cur_file_name = fshdr->file_name[i];
	    if (strlen(cur_file_name) == 0) continue; // skip over deleted file
	    
	    int file_size = fshdr->file_size[i];
	    int file_offset = fshdr->file_offset[i];

	    int individual = 0;	
	    if (file_name)		// individual file read
	    {
		if (strcmp(file_name, cur_file_name) != 0 ) continue;
		else individual = 1;
	    }
	
	    if (file_size > 0) 	// if file is not empty
	    {
		fseek(arch_file, FSHEADERSZ + file_offset, SEEK_SET);
		// dump the file contents
		for (int j = 0; j < file_size; j++)
		{
		    char ch;
		    fread((void *)&ch, 1, 1, arch_file);
		    printf("%c", ch);
		}
		printf("\n");
	    }
	    else DBGOUT("file is emptied\n");
	    if (individual == 1) break;
	}
	fclose(arch_file);
    }
    free(fshdr);
}

// list file informations from the archive
void listfiles(char *arch_name)
{
    printf("List files in %s\n", arch_name);
    // load file system header
    FSHDR * fshdr = init_fshdr();
    int file_count = load_fshdr(arch_name, fshdr);
    
    if (file_count == 0) // empty archive file
    {
	DBGOUT(arch_name);
	DBGOUT(" is emptied\n");
    }
    else
    {
	for (int i = 0; i < file_count; i++)
	{
	    char * cur_file_name = fshdr->file_name[i];
	    int file_size = fshdr->file_size[i];
	    int file_offset = fshdr->file_offset[i];
	
	    if (strlen(cur_file_name) > 0) printf("%d\t%s\t%dB\t0x%x\n", i + 1, cur_file_name, file_size, file_offset);
	    else printf("%d\tdeleted\t%dB\t0x%x\n", i + 1, file_size, file_offset);
	}
    }
    free(fshdr);
}

// append file into the archive
void append(char *arch_name, char *file_name)
{
    if (!file_name)
    {
	DBGOUT("You have to input valid file name, Try again!\n");
	exit(0);
    }
    printf("Append to %s with %s\n", arch_name, file_name);

    int total_file_size = get_filesize(arch_name);
    if (total_file_size == 0) // empty file, invalid file
    {
	DBGOUT(arch_name);
	DBGOUT(" is not initialized, please recreate it\n");
	return;
    }
    
    // allocate buffer
    void *arch_filebuf = malloc(total_file_size);
    if (!arch_filebuf)
    {
	ERROR("memory lacks");
	exit(0);
    }
    
    // load file contents
    FILE * arch_file = fopen(arch_name, "rb");
    fread(arch_filebuf, total_file_size, 1, arch_file);
    fclose(arch_file);
    
    // load file system header
    FSHDR * fshdr = (FSHDR*)arch_filebuf;
    int file_count = fshdr->total_file_count;

    if (file_count >= MAX_FILE_COUNT)
    {
	DBGOUT("File system is full, delete all the files\n");
	exit(0);
    }
    
    // get last file size and offset
    int file_size = 0;
    if (file_count > 0) file_size = fshdr->file_size[file_count - 1];
    int file_offset = 0;
    if (file_count > 0) file_offset = fshdr->file_offset[file_count -1];
    
    // get current file size and offset
    int cur_file_size = get_filesize(file_name);
    int cur_file_offset = file_size + file_offset;

    printf("cur_fs=%d cur_off=%d last_fs=%d last_off=%d\n", cur_file_size, cur_file_offset, file_size, file_offset);
    
    // register new file
    sprintf(fshdr->file_name[file_count], "%s", file_name);
    fshdr->file_size[file_count] = cur_file_size;
    fshdr->file_offset[file_count] = cur_file_offset;
    fshdr->total_file_count++;
    fshdr->real_file_count++;
    
    // write new header and original data
    arch_file = fopen(arch_name, "wb");
    fwrite((void*)arch_filebuf, total_file_size, 1, arch_file);
    // append a new file data
    if (cur_file_size > 0)
    {
	FILE * data_file = fopen(file_name, "rb");
	for (int j = 0; j < cur_file_size; j++)
	{
	    char ch;
	    fread((void *)&ch, 1, 1, data_file);
	    fwrite((void *)&ch, 1, 1, arch_file);
	}
	fclose(data_file);
    }
    fclose(arch_file);
    free(arch_filebuf);
}

// parse command line and execute major functions
int main( int argc, char *argv[])
{
    switch( get_function(argv[1]))
    {
	case SLUGE_APPEND:
	    append(argv[2], argv[3]);
	break;
	case SLUGE_CREATE:
	    create(argv[2], argv + 3, argc - 3);
	break;
	case SLUGE_EXTRACT:
	    extract(argv[2], argv[3]);
	break;
	case SLUGE_DELETE:
	    delete(argv[2], argv[3]);
	break;
	case SLUGE_LIST:
	    listfiles(argv[2]);
	break;
	case SLUGE_READ:
	    readfile(argv[2], argv[3]);
	break;
	default:
	    help_message(argv[0]);
	break;
    }
}
