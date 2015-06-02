/*
 * libslp-setting
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hakjoo Ko <hakjoo.ko@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VCONF_BUXTON_LOG_H__
#define __VCONF_BUXTON_LOG_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


/************** Error ***************/
#ifdef VCONF_SYSLOG_OUT
#include <syslog.h>

#define INFO(fmt, arg...) \
		do { \
			syslog(LOG_INFO, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
#define ERR(fmt, arg...) \
		do { \
			syslog(LOG_ERR, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
#define WARN(fmt, arg...) \
		do { \
			syslog(LOG_ERR, "[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)

#else
#include <stdlib.h>

#define INFO(fmt, arg...) \
		do { \
			fprintf(stdout,"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
#define WARN(fmt, arg...) \
		do { \
			fprintf(stderr,"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
#define ERR(fmt, arg...) \
		do { \
			fprintf(stderr,"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
		}while(0)
#endif


/************** Return ***************/
#define ret_if(expr) \
	do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#define retv_if(expr, val) \
	do { \
		if (expr) { \
			ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)
#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0)
#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0)
#define retex_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			ERR(fmt, ##arg); \
			goto CATCH; \
		} \
	} while (0)


#endif				/* __VCONF_BUXTON_LOG_H__ */
