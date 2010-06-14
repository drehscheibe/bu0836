// js_serial_preload
//
// Copyright (C) 2010  Melchior FRANZ  <melchior.franz@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



struct jsinfo {
	dev_t dev;
	ino_t ino;
	char *name;
} *joysticks = NULL;



int is_joystick_file(const struct dirent *d)
{
	size_t len = strlen(d->d_name);
	if (len < 13 || strncmp(d->d_name, "usb-", 4) || strcmp(d->d_name + len - 9, "-joystick"))
		return 0;
	if (len > 15 && !strcmp(d->d_name + len - 15, "-event-joystick"))
		return 0;
	return 1;
}



void __attribute__((constructor)) js_preload_begin(void)
{
	struct dirent **files;
	int n = scandir("/dev/input/by-id/", &files, is_joystick_file, alphasort);
	if (n < 0) {
		perror("scandir");
		return;
	}

	// empty entry as stop condition
	joysticks = (struct jsinfo *)calloc(sizeof(struct jsinfo), n + 1);
	if (!joysticks) {
		perror("calloc");
		return;
	}

	char path[PATH_MAX] = "/dev/input/by-id/"; // 17 bytes
	for (int i = 0; n--; ) {
		strncpy(path + 17, files[n]->d_name, PATH_MAX - 17);

		struct stat st;
		if (stat(path, &st) < 0) {
			perror("stat");
			continue;
		}

		// extract name from by-id link name
		size_t len = strlen(files[n]->d_name);
		joysticks[i].name = (char *)malloc(len - 12);
		strncpy(joysticks[i].name, files[n]->d_name + 4, len - 13);
		joysticks[i].name[len - 13] = '\0';
		joysticks[i].dev = st.st_dev;
		joysticks[i].ino = st.st_ino;

		char *s;
		while ((s = index(joysticks[i].name, '_')) != NULL)
			*s = ' ';
#ifdef DEBUG
		fprintf(stderr, "JS '%s' -> '%s' (%ld:%ld)\n", path, joysticks[i].name,
				(long)st.st_dev, (long)st.st_ino);
#endif
		free(files[n]);
		i++;
	}
	free(files);
}



void __attribute__((destructor)) js_preload_end(void)
{
	if (joysticks) {
		for (struct jsinfo *js = joysticks; js->name; js++)
			free(js->name);
		free(joysticks);
	}
}



int ioctl(int fd, unsigned long int request, void *data)
{
	static int (*_ioctl)(int fd, unsigned long int request, void *data) = NULL;

	if (!_ioctl) {
		dlerror();
		_ioctl = dlsym(RTLD_NEXT, "ioctl");
		char *error = dlerror();
		if (error) {
			fprintf(stderr, "%s\n", error);
			exit(1);
		}
	}

	int ret = _ioctl(fd, request, data);

	if ((request & ~IOCSIZE_MASK) == _IOC(_IOC_READ, 'j', 0x13, 0)) {
		struct stat st;
		if (fstat(fd, &st) < 0) {
			perror("fstat");
			goto out;
		}

		int size = _IOC_SIZE(request);
#ifdef DEBUG
		fprintf(stderr, "JSIOCGNAME(%d) = '%s' %ld:%ld <- %d\n", size,
				(char *)data, (long)st.st_dev, (long)st.st_ino, ret);
#endif
		for (struct jsinfo *js = joysticks; js->name; js++) {
			if (js->dev == st.st_dev && js->ino == st.st_ino) {
				ret = (int)strlen(js->name) + 1;
				if (ret > size)
					return -EFAULT;

				strcpy(data, js->name);
				break;
			}
		}
	}

out:
	return ret;
}
