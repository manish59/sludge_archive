/*
FUSE_Client : myFUSE
*/
#define FUSE_USE_VERSION 26
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fuse.h>
#include "sludge.h"

#define DBGOUT(msg)	do{FILE* log = fopen("/tmp/fuse.log", "a");fwrite((void *)msg, strlen(msg), 1, log);fclose(log);}while(0)
//#define DBGOUT(msg)	{}
#define ERROR(msg)	do{char msgbuf[PATH_MAX]; sprintf(msgbuf, "ERROR %s:%s\n", msg, strerror(errno)); DBGOUT(msgbuf);}while(0)
//#define ERROR(msg)  {}

static const char *arch_name = NULL;
FSHDR		  *fsheader = NULL;

/* get file size with identify its validity */
int get_file_size(const char * file_name)
{
	int i;
//	if (strcmp(file_name, "/") == 0) return 0;		// directory
	for (i = 0; i < fsheader->total_file_count; i++)
	{
		char *cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue; // skip deleted file
		if (strcmp(cur_file_name, file_name + 1) == 0) return fsheader->file_size[i];
	}
//	ERROR(":get_file_size");
	return -1;	// invalid file name
}

/* load file system header */
void* my_init(struct fuse_conn_info *conn)
{
	DBGOUT("Initializing...\n");
	printf("Initializing .....\n");
	fsheader = (FSHDR*)calloc(1, FSHEADERSZ);

	FILE *arch_file = fopen(arch_name, "rb");

	if (!arch_file)
	{
		ERROR("Archive file open failed");
		exit(0);
	}
	else
	{
		fread((void*)fsheader, FSHEADERSZ, 1, arch_file);
		DBGOUT(fsheader);
		fclose(arch_file);
	}
}

/* free file system buffer */
void my_destroy(void* private_data)
{
	free(fsheader);
}

/* Get file attributes */
int my_getattr(const char *path, struct stat *stbuf)
{
	DBGOUT(path); DBGOUT(":getattr()\n");
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_uid = geteuid();
	stbuf->st_gid = getegid();
	if (strcmp(path, "/") == 0) 
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else 
	{
		int res = get_file_size(path);
		if (res == -1) return -ENOENT;
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = res;
	}
	return 0;
}

/* Create a file node */
int my_mknod(const char *path, mode_t mode, dev_t dev)
{
	return -EPERM;
}

/* Create a directory */
int my_mkdir(const char *path, mode_t mode)
{
	return -EPERM;
}

/* Read the target of a symbolic link */
int my_readlink(const char *path, char *link, size_t size)
{
	link = strdup(path);
	size = get_file_size(path);
	return 0;
}

/* Create a symbol-link */
int my_symlink(const char *path, const char *link)
{
	return -EPERM;
}

/* Create a hard link to a file */
int my_link(const char *path, const char *newpath)
{
	return -EPERM;
}

/* Remove a file */
int my_unlink(const char *path)
{	int i;
	for ( i = 0; i < fsheader->total_file_count; i++)
	{
		char *cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue;		//skip deleted file
		
		if (strcmp(path + 1, cur_file_name) == 0)
		{
			fsheader->real_file_count--;
			memset(fsheader->file_name[i], 0, MAX_FILE_NAME);
			
			struct stat st;
			stat(arch_name, &st);
			int file_size = st.st_size;
			void* filebuf = malloc(file_size);
			FILE *arch_file = fopen(arch_name, "rb");
			fread(filebuf, file_size, 1, arch_file);
			fclose(arch_file);
			
			memcpy(filebuf, fsheader, FSHEADERSZ);
			arch_file = fopen(arch_name, "wb");
			fwrite(filebuf, file_size, 1, arch_file);
			fclose(arch_file);
			
			free(filebuf);
			return 0;
		}
	}
	return -ENOENT;
}

/* Remove a directory */
int my_rmdir(const char *path)
{
	return -EPERM;
}

/* Rename a file */
int my_rename(const char *path, const char *newpath)
{
	return -EPERM;
}

/* Change the permission of a file */
int my_chmod(const char *path, mode_t mode)
{
	return 0;
}

/* Change the owner of a file */
int my_chown(const char *path, uid_t uid, gid_t gid)
{
	return 0;
}

/* File open operation */
int my_open(const char *path, struct fuse_file_info *fi)
{
	DBGOUT(path); DBGOUT(":open\n");
/*	
	for(int i = 0; i < fsheader->total_file_count; i++)
	{
		char * cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue;
		
		if (strcmp(path - 1, cur_file_name) == 0) return 0;
	}
	return -ENOENT;
*/
	return 0;
}

