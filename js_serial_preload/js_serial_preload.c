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
#include <unistd.h>

// These macros can't be used from <linux/joystick.h> and <linux/input.h>, because
// those pull in <sys/ioctl.h>, whose ioctl() prototype conflicts with ours.

#define JSIOCGNAME(len) _IOC(_IOC_READ, 'j', 0x13, len) // get identifier string
#define EVIOCGNAME(len) _IOC(_IOC_READ, 'E', 0x06, len) // get device name
#define EVIOCGUNIQ(len) _IOC(_IOC_READ, 'E', 0x08, len) // get unique identifier (serial number)


char *get_js_id(const char *path);



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

	if ((joysticks = (struct jsinfo *)calloc(sizeof(struct jsinfo), n + 1)) == NULL) {
		perror("calloc");
		return;
	}

	char path[PATH_MAX] = "/dev/input/by-id/"; // 17 bytes
	for (int i = 0; n--; ) {
		strncpy(path + 17, files[n]->d_name, PATH_MAX - 17);
		path[PATH_MAX - 1] = '\0';
		free(files[n]);

		struct stat st;
		if (stat(path, &st) < 0) {
			perror("stat");
			continue;
		}

		joysticks[i].dev = st.st_dev;
		joysticks[i].ino = st.st_ino;

		size_t len = strlen(path) - 9;
		strncpy(path + len, "-event-joystick", PATH_MAX - len);
		path[PATH_MAX - 1] = '\0';

		if ((joysticks[i].name = get_js_id(path)) == NULL)
			continue;
#ifdef DEBUG
		fprintf(stderr, "JS '%s' -> '%s'(%d) [%ld:%ld]\n", path, joysticks[i].name,
				(int)strlen(joysticks[i].name) + 1, (long)st.st_dev, (long)st.st_ino);
#endif
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



int ioctl(int fd, unsigned long request, void *data)
{
	static int (*_ioctl)(int fd, unsigned long request, void *data) = NULL;

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
	if (!joysticks || (request & ~IOCSIZE_MASK) != JSIOCGNAME(0))
		return ret;

	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("fstat");
		return ret;
	}

	int size = _IOC_SIZE(request);
#ifdef DEBUG
	fprintf(stderr, "JSIOCGNAME(%d) = '%s'(%d) [%ld:%ld]\n", size,
			(char *)data, ret, (long)st.st_dev, (long)st.st_ino);
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
	return ret;
}



char *get_js_id(const char *path)
{
	int fd = open(path, O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		return NULL;
	}

	char name[256], uniq[256], *id = NULL;

	if (ioctl(fd, EVIOCGNAME(256), name) < 0) {
		perror("ioctl/EVIOCGNAME");

	} else if (ioctl(fd, EVIOCGUNIQ(256), uniq) < 0) {
		perror("ioctl/EVIOCGUNIQ");

	} else if ((id = (char *)malloc(strlen(name) + strlen(uniq) + 2)) == NULL) {
		perror("malloc");

	} else {
		strcpy(id, name);
		if (name[0] && uniq[0])
			strcat(id, " ");
		strcat(id, uniq);
	}

	if (close(fd) < 0)
		perror("close");
	return id;
}
