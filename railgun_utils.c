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