/* Read data from an open file */
int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	char msg[PATH_MAX];
	sprintf(msg, "read:%s %dB from 0x%x\n", path, size, offset);
	DBGOUT(msg);
	
	(void) fi;
	int i;
	for (i = 0; i < fsheader->total_file_count; i++)
	{
		char * cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue;	// skip deleted file

		if (strcmp(path + 1, cur_file_name) == 0)
		{
			int file_size = fsheader->file_size[i];
			int file_offset = fsheader->file_offset[i];
			if (size > file_size) size = file_size;
			if ((size + offset) > file_size) return -EACCES;
			FILE * arch_file = fopen(arch_name, "rb");
			fseek(arch_file, FSHEADERSZ + file_offset + offset, SEEK_SET);
			fread((void *)buf, size, 1, arch_file);
			fclose(arch_file);
			return size;
		}
	}

	return -EACCES;
}

/* Write data to an open file */
int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	char msg[PATH_MAX];
	sprintf(msg, "write:%s %dB from 0x%x\n", path, size, offset);
	DBGOUT(msg);

	// find same file name and delete it first
	int total_file_count = fsheader->total_file_count;
	if (total_file_count == MAX_FILE_COUNT) return -EACCES;	// archive file is full
	int i;
	for(i = 0; i < total_file_count - 1; i++)
	{
		char * cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue;
		if (strcmp(path + 1, cur_file_name) == 0)
		{
			memset(cur_file_name, 0, MAX_FILE_NAME);
			break;
		}
	}
	
	if (strcmp(path + 1, fsheader->file_name[total_file_count - 1]) == 0) // last appended empty file
	{
		fsheader->file_size[total_file_count - 1] = size;
		fsheader->file_offset[total_file_count - 1] = fsheader->file_size[total_file_count - 2] + fsheader->file_offset[total_file_count - 2];
	}
	else	// append a new file item
	{
		sprintf(fsheader->file_name[total_file_count],"%s", path + 1);
		fsheader->file_size[total_file_count] = size;
		fsheader->file_offset[total_file_count] = fsheader->file_size[total_file_count - 1] + fsheader->file_offset[total_file_count - 1];	
		fsheader->real_file_count++;
		fsheader->total_file_count++;
	}
			
	struct stat st;
	stat(arch_name, &st);
	int file_size = st.st_size;
	void* filebuf = calloc(1, file_size + size);

	FILE *arch_file = fopen(arch_name, "rb");
	fread(filebuf, file_size, 1, arch_file);
	fclose(arch_file);
			
	memcpy(filebuf, fsheader, FSHEADERSZ);
	if (size > 0) memcpy(filebuf + file_size, buf, size);
	
	arch_file = fopen(arch_name, "wb");
	fwrite(filebuf, file_size + size, 1, arch_file);
	fclose(arch_file);
	
	free(filebuf);
	return size;
}

/* Release an open file */
int my_release(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/* Get file system statistics */
int my_statfs(const char *path, struct statvfs *buff)
{
	return statvfs(get_current_dir_name(), buff);
}

/* Open directory */
int my_opendir(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/* Release directory */
int my_releasedir(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/* Read directory */
int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	(void)offset;
	(void)fi;

	if (*path == '\0') return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	int i;
	for (i = 0; i < fsheader->total_file_count; i++)
	{
		char *cur_file_name = fsheader->file_name[i];
		if (strlen(cur_file_name) == 0) continue;	// skip deleted file
		filler(buf, cur_file_name, NULL, 0);
	}

	return 0;
}

/* update time */
int my_utime(const char *path, struct utimbuf *ubuf)
{
	return 0;
}

/* Check file permissions */
int my_access(const char *path, int mask)
{
	return 0;
}

/* Change the size of a file */
int my_truncate(const char *path, off_t newsize)
{
	return 0;
}

/* Get attributes from an open file */
int my_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *fi)
{
	return my_getattr(path, buf);
}

/* flush cached data */
int my_flush(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/* Synchronize file contents */
int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	return 0;
}

/* Synchronize directory contents */
int my_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
	return 0;
}

/* Change the size of an open file */
int my_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	return 0;
}

/* Create and open a file */
int my_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	DBGOUT(path); DBGOUT(":create\n");
	// create empty file
	my_write(path, NULL, 0, 0, NULL);
	return 0;
}

static struct fuse_operations my_operations = {
	.init = my_init,
	.destroy = my_destroy,
	.getattr = my_getattr,
	.unlink = my_unlink,
	.chmod = my_chmod,
	.chown = my_chown,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.readdir = my_readdir,
	.create = my_create,
	.fgetattr = my_fgetattr,

};

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("USAGE: %s srcdir dstdir\n", argv[0]);
		printf("mount archive file to dstdir\n");
		exit(0);
	}

	arch_name = realpath(argv[1], NULL);
	printf("Mounting %s to %s\n", argv[1], argv[2]);

	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	return fuse_main(argc, argv, &my_operations, NULL);
}
