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
#include <fcntl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static int is_js_file(const struct dirent *d)
{
	unsigned u;
	return sscanf(d->d_name, "js%u", &u) == 1;
}



int main(void)
{
	struct dirent **joysticks;
	int n = scandir("/dev/input/", &joysticks, is_js_file, alphasort);
	if (n < 0) {
		perror("scandir /dev/input/");
	} else {
		int i;
		for (i = 0; i < n; i++) {
			char path[PATH_MAX];
			snprintf(path, PATH_MAX, "/dev/input/%s", joysticks[i]->d_name);
			free(joysticks[i]);

			int fd = open(path, O_RDONLY);
			if (fd < 0) {
				perror(path);
				continue;
			}

			unsigned char numaxes, numbuttons;
			ioctl(fd, JSIOCGAXES, &numaxes);
			ioctl(fd, JSIOCGBUTTONS, &numbuttons);

			char name[256];
			if (ioctl(fd, JSIOCGNAME(sizeof(name)), name) < 0)
				perror("ioctl/EVIOCGNAME");
			else
				fprintf(stderr, "%s:\t\"%s\"  (%d axes, %d buttons)\n",
						path, name, (int)numaxes, (int)numbuttons);

			if (close(fd) < 0)
				perror("close");
		}
		free(joysticks);
	}
	return 0;
}
