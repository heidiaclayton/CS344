#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <ar.h>
#include <dirent.h>

int is_correct_extension(char **argv) {
	int length = strlen(argv[1]);
	char ext[2];

	if (length < 3) {
		fprintf(stderr, "Incorrect file type. Must have '.a' extension.\n");
		return 0;
	}

	ext[0] = argv[1][length - 2];
	ext[1] = argv[1][length - 1];

	if (strcmp(ext, ".a")) {
		fprintf(stderr, "Incorrect file type. Must have '.a' extension.\n");
		return 0;
	}

	return 1;
}



void quick_append(int argc, char **argv) {
	int fd, x, bytes_read, i, current;
	
	/*size 15 because ar.h*/
	char start_string[SARMAG], ar_name[17], ar_date[13], ar_uid[7], ar_gid[7], ar_mode[8], ar_size[10];

	if (!is_correct_extension(argv))
		return;

	/*open or create given .a file*/
	/*0666 is octal number that sets permissions to rwx*/
	fd = open(argv[1], O_RDWR | O_CREAT, 0666); 

	if (fd == -1) {
		printf("Cannot open or create archive file.\n");
		return;
	}
	/*checking for required starting string of archive*/
	bytes_read = read(fd, start_string, SARMAG);

	if (bytes_read == -1) {
		printf("Error reading file.\n");
		return;
	}

	/*if file isn't empty, make sure it contains starting string*/
	if (bytes_read > 0) {
		if (strcmp(start_string, ARMAG)) {
			printf("Incorrectly formatted .a file.\n");
			return;
		}
	}

	/*if file is empty, put starting string at the beginning*/
	else {
		x = write(fd, ARMAG, SARMAG);
		if (x == -1) {
			printf("Error formatting archive.\n");
			return;
		}
	}


	/*start looping through files given on command line starting at argv[3]*/
	for (i = 0; i < (argc - 3); i++) {	
		
		/*seek to end of archive*/
		x = lseek(fd, 0, SEEK_END);

		if (x == -1) {
			printf("Error seeking end of file.\n");
			return;
		}

		/*open file*/
		
		current = open(argv[3 + i], O_RDONLY);

		if (current == -1) {
			printf("Could not open file %s.\n", argv[3 + i]);
			return;
		}

		/*declare stats and header*/
		struct stat stats;
		struct ar_hdr header;

		x = fstat(current, &stats);

		if (x == -1) {
			printf("Error stat-ing file %s.\n", argv[3 + i]);
			return;
		}
		/*get info from stat struct*/
		snprintf(ar_name, 17, "%-16.16s", argv[3 + i]);
		snprintf(ar_date, 13, "%-12u", stats.st_mtime);
	        snprintf(ar_uid, 7, "%-6u", stats.st_uid);
                snprintf(ar_gid, 7, "%-6u", stats.st_gid);
	        snprintf(ar_mode, 9, "%-8o", stats.st_mode);
		snprintf(ar_size, 11, "%-10u", stats.st_size);
		

		/*put info in header*/
		memcpy(header.ar_name, ar_name, 16);
		memcpy(header.ar_date, ar_date, 12);
		memcpy(header.ar_uid, ar_uid, 6);
		memcpy(header.ar_gid, ar_gid, 6);
		memcpy(header.ar_mode, ar_mode, 8);
		memcpy(header.ar_size, ar_size, 10);
		memcpy(header.ar_fmag, ARFMAG, SARMAG);

		/*write contents of header into archive*/		
		write(fd, (char*) &header, sizeof(header));

		/*must create new one every time since file sizes will be different*/
		char entire_file_contents[stats.st_size];		

		/*write entire file to archive*/
		read(current, entire_file_contents, sizeof(entire_file_contents));
		write(fd, entire_file_contents, sizeof(entire_file_contents));

		/*if the size is odd, make it even*/
		if (stats.st_size % 2 == 1) {
			write(fd, "\n", 1);
		}
		
		close(current);	
	}


	/*close when we're done*/
	close(fd);
}

