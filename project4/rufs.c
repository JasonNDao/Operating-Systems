/*
 *  Copyright (C) 2022 CS416/518 Rutgers CS
 *	RU File System
 *	File:	rufs.c
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>
#include <pthread.h>

#include "block.h"
#include "rufs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
struct superblock * a;
bitmap_t iBitmap;
bitmap_t dBitmap;
pthread_mutex_t lock1=PTHREAD_MUTEX_INITIALIZER;
/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {

	// Step 1: Read inode bitmap from disk
	bio_read(1,iBitmap);
	
	// Step 2: Traverse inode bitmap to find an available slot
	int index;
	for (index=0;index<MAX_INUM;index++){ 
	    if (get_bitmap(iBitmap,index)==0){
            set_bitmap(iBitmap, index);
			bio_write(1,iBitmap);
			return index;
        }
    }
	fprintf(stderr,"Out of inodes");
	return -1;

	// Step 3: Update inode bitmap and write to disk 
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {

	// Step 1: Read data block bitmap from disk
	bio_read(2,dBitmap);
	// Step 2: Traverse data block bitmap to find an available slot
	int index;
	for (index=0;index<MAX_DNUM;index++){    //find virtual page in bitmap
	    if (get_bitmap(dBitmap,index)==0){
            set_bitmap(dBitmap, index);
			bio_write(2,dBitmap);
			return index;
        }
    }
	// Step 3: Update data block bitmap and write to disk 
	fprintf(stderr,"Out of blocks");
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number
	int in=ino/(BLOCK_SIZE/sizeof(struct inode));
  // Step 2: Get offset of the inode in the inode on-disk block
	int offset=ino%(BLOCK_SIZE/sizeof(struct inode));
  // Step 3: Read the block from disk and then copy into inode structure
	struct inode * first= (struct inode *)malloc(BLOCK_SIZE);
	bio_read(in+a->i_start_blk,first);    //reads first inode
	*inode=first[offset];
	free(first);
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	int in=ino/(BLOCK_SIZE/sizeof(struct inode));
	// Step 2: Get the offset in the block where this inode resides on disk
	int offset=ino%(BLOCK_SIZE/sizeof(struct inode));
	// Step 3: Write inode to disk 
	struct inode * first= (struct inode *)malloc(BLOCK_SIZE);
	bio_read(in+a->i_start_blk,first);    //reads first inode
	first[offset]=*inode;
	bio_write(in+a->i_start_blk, first);
	free(first);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
	struct inode currdirectory;
	readi(ino, &currdirectory);
  // Step 2: Get data block of current directory from inode
	struct dirent * traverse=(struct dirent *)malloc(BLOCK_SIZE);
	struct dirent * traverse2;
	for (int k=0;k<16;k++){
		if (currdirectory.direct_ptr[k]==-1){
			break;
		}
		//printf("%d is the block number for find and the name is %s\n", currdirectory.direct_ptr[k], fname);
		bio_read(currdirectory.direct_ptr[k], traverse);
  // Step 3: Read directory's data block and check each directory entry.
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			/*if (traverse[i].valid==0){
				printf("%s\t%d", traverse[i].name, traverse[i].len);
			}*/
			if (traverse[i].valid==0 && strcmp(traverse[i].name, fname)==0){
				traverse2=&traverse[i];
				memcpy(dirent, traverse2, sizeof(struct dirent));
				//printf("Finished find\n");
				time(&currdirectory.vstat.st_atime);
				writei(ino, &currdirectory);
				free(traverse);
				return 0;
			}
		}
	}
  //If the name matches, then copy directory entry to dirent structure
 	free(traverse);
	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode	
	// Step 2: Check if fname (directory name) is already used in other entries
	struct dirent * blockreader = (struct dirent *)malloc(BLOCK_SIZE);
	struct dirent * traverse2;
	int k=0;
	int lw=0;
	int first=0;
	for (k=0;k<16;k++){
		if (dir_inode.direct_ptr[k]==-1){
			break;
		}
		bio_read(dir_inode.direct_ptr[k], blockreader );
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			/*if (blockreader[i].valid==0){
				printf("%s\t%d", blockreader[i].name, blockreader[i].len);
			}*/
			if (blockreader[i].valid==1 && first==0){
				traverse2=&blockreader[i];
				first=1;
				lw=k;
			}
			if (blockreader[i].valid==0 && strcmp(blockreader[i].name, fname)==0){
				free(blockreader);
				return -1;
			}
		}
	}
	// Step 3: Add directory entry in dir_inode's data block and write to disk
	// Allocate a new data block for this directory if it does not exist
	if (k==16 && lw==0){
		fprintf(stderr,"No more room for a dirent");
		free(blockreader);
		return -1;
	}
	if (first==0){
		int next2=get_avail_blkno();
		dir_inode.direct_ptr[k]=next2;
		writei(dir_inode.ino, &dir_inode);
		struct dirent* blank2=(struct dirent *)malloc(sizeof(struct dirent));
		blank2->valid=1;
		struct dirent* blank=(struct dirent *)malloc(BLOCK_SIZE);
		memset(blank,0,BLOCK_SIZE);
		for(int i=0;i<BLOCK_SIZE/sizeof(struct dirent); i++){
			blank[i]=*blank2;
		}
		bio_write(next2, blank);
		free(blank);
		free(blank2);
		bio_read(dir_inode.direct_ptr[k], blockreader );
		int first=0;
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			if (blockreader[i].valid==1 && first==0){
				traverse2=&blockreader[i];
				first=1;
			}
			if (blockreader[i].valid==0 && strcmp(blockreader[i].name, fname)==0){
				free(blockreader);
				return -1;
			}
		}
		lw=k;
	}
	// Update directory inode
	strcpy(traverse2->name, fname);
	traverse2->len=name_len;
	traverse2->ino=f_ino;
	traverse2->valid=0;
	dir_inode.size+=sizeof(struct dirent);
	dir_inode.vstat.st_size=dir_inode.size;
	dir_inode.link++;
	dir_inode.vstat.st_nlink=dir_inode.link;
	time(&dir_inode.vstat.st_mtime);
	time(&dir_inode.vstat.st_atime);
	writei(dir_inode.ino, &dir_inode);
	// Write directory entry
	//printf("%d is the block number for add and the name is %s\n", dir_inode.direct_ptr[lw], fname);
	//printf("%d\n", lw);
	bio_write(dir_inode.direct_ptr[lw], blockreader);
	free(blockreader);
	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	struct dirent * blockreader = (struct dirent *)malloc(BLOCK_SIZE);
	for (int k=0;k<16;k++){
		if (dir_inode.direct_ptr[k]==-1){
			break;
		}
		bio_read(dir_inode.direct_ptr[k], blockreader );
	// Step 2: Check if fname exist
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			if (blockreader[i].valid==0 && strcmp(blockreader[i].name, fname)==0){
				struct dirent* blank2=(struct dirent *)malloc(sizeof(struct dirent));
				blank2->valid=1;
				blockreader[i]=*blank2;
				dir_inode.size-=sizeof(struct dirent);
				dir_inode.vstat.st_size=dir_inode.size;
				dir_inode.link--;
				dir_inode.vstat.st_nlink=dir_inode.link;
				time(&dir_inode.vstat.st_mtime);
				time(&dir_inode.vstat.st_atime);
				writei(dir_inode.ino, &dir_inode);
				bio_write(dir_inode.direct_ptr[k], blockreader);
				free(blank2);
				free(blockreader);
				return 0;
			}
		}
	}
	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	free(blockreader);
	return -1;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	struct dirent root;
	char s[strlen(path)+100];
	memset(s, 0, strlen(path)+100);
	int first=1;
	int count=1;
	int flag=0;
	if (strcmp(path, "/")==0){
		readi(0, inode);
		return 0;
	}
	for (int i=1;i<strlen(path);i++){
		if (path[i]=='/' || i==strlen(path)-1){
			if (path[i]=='/'){
				memcpy(s, &path[first], count-1);
			}
			else{
				memcpy(s, &path[first], count);
			}
			s[count]= '\0';
			if (flag==0){
				int t=dir_find(ino, s, strlen(s), &root);
				flag=1;
				if (t==-1){
					readi(0, inode);
					return -1;
				}
			}
			else{
				int t=dir_find(root.ino, s, strlen(s), &root);
				if (t==-1){
					readi(root.ino, inode);
					return -1;
				}
			}
			memset(s, 0, strlen(path)+100);
			count=0;
			first=i+1;
		}		
		count++;
	}
	readi(root.ino, inode);
	return 0;
}

