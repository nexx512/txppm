/*
 *  PPM to TX Terminal program
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2017  Jürgen Diez (jdiez@web.de)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include "ppm.h"
#include "sys.h"

int main (int argc, char **argv)
{
	printf ("TxPPM - ppm2tx v." VERSION " by Jürgen Diez\n");
	printf ("This program comes with ABSOLUTELY NO WARRANTY; "
		"for details see the LICENSE shipped with this software.\n"
		"This is free software, and you are welcome to redistribute it "
		"under certain conditions; check the LICENSE file shipped with "
		"this software for details.");

	if (argc < 3) {
		printf ("Syntax: %s <deviceid> <mixing>\n\nList of available devices:\n", argv[0]);

		int n = list_audio ();

		int i;
		for (i = 0; i < n; i ++) {
			char *s = get_audio_name (i);

			if (s)
				printf("  DeviceID (%d) name: %s.\n", i, s);
		}

		printf("\nAvailable Mixings: \n  0  No mix\n  1  CCPM 120°\n  2  CCPM 120° (Spektrum)\n");

		return 0;
	}

	int fd = 0;

	fd = device_open ();

	if (fd < 0)
		return -1;

	int mix = atoi (argv[2]);

	if (mix)
		printf ("> Transmitter mixing-filter enabled\n");

	int dev = atoi (argv[1]);

	if (dev < 0)
		goto end;

	int r = init_audio (dev);

	if (r == -1) {
		printf ("ERROR -> You've selected wrong input device ID, choose one from the list\n");
		goto end;
	}

	printf ("> Audio initialization -> OK\n");

	r = setup_audio (dev);

	if (r == -1) {
		printf ("ERROR -> Audio setup -> FAIL : DeviceID(%d)\n", dev);
		goto end;
	}

	app_exit = 0;

#ifndef __WIN32__
	init_sig ();
#endif
	ppm_decode (fd, mix);
end:
	device_close (fd);

	close_audio ();

	printf ("> Closing audio\n");

	return 0;
}
