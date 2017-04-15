/*
 *  PPM to TX
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
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


#ifndef __ppm__
#define __ppm__

#define VERSION			"0.3.7"

#define PORT			2600

#define SETTINGS_FILE		".txppm"

#define SERVER_INFO		"txppm.spirit-system.com"
#define SERVER_INFO_PORT	2601

extern int c[12];
extern int app_exit;
extern char *get_audio_name (int id);
extern int list_audio ();
extern int init_audio (int dev);
extern int close_audio ();
extern int setup_audio (int dev);
extern int init_sig (void);
extern int client_connect (char *address, int port);
extern void ppm_decode (int fd, int mix);

#endif