/* 
 * Make file system
 */
int rufs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);

	// write superblock information
	dev_open(diskfile_path);
	struct superblock * reala2 = (struct superblock *)malloc(BLOCK_SIZE);
	a=reala2;
	a->magic_num=MAGIC_NUM;
	a->max_dnum=MAX_DNUM;
	a->max_inum=MAX_INUM;
	a->d_start_blk=(MAX_INUM/(BLOCK_SIZE/sizeof(struct inode))+3);
	a->i_start_blk=3;
	a->i_bitmap_blk=1;
	a->d_bitmap_blk=2;
	bio_write(0, reala2);

	// initialize inode bitmap
	iBitmap = (bitmap_t)malloc(BLOCK_SIZE);
	memset(iBitmap,0,BLOCK_SIZE);
	bio_write(1,iBitmap);

	// initialize data block bitmap
	dBitmap = (bitmap_t)malloc(BLOCK_SIZE);
	memset(dBitmap,0,BLOCK_SIZE);
	set_bitmap(dBitmap, 0);
	set_bitmap(dBitmap, 1);
	set_bitmap(dBitmap, 2);
	bio_write(2,dBitmap);
	// update bitmap information for root directory
	struct inode * root=(struct inode *)malloc(sizeof(struct inode));
	root->valid=1;
	struct inode * root3=(struct inode *)malloc(BLOCK_SIZE);
	struct dirent* blank2=(struct dirent *)malloc(sizeof(struct dirent));
	blank2->valid=1;
	struct dirent* blank=(struct dirent *)malloc(BLOCK_SIZE);
	memset(blank, 0, BLOCK_SIZE);
	memset(root3, 0, BLOCK_SIZE);
	for(int i=0;i<BLOCK_SIZE/sizeof(struct dirent); i++){
		blank[i]=*blank2;
	}
	for(int i=0;i<BLOCK_SIZE/sizeof(struct inode); i++){
		root3[i]=*root;
	}
	root->valid=0;
	root->ino=0;
	for (int i=0;i<16;i++){
		root->direct_ptr[i]=-1;
	}
	root->direct_ptr[0]=a->d_start_blk;
	root->link=2;
	root->size=sizeof(struct dirent)*2;
	root->type=S_IFDIR | 0755;
	root->vstat.st_uid = getuid();
	root->vstat.st_gid = getgid();
	root->vstat.st_size = root->size;
	root->vstat.st_mode = S_IFDIR | 0755;
	root->vstat.st_nlink = root->link;
	time(&root->vstat.st_mtime);
	time(&root->vstat.st_atime);
	for (int i=4;i<a->d_start_blk;i++){
		set_bitmap(dBitmap,i);
		bio_write(i, root3);
	}
	root3[0]=*root;
	bio_write(3,root3);
	set_bitmap(dBitmap, a->d_start_blk);
	set_bitmap(dBitmap, 3);
	set_bitmap(iBitmap, 0);
	bio_write(1,iBitmap);
	bio_write(2,dBitmap);
	bio_write(a->d_start_blk, blank);


	// update inode for root directory
	free(blank2);
	free(root);
	free(blank);
	free(root3);
	return 0;
}


