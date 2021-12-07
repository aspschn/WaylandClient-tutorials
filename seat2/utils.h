#ifndef _UTILS_H
#define _UTILS_H

#include <stdlib.h>

int set_cloexec_or_close(int fd);

int create_tmpfile_cloexec(char *tmpname);

int os_create_anonymous_file(off_t size);

#endif /* _UTILS_H */
