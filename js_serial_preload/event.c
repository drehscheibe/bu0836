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
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



// dest must be of at least size 512
int get_js_name(const char *path, char *dest)
{
	int fd = open(path, O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	int ret = 0;
	char name[256], uniq[256];

	if (ioctl(fd, EVIOCGNAME(256), name) < 0) {
		perror("ioctl/EVIOCGNAME");
		ret = -2;
 	} else if (ioctl(fd, EVIOCGUNIQ(256), uniq) < 0) {
		perror("ioctl/EVIOCGUNIQ");
		ret = -3;
	} else {
		strcpy(dest, name);
		if (name[0] && uniq[0])
			strcat(dest, " ");
		strcat(dest, uniq);
	}

	if (close(fd) < 0)
		perror("close");
	return ret;
}