/* 
 * FUSE file operations
 */
static void *rufs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
	pthread_mutex_lock(&lock1);
	dev_open(diskfile_path);
	a=(struct superblock *)malloc(BLOCK_SIZE);
	bio_read(0, a);
	if (a->magic_num!=MAGIC_NUM){
		dev_close();
		rufs_mkfs();
		pthread_mutex_unlock(&lock1);
		return NULL;
	}

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
	iBitmap = (bitmap_t)malloc(BLOCK_SIZE);
	bio_read(1,iBitmap);
	dBitmap = (bitmap_t)malloc(BLOCK_SIZE);
	bio_read(2,dBitmap);
	pthread_mutex_unlock(&lock1);
	return NULL;
}

static void rufs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	pthread_mutex_lock(&lock1);
	for (int index=0;index<MAX_INUM;index++){ 
	    if (get_bitmap(iBitmap,index)==0){
			printf("INODE INDEXES USED %d\n", index);
			break;
        }
    }
	for (int index=0;index<MAX_DNUM;index++){ 
	    if (get_bitmap(dBitmap,index)==0){
			printf("DATA INDEXES USED %d\n", index);
			break;
        }
    }
	free(a);
	free(dBitmap);
	free(iBitmap);
	// Step 2: Close diskfile
	dev_close();
	pthread_mutex_unlock(&lock1);

}

