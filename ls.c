#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <bsd/string.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fts.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>

#define BUFFSIZE 65536


static void display(FTSENT*, FTSENT*);
int compare(const FTSENT**, const FTSENT**);
int intlen(int num);
double logk(double x);



long blocksize = 512;

/*flag*/
int f_hide = 0;		/*list hidden file*/
int f_ctime = 0;
int f_atime = 0;
int f_mtime = 1;
int f_sort = 1;		/*sort the order*/
int f_reverse = 1;	/*reverse the order of sort*/
int f_type = 0;		/*display sign for different file type*/
int f_inode = 0;
int f_human = 0;
int f_longformat = 0;
int f_rec = 0;		/*recursively list subdirectories*/
int f_numID = 0;
int f_sortbysize = 0;
int f_sortbytime = 0;
int f_size = 0;		/*display number of file system blocks*/
int f_listdir = 0;
int f_printqustion = 0;


char dot[] = ".";
char *curdirec[] = { dot, NULL };


void
usage()
{
	printf("Usage:ls [-AacdFfhiklnqRrSstuw1] [file ...]\n");

	printf("\
		   		   	-A, --almost-all		list all entries except for . and ..\n\
								-a, --all				list all include directory entries who starting with .\n\
											-c						with -t: sort by ctime (time of file status last changed\n\
																				with -l: print ctime of files\n\
																					");
	printf("\
		   		   	-d, --directory			list directories as plain file, and symbolic links in \n\
														the argument list are not indirected through\n\
																	-F						display a / after each pathname that is a directory,\n\
																										a * after each that is executable, a @ after each\n\
																																			symbolic link, a %% after each whiteout, a = after each\n\
																																												socket, and a | after each that is a FIFO\n\
																																															-f						output is not sorted\n\
																																																");
	printf("\
		   		   	-h, --human-readable	with -s and -l, print human readable sizes. overrides -k\n\
								-i, --inode				print file's file serial number\n\
											-k, --kibibytes			display sizes in kilobytes\n\
												");
	printf("\
		   		   	-l						list in long formet, a total sum for all the file sizes\n\
														is output on a line before the long listing\n\
																							-n, --numeric-uid-gid	same as -l, but list numeric user and group IDs\n\
																										-q						force print of non-printable characters in file names as\n\
																																			the characters ?, this is default when output to terminal\n\
																																				");
	printf("\
		   		   	-R, --recursive			recursively list subdirectories encountered\n\
								-r, --reserve			reserve the order of sort\n\
											-S						sort by size, largest file first\n\
												");
	printf("\
		   		   	-s, --size				print the number of file system blocks actually use by each\n\
														file, in units of 512 bytes or BLOCKSIZE where partial units\n\
																							are rounded up to the next interger value.\n\
																										-t						sort by time modififed\n\
																													-u						with -t or -l, use time of last access instead of last\n\
																																						modification\n\
																																							");
	printf("\
		   		   	-w						force raw printing of non-printable characters. This is the\n\
														default when output is not to a terminal\n\
																	-1						force output to be one entry per line. This is the default\n\
																										when output is not to a terminal\n\
																											");
}



