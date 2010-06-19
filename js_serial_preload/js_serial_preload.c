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
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/major.h>
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

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X
#define ORIGIN __FILE__":"STRINGIZE(__LINE__)": "

#ifdef DEBUG
#  define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#  define DBG(...)
#endif


int ioctl(int fd, unsigned long request, void *data);
int (*sys_ioctl)(int fd, unsigned long request, void *data) = NULL;


struct jsinfo {
	int num;
	char *name;
} *joysticks = NULL;



static char *get_event_id(const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror(ORIGIN"open");
		return NULL;
	}

	char name[256], uniq[256], *id = NULL;

	if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
		perror(ORIGIN"ioctl/EVIOCGNAME");

	} else if (ioctl(fd, EVIOCGUNIQ(sizeof(uniq)), uniq) < 0) {
		perror(ORIGIN"ioctl/EVIOCGUNIQ");

	} else if ((id = (char *)malloc(strlen(name) + strlen(uniq) + 2)) == NULL) {
		perror(ORIGIN"malloc");

	} else {
		strcpy(id, name);
		if (name[0] && uniq[0])
			strcat(id, " ");
		strcat(id, uniq);
	}

	if (close(fd) < 0)
		perror(ORIGIN"close");
	return id;
}



static int get_js_number(const struct stat *s)
{
	if (!(s->st_mode & S_IFCHR))
		return -1;

	int maj = major(s->st_rdev);
	int min = minor(s->st_rdev);

	if (maj == INPUT_MAJOR && min < 32)
		return min;
	if (maj == JOYSTICK_MAJOR)
		return min;
	return -1;
}



static int is_js_file(const struct dirent *d)
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
	dlerror();
	sys_ioctl = dlsym(RTLD_NEXT, "ioctl");
	char *error = dlerror();
	if (error) {
		fprintf(stderr, __FILE__": %s\n", error);
		exit(EXIT_FAILURE);
	}

	struct dirent **files;
	int n = scandir("/dev/input/by-id/", &files, is_js_file, alphasort);
	if (n < 0) {
		perror(ORIGIN"scandir");
		return;
	}

	if ((joysticks = (struct jsinfo *)calloc(sizeof(struct jsinfo), n + 1)) == NULL) {
		perror(ORIGIN"calloc");
		return;
	}

	char path[PATH_MAX] = "/dev/input/by-id/"; // 17 bytes
	for (int i = 0; n--; ) {
		strncpy(path + 17, files[n]->d_name, PATH_MAX - 17);
		path[PATH_MAX - 1] = '\0';
		free(files[n]);

		struct stat st;
		if (stat(path, &st) < 0) {
			perror(ORIGIN"stat");
			continue;
		}

		if ((joysticks[i].num = get_js_number(&st)) < 0)
			continue;

		size_t len = strlen(path) - 9;
		strncpy(path + len, "-event-joystick", PATH_MAX - len);
		path[PATH_MAX - 1] = '\0';

		if ((joysticks[i].name = get_event_id(path)) == NULL)
			continue;

		DBG(__FILE__": '%s'  <-  #%u '%s'(%d)\n", path, joysticks[i].num,
				joysticks[i].name, (int)strlen(joysticks[i].name) + 1);
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
	int ret = sys_ioctl(fd, request, data);
	if ((request & ~IOCSIZE_MASK) != JSIOCGNAME(0) || !joysticks)
		return ret;

	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror(ORIGIN"fstat");
		return ret;
	}

	int num, size = _IOC_SIZE(request);
	if ((num = get_js_number(&st)) < 0)
		return ret;

	DBG(__FILE__": JSIOCGNAME(%d) = #%u '%s'(%d)", size, num, (char *)data, ret);
	for (struct jsinfo *js = joysticks; js->name; js++) {
		if (js->num == num) {
			int len = (int)strlen(js->name);
			if (len < size) {
				strcpy(data, js->name);
				ret = ++len;
				DBG("  ->  '%s'(%d)", (char *)data, ret);
			}
			break;
		}
	}
	DBG("\n");
	return ret;
}