static int rufs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int turn=get_node_by_path(path, 0, &attr);
	if (turn==-1){
		pthread_mutex_unlock(&lock1);
		return -ENOENT;
	}
	//printf("Getting attribute of this path:%s\n", path);
	// Step 2: fill attribute of file into stbuf from inode
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	stbuf->st_size = attr.size;
	stbuf->st_mode = attr.type;
	stbuf->st_nlink = attr.link;
	stbuf->st_mtime= attr.vstat.st_mtime;
	stbuf->st_atime= attr.vstat.st_atime;
	if (attr.size%BLOCK_SIZE==0){
		stbuf->st_blksize=stbuf->st_size/BLOCK_SIZE;
	}
	else{
		stbuf->st_blksize=(stbuf->st_size/BLOCK_SIZE)+1;
	}
	pthread_mutex_unlock(&lock1);
	return 0;
}

static int rufs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int turn=get_node_by_path(path, 0, &attr);
	// Step 2: If not find, return -1
	pthread_mutex_unlock(&lock1);
    return turn;
}

static int rufs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int u=get_node_by_path(path, 0, &attr);
	if (u==-1){
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	char * base = (char *)malloc((strlen(path)+100)*sizeof(char));
	char * parent = (char *)malloc((strlen(path)+100)*sizeof(char));
	memset(base, 0, (strlen(path)+100));
	memset(parent, 0, (strlen(path)+100));
	for (int i=strlen(path)-1;i>-1;i--){
		if (path[i]=='/' && i!=strlen(path)-1){
			memcpy(base, &path[i+1], strlen(path)-1-i);
			base[strlen(path)-1-i+1]= '\0';
			memcpy(parent, &path[0], strlen(path)-(strlen(path)-1-i));
			parent[strlen(path)-(strlen(path)-1-i)+1]= '\0';
			break;
		}
	}
	// printf("Parent:%s\n", parent);
	// printf("Base:%s\n", base);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode attr0;
	get_node_by_path(parent, 0, &attr0);
	filler( buffer, ".", &attr.vstat, 0 );
	if (strcmp(path, "/")!=0){
		filler( buffer, "..", &attr0.vstat, 0 );
	}
	else{
		filler( buffer, "..", &attr.vstat, 0 );
	}
	// Step 2: Read directory entries from its data blocks, and copy them to filler
	struct dirent * traverse=(struct dirent *)malloc(BLOCK_SIZE);
	for (int k=0;k<16;k++){
		if (attr.direct_ptr[k]==-1){
			break;
		}
		bio_read(attr.direct_ptr[k], traverse);
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			if (traverse[i].valid==0){
				struct inode attr2;
				char s[strlen(path)+strlen(traverse[i].name)+50];
				strcat(s,path);
				strcat(s,traverse[i].name);
				get_node_by_path(s, 0, &attr2);
				filler( buffer, traverse[i].name, &attr2.vstat, 0 );
			}
		}
	}
	free(traverse);
	free(base);
	free(parent);
	pthread_mutex_unlock(&lock1);
	return 0;
}


