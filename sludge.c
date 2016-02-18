#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int file_count=0;
int size_of_file(char * file_name) {
	int size;
	FILE *fh;
	fh=fopen(file_name,"r");
	fseek(fh, 0, SEEK_END);
	size = ftell(fh);	
	fclose(fh);
	return  size;
}
void create(char  * name,char * argv[],int count)
{
	//name=archive name
	FILE *file_in,*file_out;
	char fname;
	int length=0;
	int i=0;
	int j=3;//j variable is the index where the file names starts in the array argv which are given to archive
	length=size_of_file(argv[j]);
	file_out=fopen(name,"ab+");
	fwrite(&count,sizeof(int),1,file_out);
	fclose(file_out);
	for(i=0;i<count;i++)
		{
		length=size_of_file(argv[j]);
		file_in=fopen(argv[j],"rb");
                file_out=fopen(name,"ab+");
		if(!file_in)
		{
			printf("No file found\n");
			exit(2);
		}
		char *buffer;
		buffer= (char *)malloc(sizeof(char)*length);
		//printf("Address=%d",count);
		file_count++;
		//fname=*argv[j];
		fwrite(argv[j],sizeof(char),255,file_out);//writing name of the file 
		printf("File name :%s\n",argv[j]);
		fwrite(&length,sizeof(int),1,file_out);//Size of the file
		fread(buffer,sizeof(char),length,file_in);
		fwrite(buffer,sizeof(char),length,file_out);
		printf("%s has been written with %d memory\n",argv[j],length);
		j++;fclose(file_in);fclose(file_out);
		free(buffer);
		}
}
void extract(char *name)
{	

	FILE *file_in,*file_out;
	int a,position;
	file_in=fopen(name,"rb");
	fread(&a,sizeof(int),1,file_in);
	printf("File count=%d\n",a);
	int i=0;
	/*if(a<=0)
	{printf("no files are there for extraction\n");
	exit(2);}*/
	for(i;i<a;i++){
	char name_of_file[255];
	int size;
	fread(name_of_file,sizeof(char),255,file_in);
	fread(&size,sizeof(int),1,file_in);
	printf("Name of the file %s\t",name_of_file);
	printf("size of the file :%d\n",size);
	char buffer[size];
	fread(buffer,sizeof(char),size,file_in);
	file_out=fopen(name_of_file,"ab+");
	fwrite(buffer,sizeof(char),size,file_out);
	}
	//printf("files are extracted\n");
	fclose(file_in);
	fclose(file_out);
	
}
void remv(char *archive,char *fname)
{
	FILE *file_in,*file_out,*file_inin;
	int a=0,i=0,count;
	file_in=fopen(archive,"rb");
	fread(&a,sizeof(int),1,file_in);
       // printf("File count=%d\n",a);
	file_out=fopen("temp","wb+");
	fwrite(&a,sizeof(int),1,file_out);
	count=a;
        for(i;i<a;i++){
	char name_of_file[255];
	int size;
	fread(name_of_file,sizeof(char),255,file_in);
	//printf("name::%s\n",name_of_file);
	fread(&size,sizeof(int),1,file_in);
	//printf("size::%d\n",size);
	char *buffer;
	buffer= (char *)malloc(sizeof(char)*size);
	if(strcmp(name_of_file,fname)==0)
	{
	//fseek(file_in,59,SEEK_SET);
	printf("%s has been removed \n",name_of_file);
	fread(buffer,sizeof(char),size,file_in);
	free(buffer);
	count--;
	}
	else
	{
	fwrite(name_of_file,sizeof(char),255,file_out);
	fwrite(&size,sizeof(int),1,file_out);
	fread(buffer,sizeof(char),size,file_in);
	fwrite(buffer,sizeof(char),size,file_out);
	free(buffer);
	}
	}
	fseek(file_out, 0, SEEK_SET);
 	fwrite(&count,sizeof(int),1,file_out);
        rename("temp",archive);
	fclose(file_out);	
	fclose(file_in);
}
int read_archive(char *archive)
{
	FILE *file_in;
	int a,position;
	file_in=fopen(archive,"rb");
	fread(&a,sizeof(int),1,file_in);
	printf("File count=%d\n",a);
	return a;
	fclose(file_in);
}