int
main(int argc, char *argv[])
{
	int fts_options;
	char ch;
	char *b;

	fts_options = FTS_PHYSICAL;

	if ((b = getenv("BLOCKSIZE")) != NULL)
		blocksize = (int)strtol(b, NULL, 10);
	if (blocksize <= 0)
		blocksize = 512;

	if (isatty(STDOUT_FILENO)){
		f_printqustion = 1;
	}
	else{
		f_printqustion = 0;
	}


	while ((ch = getopt(argc, argv, "AacdFfhiklnqRrSstuw1")) != -1){
		switch (ch){
		case 'A':
			f_hide = 1;
			break;
		case 'a':
			f_hide = 1;
			fts_options |= FTS_SEEDOT;
			break;
		case 'c':
			f_ctime = 1;
			f_atime = 0;
			f_mtime = 0;
			break;
		case 'd':
			f_listdir = 1;
			f_rec = 0;
			break;
		case 'F':
			f_type = 1;
			break;
		case 'f':
			f_sort = 0;
			break;
		case 'h':
			f_human = 1;
			break;
		case 'i':
			f_inode = 1;
			break;
		case 'k':
			blocksize = 1024;
			f_human = 0;
			break;
		case 'l':
			f_longformat = 1;
			break;
		case 'n':
			f_numID = 1;
			break;
		case 'q':
			f_printqustion = 1;
			break;
		case 'R':
			f_rec = 1;
			break;
		case 'r':
			f_reverse = -1;
			break;
		case 'S':
			f_sortbysize = 1;
			f_sortbytime = 0;
			break;
		case 's':
			f_size = 1;
			break;
		case 't':
			f_sortbysize = 0;
			f_sortbytime = 1;
			break;
		case 'u':
			f_ctime = 0;
			f_atime = 1;
			f_mtime = 0;
			break;
		case 'w':
			f_printqustion = 0;
			break;
		case '1':
			break;
		}
	}
	argc -= optind;
	argv += optind;




	if (!argc)
		argv = curdirec;

	FTS *ftsp = NULL;
	FTSENT *parent_p = NULL;
	FTSENT *child_p = NULL;
	int ch_options = 0;

	if ((ftsp = fts_open(argv, fts_options, f_sort ? &compare : NULL)) == NULL)
		err(EXIT_FAILURE, NULL);

	display(NULL, fts_children(ftsp, 0));
	if (f_listdir) {
		(void)fts_close(ftsp);
		return 0;
	}


	while ((parent_p = fts_read(ftsp)) != NULL)
	{
		switch (parent_p->fts_info){
		case FTS_DC:
			warnx("%s: directory causes a cycle", parent_p->fts_name);
			break;
		case FTS_DNR:
			warnx("%s: %s", parent_p->fts_name, strerror(parent_p->fts_errno));
			break;
		case FTS_ERR:
			warnx("%s: %s", parent_p->fts_name, strerror(parent_p->fts_errno));
			break;
		case FTS_D:
			if (parent_p->fts_level != FTS_ROOTLEVEL &&
				parent_p->fts_name[0] == '.' && !f_hide)
				break;

			child_p = fts_children(ftsp, ch_options);
			display(parent_p, child_p);

			if (!f_rec && child_p != NULL)
				(void)fts_set(ftsp, parent_p, FTS_SKIP);

			break;
		default:
			break;
		}
	}

	(void)fts_close(ftsp);



	return 0;
}