static int rufs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	pthread_mutex_lock(&lock1);
	char * base = (char *)malloc((strlen(path)+100)*sizeof(char));
	char * parent = (char *)malloc((strlen(path)+100)*sizeof(char));
	memset(base, 0, (strlen(path)+100));
	memset(parent, 0, (strlen(path)+100));
	for (int i=strlen(path)-1;i>-1;i--){
		if (path[i]=='/' && i!=strlen(path)-1){
			memcpy(base, &path[i+1], strlen(path)-1-i);
			base[strlen(path)-1-i+1]= '\0';
			memcpy(parent, &path[0], strlen(path)-(strlen(path)-1-i));
			parent[strlen(path)-(strlen(path)-1-i)+1]= '\0';
			break;
		}
	}
	// printf("Parent:%s\n", parent);
	// printf("Base:%s\n", base);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode attr;
	int u=get_node_by_path(parent, 0, &attr);
	if (u==-1){
		free(parent);
		free(base);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	// Step 3: Call get_avail_ino() to get an available inode number
	int next=get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	u=dir_add(attr, next, base, strlen(base));
	if (u==-1){
		free(parent);
		free(base);
		unset_bitmap(iBitmap, next);
		bio_write(1, iBitmap);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	int next2=get_avail_blkno();
	// Step 5: Update inode for target directory
	struct inode newfile;
	newfile.ino=next;
	for (int i=0;i<16;i++){
		newfile.direct_ptr[i]=-1;
	}
	newfile.direct_ptr[0]=next2;
	newfile.valid=0;
	newfile.link=2;
	newfile.size=sizeof(struct dirent)*2;
	newfile.type=S_IFDIR | 0755;
	newfile.vstat.st_uid = getuid();
	newfile.vstat.st_gid = getgid();
	newfile.vstat.st_size = newfile.size;
	newfile.vstat.st_mode = S_IFDIR | 0755;
	newfile.vstat.st_nlink = newfile.link;
	time(&newfile.vstat.st_mtime);
	time(&newfile.vstat.st_atime);
	// Step 6: Call writei() to write inode to disk
	writei(next, &newfile);
	struct dirent* blank2=(struct dirent *)malloc(sizeof(struct dirent));
	blank2->valid=1;
	struct dirent* blank=(struct dirent *)malloc(BLOCK_SIZE);
	memset(blank,0,BLOCK_SIZE);
	for(int j=0;j<BLOCK_SIZE/sizeof(struct dirent); j++){
		blank[j]=*blank2;
	}
	bio_write(next2, blank);
	free(parent);
	free(base);
	free(blank2);
	free(blank);
	pthread_mutex_unlock(&lock1);
	return 0;
}

static int rufs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	pthread_mutex_lock(&lock1);
	char * base = (char *)malloc((strlen(path)+100)*sizeof(char));
	char * parent = (char *)malloc((strlen(path)+100)*sizeof(char));
	memset(base, 0, (strlen(path)+100));
	memset(parent, 0, (strlen(path)+100));
	for (int i=strlen(path)-1;i>-1;i--){
		if (path[i]=='/' && i!=strlen(path)-1){
			memcpy(base, &path[i+1], strlen(path)-1-i);
			base[strlen(path)-1-i+1]= '\0';
			memcpy(parent, &path[0], strlen(path)-(strlen(path)-1-i));
			parent[strlen(path)-(strlen(path)-1-i)+1]= '\0';
			break;
		}
	}
	// printf("Parent:%s\n", parent);
	// printf("Base:%s\n", base);
	// Step 2: Call get_node_by_path() to get inode of target directory
	struct inode attr;
	int j=get_node_by_path(path, 0, &attr);
	if (j==-1){
		free(base);
		free(parent);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
  // Step 2: Get data block of current directory from inode
	struct dirent * traverse=(struct dirent *)malloc(BLOCK_SIZE);
	for (int k=0;k<16;k++){
		if (attr.direct_ptr[k]==-1){
			break;
		}
		bio_read(attr.direct_ptr[k], traverse);
		for (int i=0; i<BLOCK_SIZE/sizeof(struct dirent); i++){
			if (traverse[i].valid==0){
				free(base);
				free(parent);
				free(traverse);
				fprintf(stderr,"Cannot delete directory with file inside");
				pthread_mutex_unlock(&lock1);
				return -1;
			}
		}
	}
	// Step 3: Clear data block bitmap of target directory
	for (int i=0;i<16;i++){
		if (attr.direct_ptr[i]!=-1){
			void * st=malloc(BLOCK_SIZE);
			unset_bitmap(dBitmap,attr.direct_ptr[i]);
			bio_write(attr.direct_ptr[i], st);
			free(st);
		}
	}
	bio_write(2,dBitmap);
	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(iBitmap,attr.ino);
	bio_write(1,iBitmap);
	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode attr2;
	get_node_by_path(parent, 0, &attr2);
	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory
	j=dir_remove(attr2, base, strlen(base));
	free(base);
	free(parent);
	free(traverse);
	pthread_mutex_unlock(&lock1);
	if (j==-1){
		return -1;
	}
	return 0;
}

static int rufs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	pthread_mutex_lock(&lock1);
	char * base = (char *)malloc((strlen(path)+100)*sizeof(char));
	char * parent = (char *)malloc((strlen(path)+100)*sizeof(char));
	memset(base, 0, (strlen(path)+100));
	memset(parent, 0, (strlen(path)+100));
	for (int i=strlen(path)-1;i>-1;i--){
		if (path[i]=='/' && i!=strlen(path)-1){
			memcpy(base, &path[i+1], strlen(path)-1-i);
			base[strlen(path)-1-i+1]= '\0';
			memcpy(parent, &path[0], strlen(path)-(strlen(path)-1-i));
			parent[strlen(path)-(strlen(path)-1-i)+1]= '\0';
			break;
		}
	}
	// printf("Parent:%s\n", parent);
	// printf("Base:%s\n", base);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode attr;
	int u=get_node_by_path(parent, 0, &attr);
	if (u==-1){
		free(parent);
		free(base);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	
	// Step 3: Call get_avail_ino() to get an available inode number
	int next=get_avail_ino();
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	u=dir_add(attr, next, base, strlen(base));
	if (u==-1){
		free(parent);
		free(base);
		unset_bitmap(iBitmap, next);
		bio_write(1, iBitmap);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	int next2=get_avail_blkno();
	// Step 5: Update inode for target file
	struct inode newfile;
	newfile.ino=next;
	for (int i=0;i<16;i++){
		newfile.direct_ptr[i]=-1;
	}
	newfile.direct_ptr[0]=next2;
	newfile.valid=0;
	newfile.link=1;
	newfile.size=0;
	newfile.type=S_IFREG | 0644;
	newfile.vstat.st_uid = getuid();
	newfile.vstat.st_gid = getgid();
	newfile.vstat.st_size = newfile.size;
	newfile.vstat.st_mode = S_IFREG | 0644;
	newfile.vstat.st_nlink = newfile.link;
	time(&newfile.vstat.st_mtime);
	time(&newfile.vstat.st_atime);
	//printf("%dABIGAIL%d\n", next, next2);
	char* blank=(char *)malloc(BLOCK_SIZE);
	memset(blank,0,BLOCK_SIZE);
	bio_write(next2, blank);
	// Step 6: Call writei() to write inode to disk
	writei(next, &newfile);
	free(blank);
	free(base);
	free(parent);
	pthread_mutex_unlock(&lock1);
	return 0;
}

static int rufs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int turn=get_node_by_path(path, 0, &attr);
	// Step 2: If not find, return -1
	pthread_mutex_unlock(&lock1);
	return turn;
}

static int rufs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int u=get_node_by_path(path, 0, &attr);
	if (u==-1 || size>16*BLOCK_SIZE || offset>16*BLOCK_SIZE){
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	int othersize=size;
	int start=offset/BLOCK_SIZE;
	int otheroffset=offset-(start*BLOCK_SIZE);
	int remaining=offset-(start*BLOCK_SIZE);
	remaining=BLOCK_SIZE-(remaining%BLOCK_SIZE);
	int count=0;
	char * ll=(char *)malloc(16*BLOCK_SIZE);
	// Step 2: Based on size and offset, read its data blocks from disk
	while(othersize!=0){
		char * stuff=(char *)malloc(BLOCK_SIZE);
		bio_read(attr.direct_ptr[start], stuff);
		// Step 3: copy the correct amount of data from offset to buffer
		// printf("%ldasdaasdasd\n", size);
		// printf("%ldasdaasdasd\n", offset);
		// printf("%d\t", othersize);
		// printf("%d\t", start);
		// printf("%d\t", remaining);
		// printf("%d\n", otheroffset);
		if (othersize>remaining){
			memcpy(ll+count,stuff+otheroffset, remaining);
			othersize-=remaining;
			count+=remaining;
		}
		else{
			memcpy(ll+count,stuff+otheroffset, othersize);
			othersize=0;
		}
		//printf("%s", buffer);
		free(stuff);
		start++;
		otheroffset=0;
		remaining=BLOCK_SIZE;

	}
	// Note: this function should return the amount of bytes you copied to buffer
	memcpy(buffer,ll,size);
	time(&attr.vstat.st_atime);
	writei(attr.ino, &attr);
	free(ll);
	pthread_mutex_unlock(&lock1);
	return size;
}

static int rufs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path
	pthread_mutex_lock(&lock1);
	struct inode attr;
	int u=get_node_by_path(path, 0, &attr);
	if (u==-1 || size>16*BLOCK_SIZE || offset>16*BLOCK_SIZE){
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	int othersize=size;
	int start=offset/BLOCK_SIZE;
	int otheroffset=offset-(start*BLOCK_SIZE);
	int remaining=offset-(start*BLOCK_SIZE);
	remaining=BLOCK_SIZE-(remaining%BLOCK_SIZE);
	// Step 2: Based on size and offset, read its data blocks from disk
	while (othersize!=0){
		char * stuff=(char *)malloc(BLOCK_SIZE);
		if(attr.direct_ptr[start]==-1){
			int next2=get_avail_blkno();
			attr.direct_ptr[start]=next2;
			char * blank=(char *)malloc(BLOCK_SIZE);
			memset(blank,0,BLOCK_SIZE);
			bio_write(next2, blank);
			free(blank);
		}
		bio_read(attr.direct_ptr[start], stuff);
		// Step 3: Write the correct amount of data from offset to disk
		if (othersize>remaining){
			memcpy(stuff+otheroffset,buffer, remaining);
			othersize-=remaining;
		}
		else{
			memcpy(stuff+otheroffset,buffer, othersize);
			othersize=0;
		}
		// Step 4: Update the inode info and write it to disk
		bio_write(attr.direct_ptr[start], stuff);
		free(stuff);
		start++;
		otheroffset=0;
		remaining=BLOCK_SIZE;
	}
	// Note: this function should return the amount of bytes you write to disk
	attr.size=offset+size;
	time(&attr.vstat.st_mtime);
	time(&attr.vstat.st_atime);
	writei(attr.ino, &attr);
	pthread_mutex_unlock(&lock1);
	return size;
}

static int rufs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	pthread_mutex_lock(&lock1);
	char * base = (char *)malloc((strlen(path)+100)*sizeof(char));
	char * parent = (char *)malloc((strlen(path)+100)*sizeof(char));
	memset(base, 0, (strlen(path)+100));
	memset(parent, 0, (strlen(path)+100));
	for (int i=strlen(path)-1;i>-1;i--){
		if (path[i]=='/' && i!=strlen(path)-1){
			memcpy(base, &path[i+1], strlen(path)-1-i);
			base[strlen(path)-1-i+1]= '\0';
			memcpy(parent, &path[0], strlen(path)-(strlen(path)-1-i));
			parent[strlen(path)-(strlen(path)-1-i)+1]= '\0';
			break;
		}
	}
	// printf("Parent:%s\n", parent);
	// printf("Base:%s\n", base);
	// Step 2: Call get_node_by_path() to get inode of target file
	struct inode attr;
	int j=get_node_by_path(path, 0, &attr);
	if (j==-1){
		free(base);
		free(parent);
		pthread_mutex_unlock(&lock1);
		return -1;
	}
	// Step 3: Clear data block bitmap of target file
	for (int i=0;i<16;i++){
		if (attr.direct_ptr[i]!=-1){
			void * st=malloc(BLOCK_SIZE);
			unset_bitmap(dBitmap,attr.direct_ptr[i]);
			bio_write(attr.direct_ptr[i], st);
			free(st);
		}
	}
	bio_write(2,dBitmap);
	// Step 4: Clear inode bitmap and its data block
	unset_bitmap(iBitmap,attr.ino);
	bio_write(1,iBitmap);
	// Step 5: Call get_node_by_path() to get inode of parent directory
	struct inode attr2;
	get_node_by_path(parent, 0, &attr2);
	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	j=dir_remove(attr2, base, strlen(base));
	free(base);
	free(parent);
	pthread_mutex_unlock(&lock1);
	if (j==-1){
		return -1;
	}
	return 0;
}

static int rufs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int rufs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int rufs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations rufs_ope = {
	.init		= rufs_init,
	.destroy	= rufs_destroy,

	.getattr	= rufs_getattr,
	.readdir	= rufs_readdir,
	.opendir	= rufs_opendir,
	.releasedir	= rufs_releasedir,
	.mkdir		= rufs_mkdir,
	.rmdir		= rufs_rmdir,

	.create		= rufs_create,
	.open		= rufs_open,
	.read 		= rufs_read,
	.write		= rufs_write,
	.unlink		= rufs_unlink,

	.truncate   = rufs_truncate,
	.flush      = rufs_flush,
	.utimens    = rufs_utimens,
	.release	= rufs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &rufs_ope, NULL);

	return fuse_stat;
}