void extract(char **argv) {
	int fd, x, y, size = sizeof(struct ar_hdr), length;
	struct ar_hdr header;
	char name[15];

	if (!is_correct_extension(argv))
		return;
	
	/*open archive*/
	fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		printf("Error opening %s.\n", argv[1]);
		return;
	}

	/*skip beginning string*/
	x = lseek(fd, SARMAG, SEEK_SET);

	if (x == -1) {
		printf("Error seeking beginning of archive.\n");
		return;
	}
	
	/*read in all files*/

	while ((read(fd, (char*) &header, size)) == size) {
		sscanf(header.ar_name, "%s", name);	
		printf("Extracting %s...\n", name);

		/*get length of file*/
		length = atoi(header.ar_size);

		/*read in contents of file to variable*/
		char entire_file_contents[length];

		x = read(fd, entire_file_contents, length);

		if (x == -1) {
			printf("Error extracting %s.\n", header.ar_name);
			return;
		}
	
		/*write contents to file*/
		x = open(name, O_CREAT | O_WRONLY, 0666);

		if (x == -1) {
			printf("Error creating file %s.\n", header.ar_name);
			return;
		}

		y = write(x, entire_file_contents, length);

		if (y == -1) {
			printf("Error creating contents of %s.\n", header.ar_name);
			return;
		}

		close(x);
	}

	close(fd);
}

void table_of_contents(char **argv, char option) {
	int fd, length, size = sizeof(struct ar_hdr), uid, gid, mode;
	char name[15], time[50];
	struct ar_hdr header;

	if (!is_correct_extension(argv))
		return;

	/*open archive*/
	fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		printf("Error opening %s.\n", argv[1]);
	}

	/*skip string at beginning*/
	lseek(fd, SARMAG, SEEK_SET);

	printf("FILES IN ARCHIVE FILE %s:\n", argv[1]);

	while ((read(fd, (char*) &header, size)) == size) {
		/*print name regardless of option*/
		sscanf(header.ar_name, "%s", name);	
		printf("FILE NAME: %s\n", name);
	
		/*print other info is option is v*/
		if (option == 'v') {
			time_t file_times;
			struct tm *laymans_time;

			file_times = atoi(header.ar_date);
			laymans_time = localtime(&file_times);

			strftime(time, 50, "%b %d %R %Y", laymans_time);

			sscanf(header.ar_size, "%d", &length);
			sscanf(header.ar_gid, "%d", &gid);
			sscanf(header.ar_uid, "%d", &uid);
			sscanf(header.ar_mode, "%o", &mode);

			printf("CREATED: %s\n", time);

			printf("SIZE: %d | MODE: %d | UID: %d | GID: %d\n", length, mode, uid, gid);
		}
	
		printf("\n\n");

		/*skip to next file. must set length again in case option is r and not v*/
		length = atoi(header.ar_size);
		length += (length % 2);

		/*skip file contents and go to next header if applicable*/
		lseek(fd, length, SEEK_CUR);
	}

	close(fd);
}

int file_exists(int argc, char **argv, char *file) {
	int i;

	for (i = 3; i < argc; i++) {
		if (!strcmp(argv[i], file))
			return 1;
	}

	return 0;
}

void delete_files(int argc, char **argv) {
	int fd, new_archive, x, size = sizeof(struct ar_hdr), length, i;
	char filename[15];
	struct ar_hdr header;

	if (!is_correct_extension(argv))
		return;

	fd = open(argv[1], O_RDONLY);
	new_archive = open("temp.a", O_CREAT | O_WRONLY, 0666);

	if (fd == -1 || new_archive == -1) {
		printf("Error opening archive.\n");
		return;
	}

	x = write(new_archive, ARMAG, SARMAG);

	if (x == -1) {
		printf("Error writing to new archive file.\n");
		return;
	}

	lseek(fd, SARMAG, SEEK_SET);
	lseek(new_archive, SARMAG, SEEK_SET);
	
	while ((read(fd, (char*) &header, size)) == size) {
		sscanf(header.ar_name, "%s", filename);
		if (file_exists(argc, argv, filename)) {
			printf("Deleting %s...\n", filename);
			
			length = atoi(header.ar_size);
			length += (length % 2);

			lseek(fd, length, SEEK_CUR);
		}

		else {
			length = atoi(header.ar_size);
			length += (length % 2);

			write(new_archive, (char*) &header, size);
			
			char entire_file_contents[length];
			
			read(fd, entire_file_contents, length);

			write(new_archive, entire_file_contents, length);	
		}
	}

	remove(argv[1]);

	rename("temp.a", argv[1]);

	close(fd);
	close(new_archive);
}