void list(char *archive)
{
	FILE *file_in;
	int a,position;
	file_in=fopen(archive,"rb");
	fread(&a,sizeof(int),1,file_in);
	printf("File count=%d\n",a);//here a stores file count
	int i=0;
	for(i;i<a;i++)
	{
	char name_of_file[255];
	int size;
	fread(name_of_file,sizeof(char),255,file_in);
	fread(&size,sizeof(int),1,file_in);
	printf("Name of the file %s\t",name_of_file);
	printf("size of the file :%d\n",size);
	char buffer[size];
	fread(buffer,sizeof(char),size,file_in);
	}
	fclose(file_in);
	
}
void add(char *archive,char *argv[],int argc)
{
	FILE *file_in,*file_out;
	int count,i,j=3;
	int length;
	count=read_archive(archive);
file_in=fopen(argv[j],"rb");
                file_out=fopen(archive,"ab+");
	for(i=0;i<argc-3;i++)
	{
		length=size_of_file(argv[j]);
		
		if(!file_in)
		{
			printf("No file found\n");
			exit(2);
		}
		char *buffer;
		buffer= (char *)malloc(sizeof(char)*length);
		//printf("Address=%d",count);
		count++;
		//fname=*argv[j];
		fwrite(argv[j],sizeof(char),255,file_out);//writing name of the file 
		printf("File name :%s\n",argv[j]);
		fwrite(&length,sizeof(int),1,file_out);//Size of the file
		fread(buffer,sizeof(char),length,file_in);
		fwrite(buffer,sizeof(char),length,file_out);
		printf("%s has been written with %d memory\n",argv[j],length);
		j++;
		free(buffer);
	}fclose(file_in);
	fclose(file_out);
		FILE *outfile;
		outfile=fopen(archive,"r+b");
		fseek(outfile, 0, SEEK_SET);
	 	fwrite(&count,sizeof(int),1,outfile);
		fclose(outfile);
		//fclose(file_in);
	printf("count %d\n",count);
}
void extract_individual(char *archive,char *argv[],int argc)
{	
	FILE *file_in,*file_out;
	int a,position;
	file_in=fopen(archive,"rb");
	fread(&a,sizeof(int),1,file_in);
	printf("File count=%d\n",a);
	int i=0,j=3;
	/*if(a<=0)
	{printf("no files are there for extraction\n");
	exit(2);}*/
	for(i;i<a;i++){
	char name_of_file[255];
	int size;
	fread(name_of_file,sizeof(char),255,file_in);
	fread(&size,sizeof(int),1,file_in);
	if(strcmp(name_of_file,argv[j])==0)
	{
	printf("True:\n");
	printf("name :%s\n",name_of_file);
	file_out=fopen(name_of_file,"ab+");
	char buffer[size];
	fread(buffer,sizeof(char),size,file_in);
	fwrite(buffer,sizeof(char),size,file_out);
	}
	else
	{
	char buffer[size];
	fread(buffer,sizeof(char),size,file_in);
	}
	//
	//
	}
	//printf("files are extracted\n");
	fclose(file_in);
	fclose(file_out);
}
int main( int argc, char *argv[] )  {
	if(argc<2)
	{
	printf("No enough arguments !");
	}
	else if (strcmp(argv[1],"-c")==0)//Create 
	{
	create(argv[2],argv,argc-3);
	}
	else if(strcmp(argv[1],"-e")==0) //Extract
	{// ./a.out -a archivename 
		if(argc==3)
		{
		extract(argv[2]);
		}
		else
		{ 
		extract_individual(argv[2],argv,argc);
		}
	}
	else if(strcmp(argv[1],"-l")==0)//list
	{
	list(argv[2]);
	}
	else if(strcmp(argv[1],"-r")==0)//remove
	{
		if(argc==3)//remove all 
		{
		FILE *file_in;
		file_in=fopen(argv[2],"wb");
		printf("Removed all files from the %s\n",argv[2]);
		fclose(file_in);			
		}
		else
		{
			int i=0;
			for(i=3;i<argc;i++)
			remv(argv[2],argv[i]);//remove specific file
		}		
	}
	else if(strcmp(argv[1],"-a")==0)//add files
	{
		add(argv[2],argv,argc);
	}
	else if(strcmp(argv[1],"-ra")==0)//read file and return the length of the file
	{
		read_archive(argv[2]);
	}
	
	
}