static void
display(FTSENT* parent_p, FTSENT* child_p)
{

	FTSENT *tmp;
	int block = 0;
	int maxblock = 0;
	int sumblock = 0;
	int maxblocklen = 0;
	int maxnlink = 0;
	int maxnlinklen = 0;
	int unamelen = 0;
	int maxunamelen = 0;
	int gnamelen = 0;
	int maxgnamelen = 0;
	uintmax_t sizelen = 0;
	uintmax_t maxsizelen = 0;
	int maxpathlen = 0;
	int maxinode = 0;
	int maxinodelen = 0;
	time_t t;
	time_t curtime;
	double maxunit;
	uintmax_t sumsize;
	int i;
	
	char unit_char[] = { ' ', 'K', 'M', 'G', 'T', 'P','E','Z','Y' };

	//
	if (parent_p != NULL){



		if (f_inode){
			for (tmp = child_p; tmp != NULL; tmp = tmp->fts_link){
				if ((tmp->fts_statp->st_ino > maxinode))
					maxinode = tmp->fts_statp->st_ino;
			}
		}

		if (f_size){
			for (tmp = child_p; tmp != NULL; tmp = tmp->fts_link){
				if ((tmp->fts_statp->st_blocks * 512 + blocksize - 1) / blocksize > maxblock)
					maxblock = (tmp->fts_statp->st_blocks * 512 + blocksize - 1) / blocksize;
				sumblock += (tmp->fts_statp->st_blocks * 512 + blocksize - 1) / blocksize;
			}
		}

		if (f_longformat){
			for (tmp = child_p; tmp != NULL; tmp = tmp->fts_link){
				if (!f_size){
					sumblock += (tmp->fts_statp->st_blocks * 512 + blocksize - 1) / blocksize;
				}

				if (tmp->fts_statp->st_nlink > maxnlink)
					maxnlink = tmp->fts_statp->st_nlink;

				if ((getpwuid(tmp->fts_statp->st_uid) != NULL) && !f_numID){
					if ((unamelen = strlen(getpwuid(tmp->fts_statp->st_uid)->pw_name)) > maxunamelen)
						maxunamelen = unamelen;
				}
				else{
					if ((unamelen = intlen(tmp->fts_statp->st_uid)) > maxunamelen)
						maxunamelen = unamelen;
				}

				if ((getgrgid(tmp->fts_statp->st_gid) != NULL) && !f_numID){
					if ((gnamelen = strlen(getgrgid(tmp->fts_statp->st_gid)->gr_name)) > maxgnamelen)
						maxgnamelen = gnamelen;
				}
				else{
					if ((gnamelen = intlen(tmp->fts_statp->st_gid)) > maxgnamelen)
						maxgnamelen = gnamelen;
				}

				if (S_ISCHR(tmp->fts_statp->st_mode) || S_ISBLK(tmp->fts_statp->st_mode)){
					sizelen = intlen(major(tmp->fts_statp->st_rdev)) +
						intlen(minor(tmp->fts_statp->st_rdev)) + 1;
				}
				else{
					sizelen = intlen(tmp->fts_statp->st_size);
				}
				if (sizelen > maxsizelen)
					maxsizelen = sizelen;

				if (tmp->fts_pathlen > maxpathlen)
					maxpathlen = (int)tmp->fts_pathlen;

			}
		}

		maxinodelen = intlen(maxinode);
		maxblocklen = intlen(maxblock);
		maxnlinklen = intlen(maxnlink);
		
		sumsize = sumblock*blocksize;

		if (sumsize <= 0)
			maxunit = 0;
		else
			maxunit = floor(logk(sumsize));

		if (f_rec)
			printf("%s:\n", parent_p->fts_path);

		if (f_size || f_longformat){
			if (!f_human)
				printf("total %i\n", (int)sumblock);
			else{
				if ((sumsize / pow(1024, maxunit)) > 10)
					printf("total %i%c\n", (int)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);
				else
					printf("total %1.1f%c\n", (float)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);

			}
		}

		//
	}

	while (child_p != NULL)
	{
		if (child_p->fts_name[0] != '.' || f_hide){

			if (f_inode){
				printf("%*i ", (int)maxinodelen, (int)child_p->fts_statp->st_ino);
			}


			if (f_size){
				block = (uintmax_t)(child_p->fts_statp->st_blocks * 512 + blocksize - 1) / blocksize;
				if (!f_human)
					printf("%*i ", (int)maxblocklen, (int)block);
				else{
					sumsize = block*blocksize;
					if (sumsize <= 0)
						maxunit = 0;
					else
						maxunit = floor(logk(sumsize));
					if ((int)maxunit == 0){
							printf("%5i ", (int)(sumsize / pow(1024, maxunit)));
					}
					else{
						if ((sumsize / pow(1024, maxunit)) > 10)
							printf("%4i%c ", (int)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);
						else
							printf(" %1.1f%c ", (float)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);
					}

				}
			}
			if (f_longformat){
				char bp[100];
				strmode(child_p->fts_statp->st_mode, bp);
				printf("%s ", bp);
				printf("%*i ", maxnlinklen, (int)child_p->fts_statp->st_nlink);

				if ((getpwuid(child_p->fts_statp->st_uid) != NULL) && !f_numID)
					printf("%-*s ", maxunamelen, getpwuid(child_p->fts_statp->st_uid)->pw_name);
				else
					printf("%-*i ", maxunamelen, child_p->fts_statp->st_uid);

				if ((getgrgid(child_p->fts_statp->st_gid) != NULL) && !f_numID)
					printf("%-*s ", maxgnamelen, getgrgid(child_p->fts_statp->st_gid)->gr_name);
				else
					printf("%-*i ", maxgnamelen, child_p->fts_statp->st_gid);

				if (!f_human){
					if (S_ISCHR(child_p->fts_statp->st_mode) || S_ISBLK(child_p->fts_statp->st_mode)){

						char devicenum[100];
						sprintf(devicenum, "%i/%i", major(child_p->fts_statp->st_rdev),
							minor(child_p->fts_statp->st_rdev));
						printf("%*s ", (int)maxsizelen, devicenum);
					}
					else{
						printf("%*ju ", (int)maxsizelen, (uintmax_t)child_p->fts_statp->st_size);
					}
				}
				else{
					if (S_ISCHR(child_p->fts_statp->st_mode) || S_ISBLK(child_p->fts_statp->st_mode)){

						char devicenum[100];
						sprintf(devicenum, "%i/%i", major(child_p->fts_statp->st_rdev),
							minor(child_p->fts_statp->st_rdev));
						printf("%*s ", (int)maxsizelen, devicenum);
					}
					else{
						sumsize = (double)child_p->fts_statp->st_size;
						if (sumsize <= 0)
							maxunit = 0;
						else
							maxunit = floor(logk(sumsize));
						if ((int)maxunit == 0){
							printf("%*i ", (int)maxsizelen, (int)(sumsize / pow(1024, maxunit)));
						}
						else{
							if ((sumsize / pow(1024, maxunit)) > 10)
								printf("%*i%c ", (int)maxsizelen - 1, (int)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);
							else
								printf(" %*.1f%c ", (int)maxsizelen-2,(float)(sumsize / pow(1024, maxunit)), unit_char[(int)maxunit]);
						}
					}
				}


				if (f_ctime == 1)
					t = child_p->fts_statp->st_ctime;
				if (f_atime == 1)
					t = child_p->fts_statp->st_atime;
				if (f_mtime == 1)
					t = child_p->fts_statp->st_mtime;
				curtime = time(NULL);
				char timestring[100];
				if ((t + 31556952 / 2) > curtime){	/*six months*/
					strftime(timestring, sizeof(timestring), "%b %2d %H:%M", localtime(&t));
					printf("%s ", timestring);
				}
				else{
					strftime(timestring, sizeof(timestring), "%b %2d  %Y", localtime(&t));
					printf("%s ", timestring);
				}

			}

			if (!f_printqustion)
				printf("%s", child_p->fts_name);
			else{
				char name[MAXPATHLEN + 1];
				int len = strlen(child_p->fts_name);
				snprintf(name, sizeof(name), "%s", child_p->fts_name);
				for (i = 0; i < len; i++){
					if (isprint(name[i]))
						putchar(name[i]);
					else
						putchar('?');
				}
			}
			

			if (f_type){
				if (S_ISDIR(child_p->fts_statp->st_mode))
					printf("/");
				if ((child_p->fts_statp->st_mode)&S_IXUSR)
					printf("*");
				if (S_ISLNK(child_p->fts_statp->st_mode))
					printf("@");
#ifdef S_ISWHT
				if (S_ISWHT(child_p->fts_statp->st_mode))
					printf("%");
#endif
				if (S_ISSOCK(child_p->fts_statp->st_mode))
					printf("=");
				if (S_ISFIFO(child_p->fts_statp->st_mode))
					printf("|");
			}

			

			if (f_longformat){
				if (S_ISLNK(child_p->fts_statp->st_mode)){
					char file[MAXPATHLEN + 1], path[MAXPATHLEN + 1];
					if (child_p->fts_level == FTS_ROOTLEVEL)
						snprintf(file, sizeof(file), "%s", child_p->fts_name);
					else
						snprintf(file, sizeof(file), "%s/%s",
						child_p->fts_parent->fts_accpath, child_p->fts_name);
					int linklen;
					if ((linklen = readlink(file, path, sizeof(path) - 1)) != -1){
						path[linklen] = '\0';
						printf(" -> ");
						if (!f_printqustion)
							printf("%s", path);
						else{
							int len = strlen(path);
							for (i = 0; i < len; i++){
								if (isprint(path[i]))
									putchar(path[i]);
								else
									putchar('?');
							}
						}
					}
				}
			}

			printf("\n");

		}
		child_p = child_p->fts_link;
	}
	if (f_rec)
		printf("\n");
}


