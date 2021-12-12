#ifndef _BLUSHER_UTILS_H
#define _BLUSHER_UTILS_H

#include <stdlib.h>

double pixel_to_pango_size(double pixel);

int set_cloexec_or_close(int fd);

int create_tmpfile_cloexec(char *tmpname);

int os_create_anonymous_file(off_t size);

#endif /* _BLUSHER_UTILS_H */
