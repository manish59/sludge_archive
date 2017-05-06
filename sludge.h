#define MAX_FILE_COUNT	100
#define MAX_FILE_NAME	20

typedef struct fsheader
{
    int total_file_count;				// total file count in archive file
    int real_file_count;				// real file count that not deleted
    char file_name[MAX_FILE_COUNT][MAX_FILE_NAME];	// file name list table
    int file_offset[MAX_FILE_COUNT];			// file data offset table 
    int file_size[MAX_FILE_COUNT];			// file size table
}FSHDR;

#define FSHEADERSZ	sizeof(FSHDR)