void append_all(int argc, char **argv) {
	int fd, x, y, current;
	DIR *current_directory;
	struct dirent *current_file;
	char ar_name[17], ar_date[13], ar_uid[7], ar_gid[7], ar_mode[8], ar_size[10];

	if (!is_correct_extension(argv))
		return;

	fd = open(argv[1], O_WRONLY);

	if (fd == -1) {
		printf("Error opening archive.\n");
		return;
	}

	/*set current_directory to... current directory*/
	current_directory = opendir("./");
	
	if (current_directory == 0) {
		printf("Error getting contents of current directory.\n");
		return;
	}

	while (current_file = readdir(current_directory)) {
		struct stat stats;

		/*make sure directory is valid and file is regular*/
		if (!strcmp(current_file->d_name, argv[2]) || !strcmp(current_file->d_name, "..") ||
		!strcmp(current_file->d_name, ".") || stat(current_file->d_name, &stats) || !S_ISREG(stats.st_mode)){
			printf("Cannot append %s.\n", current_file->d_name);
		}		
		
		else {	
			/*seek to end of archive*/
			x = lseek(fd, 0, SEEK_END);

			if (x == -1) {
				printf("Error seeking end of file.\n");
				return;
			}

			/*open file*/
			current = open(current_file->d_name, O_RDONLY);

			if (current == -1) {
				printf("Could not open file %s.\n", current_file->d_name);
				return;
			}

			/*declare stats and header*/
			struct stat stats;
			struct ar_hdr header;

			x = fstat(current, &stats);

			if (x == -1) {
				printf("Error stat-ing file %s.\n", current_file->d_name);
				return;
			}

			/*get info from stat struct*/
			snprintf(ar_name, 17, "%-16.16s", current_file->d_name);
			snprintf(ar_date, 13, "%-12u", stats.st_mtime);
	        	snprintf(ar_uid, 7, "%-6u", stats.st_uid);
                	snprintf(ar_gid, 7, "%-6u", stats.st_gid);
	        	snprintf(ar_mode, 9, "%-8o", stats.st_mode);
			snprintf(ar_size, 11, "%-10u", stats.st_size);
		

			/*put info in header*/
			memcpy(header.ar_name, ar_name, 16);
			memcpy(header.ar_date, ar_date, 12);
			memcpy(header.ar_uid, ar_uid, 6);
			memcpy(header.ar_gid, ar_gid, 6);
			memcpy(header.ar_mode, ar_mode, 8);
			memcpy(header.ar_size, ar_size, 10);
			memcpy(header.ar_fmag, ARFMAG, SARMAG);

			/*write contents of header into archive*/		
			write(fd, (char*) &header, sizeof(header));

			/*must create new one every time since file sizes will be different*/
			char entire_file_contents[stats.st_size];		

			/*write entire file to archive*/
			read(current, entire_file_contents, sizeof(entire_file_contents));
			write(fd, entire_file_contents, sizeof(entire_file_contents));

			/*if the size is odd, make it even*/
			if (stats.st_size % 2 == 1) {
				write(fd, "\n", 1);
			}
			
			close(current);
		}
	}

	close(fd);
}

int main(int argc, char **argv) {
	int success = 0;
	char *help = "HELP: \n"
		"Format: archive_file.a -option file1 file2 ...\n"
		"OPTIONS: \n"
		"-q quickly append named files to archive\n"
		"-x extract named files\n"
		"-t print a concise table of contents of the archive\n"
		"-v iff specified with -t, print a verbose table of contents of the archive\n"
		"-d delete named files from the archive\n"
		"-A quickly append all “regular” files in the current directory (except the archive itself)\n";

	if (argc < 3) {
		printf("You did not provide enough arguments!\n");
		return 0;
	}

	if (!strcmp(argv[2], "-q")) {
		quick_append(argc, argv);
		success = 1;
	}
		
	else if (!strcmp(argv[2], "-x")) {
		extract(argv);
		success = 1;
	}

	else if (!strcmp(argv[2], "-t")) {
		table_of_contents(argv, 'r');
		success = 1;
	}

	else if ((!strcmp(argv[2], "-tv")) || (!strcmp(argv[2], "-vt"))) {
		table_of_contents(argv, 'v');
		success = 1;
	}

	else if (!strcmp(argv[2], "-d")) {
		delete_files(argc, argv);
		success = 1;
	}

	else if (!strcmp(argv[2], "-A")) {
		append_all(argc, argv);
		success = 1;
	}
	
	
	if (!success)
		fprintf(stderr, help);
	
	return 0;
}
