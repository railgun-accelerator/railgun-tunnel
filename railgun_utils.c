/*
 * railgun_utils.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#include <railgun_common.h>
#include <railgun_utils.h>

int set_non_blocking(int sockfd) {
    if (fcntl(sockfd, F_SETFL,
        fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int map_from_file(const char* filename, void ** pdata_buffer,
		int *plength) {
	int data_fd = open(argv[2], O_RDONLY);
	if (data_fd < 0) {
		printf("open data file failed : %s", strerror(errno));
		return -1;
	}
	filelength = lseek(data_fd, 1, SEEK_END);
	lseek(data_fd, 0, SEEK_SET);
	*pdata_buffer = mmap(NULL, *plength, PROT_READ, MAP_PRIVATE,
			data_fd, 0);
	return data_fd;
}

void unmap_from_file(int fd, void* data_buffer, int length) {
	close(fd);
	munmap(data_buffer, length);
}