int compare(const FTSENT **a, const FTSENT **b)
{
	int a_info, b_info;

	a_info = (*a)->fts_info;
	if (a_info == FTS_ERR)
		return (0);
	b_info = (*b)->fts_info;
	if (b_info == FTS_ERR)
		return (0);

	if (a_info == FTS_NS || b_info == FTS_NS) {
		if (b_info != FTS_NS)
			return (1);
		else if (a_info != FTS_NS)
			return (-1);
		else
			return (strcmp((*a)->fts_name, (*b)->fts_name));
	}

	if (!f_sortbytime && !f_sortbysize)
		return (f_reverse*strcmp((*a)->fts_name, (*b)->fts_name));
	else if (f_sortbysize){
		if ((*b)->fts_statp->st_size > (*a)->fts_statp->st_size)
			return(f_reverse * 1);
		if ((*a)->fts_statp->st_size > (*b)->fts_statp->st_size)
			return(f_reverse * -1);
		else
			return (f_reverse*strcmp((*a)->fts_name, (*b)->fts_name));
	}
	else if (f_mtime){
		if ((*b)->fts_statp->st_mtime > (*a)->fts_statp->st_mtime)
			return(f_reverse * 1);
		if ((*a)->fts_statp->st_mtime > (*b)->fts_statp->st_mtime)
			return(f_reverse * -1);
		else
			return (f_reverse*strcmp((*a)->fts_name, (*b)->fts_name));
	}
	else if (f_atime){
		if ((*b)->fts_statp->st_atime > (*a)->fts_statp->st_atime)
			return(f_reverse * 1);
		if ((*a)->fts_statp->st_atime > (*b)->fts_statp->st_atime)
			return(f_reverse * -1);
		else
			return (f_reverse*strcmp((*a)->fts_name, (*b)->fts_name));
	}
	else if (f_ctime){
		if ((*b)->fts_statp->st_ctime > (*a)->fts_statp->st_ctime)
			return(f_reverse * 1);
		if ((*a)->fts_statp->st_ctime > (*b)->fts_statp->st_ctime)
			return(f_reverse * -1);
		else
			return (f_reverse*strcmp((*a)->fts_name, (*b)->fts_name));
	}
	return (strcmp((*a)->fts_name, (*b)->fts_name));

}

int intlen(int num){
	char testing[100];
	int len;
	sprintf(testing, "%i", num);
	len = strlen(testing);
	return len;
}

double logk(double x){
	return log(x) / log(1024);
}

