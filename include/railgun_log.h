/*
 * railgun_log.h
 *
 *  Created on: 2014年12月11日
 *      Author: mabin
 */

#ifndef RAILGUN_LOG_H_
#define RAILGUN_LOG_H_

#ifndef NDEBUG
#define LOGI(...) fprintf(stderr, __VA_ARGS__)
#define LOGE(...) fprintf(stdout, __VA_ARGS__)
#else
#define LOGI(...)
#define LOGE(...)
#endif

#endif /* RAILGUN_LOG_H_ */
