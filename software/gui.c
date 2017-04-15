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


#include <gtk/gtk.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WIN32__
#include "lib/pthread.h"
#endif
#include "ppm.h"

/* GtkWidget is the storage type for widgets */
GtkWidget *window;
GtkWidget *mix;
GtkWidget *device;
GtkWidget *button;
GtkWidget *pbar_ch[6];
GtkWidget *channels;
GtkWidget *frame3;

int num_devs = 0;
int fd;
int decode = 0;
int param[2];
int *devices;

pthread_t thread;

static gboolean progress_timeout (gpointer data)
{
	float ch[6];
	int i;
	for (i = 0; i < 6; i ++) {
		ch[i] = (float) (c[i]+512)/1000;
		
		if (ch[i] < 0)
			ch[i] = 0;
		else if (ch[i] > 1)
			ch[i] = 1;
		/* Set the new value */
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar_ch[i]), ch[i]);
	}
  
	/* As this is a timeout function, return TRUE so that it
	* continues to get called */
	return TRUE;
} 

static void thread_decode (void *ptr)
{
	int *param = (int *) ptr;
      
	decode = 1;
	ppm_decode (param[0], param[1]);
	decode = 0;
	gtk_button_set_label (GTK_BUTTON (button), "Start");
	
	pthread_exit (0); /* exit */
}

static void stop ()
{
	device_close (fd);
	close_audio ();
	printf ("> Closing audio\n"); 
}

static int settings_save (int dev, int mixer, int chshow)
{
	/** WRITE SETTINGS **/
#ifndef __WIN32__
	const char *home = getenv ("HOME");
	
	if (!home)
		return -1;

	unsigned l = strlen (home);
	unsigned l2 = strlen (SETTINGS_FILE);
	char *s = (char *) malloc (l+l2+2);
	
	if (!s)
		return -1;

	memcpy (s, home, l);
	s[l] = '/';
	memcpy (s+l+1, SETTINGS_FILE, l2);
	s[l+l2+1] = '\0';
	
	FILE *f = fopen (s, "w+");
#else
	char *s = (char *) malloc (65);
	
	if (!s)
		return -1;
	
	FILE *f = fopen (SETTINGS_FILE, "w+");
#endif
	if (f) {
		sprintf (s, "%d %d %d", dev, mixer, chshow);
		fwrite (s, strlen (s), 1, f);
		fclose (f);
	} else {
		free (s);
		return -1;
	}
	
	free (s);

	return 0;
}

static int settings_load (int *dev, int *mixer, int *chshow)
{
	/** WRITE SETTINGS **/
#ifndef __WIN32__
	const char *home = getenv ("HOME");
	
	if (!home)
		return -1;

	unsigned l = strlen (home);
	unsigned l2 = strlen (SETTINGS_FILE);
	char *s = (char *) malloc (l+l2+2);
	
	if (!s)
		return -1;

	memcpy (s, home, l);
	s[l] = '/';
	memcpy (s+l+1, SETTINGS_FILE, l2);
	s[l+l2+1] = '\0';

	FILE *f = fopen (s, "r");
#else
	char *s = (char *) malloc (65);
	
	if (!s)
		return -1;
	
	FILE *f = fopen (SETTINGS_FILE, "r");
#endif
	if (f) {
		fgets (s, 64, f);
		sscanf (s, "%d %d %d", dev, mixer, chshow);
		fclose (f);
	} else {
		free (s);
		return -1;
	}

	free (s);

	return 0;
}

/* This is a callback function. The data arguments are ignored
 * in this example. More on callbacks below. */
static void start (GtkWidget *widget, gpointer data )
{
	if (decode) {
		g_print ("Stopping decoding\n");
		app_exit = 1;
		stop ();
		gtk_main_quit ();
		return;
	}
	
	g_print ("Starting decoding\n");
	
	int dev = gtk_combo_box_get_active (GTK_COMBO_BOX (device));
	
	if (dev == -1)
		return;
	
	fd = device_open ();

	if (fd < 0) {
		gtk_main_quit ();
		return;
	}

	int r = init_audio (devices[dev]);
  
	if (r == -1) {
		printf ("ERROR -> You've selected wrong input device ID, choose one from the list\n");
		return;
	}
	
	printf ("> Audio initialization -> OK\n");
	
	r = setup_audio (devices[dev]);
	
	if (r == -1) {
		printf ("ERROR -> Audio setup -> FAIL : DeviceID(%d)\n", devices[dev]);
		return;
	}
	
	int mixer = gtk_combo_box_get_active (GTK_COMBO_BOX (mix));

	app_exit = 0;

	param[0] = fd;
	param[1] = mixer;
	
	int chshow = 0;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (channels)) == TRUE) {
		g_timeout_add (40, progress_timeout, NULL);
		gtk_widget_show (pbar_ch[0]);
		gtk_widget_show (pbar_ch[1]);
		gtk_widget_show (pbar_ch[2]);
		gtk_widget_show (pbar_ch[3]);
		gtk_widget_show (pbar_ch[4]);
		gtk_widget_show (pbar_ch[5]);
		gtk_widget_hide (channels);
		chshow = 1;
	} else
		gtk_widget_hide (frame3);
	
	gtk_button_set_label (GTK_BUTTON (button), "Stop");
	pthread_create (&thread, NULL, (void *) &thread_decode, (void *) &param);
	
	/** SAVE SETTINGS **/
	settings_save (devices[dev], mixer, chshow);
}

static gboolean delete_event( GtkWidget *widget, GdkEvent  *event, gpointer data )
{
	g_print ("delete event occurred\n");

	free (devices);
	/* Change TRUE to FALSE and the main window will be destroyed with
	* a "delete-event". */

	return FALSE;
}

/* Another callback */
static void destroy (GtkWidget *widget,  gpointer data)
{
	gtk_main_quit ();
}

void outdate_version (char *ver)
{
	gdk_threads_enter ();
	gtk_window_set_title (GTK_WINDOW (window), "(OUTDATE) TXPPM - ppm2tx v." VERSION);
	gdk_threads_leave ();
	
	printf ("> Your version is outdate ! Please update to %s\n", ver);
}

int main (int argc, char *argv[])
{
	printf ("TXPPM - ppm2tx v." VERSION " by Tomas Jedrzejek - ZeXx86\n");

	int dev = -1, mixer = -1, chshow = 1;
	/** LOAD SETTINGS **/
	settings_load (&dev, &mixer, &chshow);
	
	/* This is called in all GTK applications. Arguments are parsed
	* from the command line and are returned to the application. */
	gtk_init (&argc, &argv);
	
	/* create a new window */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title (GTK_WINDOW (window), "TXPPM - ppm2tx v." VERSION);

	/* When the window is given the "delete-event" signal (this is given
	* by the window manager, usually by the "close" option, or on the
	* titlebar), we ask it to call the delete_event () function
	* as defined above. The data passed to the callback
	* function is NULL and is ignored in the callback function. */
	g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);
	
	/* Here we connect the "destroy" event to a signal handler.  
	* This event occurs when we call gtk_widget_destroy() on the window,
	* or if we return FALSE in the "delete-event" callback. */
	g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);
	
	/* Sets the border width of the window. */
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	
	/** VBOX **/
	GtkWidget *box1;
	box1 = gtk_vbox_new (FALSE, 0);

	/* Put the box into the main window. */
	gtk_container_add (GTK_CONTAINER (window), box1);
	
	/** IMAGE **/
	GtkWidget *image;
#ifndef __WIN32__
	GError **error;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("/usr/share/txppm/logo.png", error);
	if (!pixbuf)
		pixbuf = gdk_pixbuf_new_from_file ("/usr/local/share/txppm/logo.png", error);
	if (!pixbuf)
		pixbuf = gdk_pixbuf_new_from_file ("logo.png", error);

	if (pixbuf) {
		image = gtk_image_new_from_pixbuf (pixbuf);
		gtk_box_pack_start (GTK_BOX (box1), image, FALSE, FALSE, 0);
	}
#else
	int pixbuf = 1;
	image = gtk_image_new_from_file ("logo.png");
	gtk_box_pack_start (GTK_BOX (box1), image, FALSE, FALSE, 0);
#endif
	/** FRAME **/
	GtkWidget *frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);
	/* Set the frame's label */
	gtk_frame_set_label (GTK_FRAME (frame), "Sound device");

	/** DEVICE **/
	device = gtk_combo_box_new_text ();

	num_devs = list_audio ();

	char *s;
	int i;
	int n = 0;

	devices = (int *) malloc (sizeof (int) * num_devs);

	if (!devices)
		return -1;

	for (i = 0; i < num_devs; i ++) {
		s = get_audio_name (i);

		if (s) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (device), s);
			devices[n] = i;
#ifdef DEBUG
			printf ("d: %d : %d : %s\n", n, i, s);
#endif
			n ++;
		}
	}

	if (argc > 1)
		gtk_combo_box_set_active (GTK_COMBO_BOX (device), atoi (argv[1]));
	else
		gtk_combo_box_set_active (GTK_COMBO_BOX (device), dev == -1 ? n-1 : dev);
	
	gtk_container_add (GTK_CONTAINER (frame), device);

	/** FRAME2 **/
	GtkWidget *frame2 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (box1), frame2, FALSE, FALSE, 0);
	/* Set the frame's label */
	gtk_frame_set_label (GTK_FRAME (frame2), "CCPM (mix)");
	
	/** MIX **/
	mix = gtk_combo_box_new_text ();
	
	gtk_combo_box_append_text (GTK_COMBO_BOX (mix), "No mix (H1)");
	gtk_combo_box_append_text (GTK_COMBO_BOX (mix), "CCPM 120°");
	gtk_combo_box_append_text (GTK_COMBO_BOX (mix), "CCPM 120° (Spektrum)");
	
	if (argc > 2)
		gtk_combo_box_set_active (GTK_COMBO_BOX (mix), atoi (argv[2]));
	else
		gtk_combo_box_set_active (GTK_COMBO_BOX (mix), mixer == -1 ? 0 : mixer);

	gtk_container_add (GTK_CONTAINER (frame2), mix);

	/** FRAME3 **/
	frame3 = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX(box1), frame3, FALSE, FALSE, 0);
	/* Set the frame's label */
	gtk_frame_set_label (GTK_FRAME (frame3), "Channels");
	
	/** HBOX **/
	GtkWidget *box2;
	box2 = gtk_hbox_new (FALSE, 0);

	gtk_container_add (GTK_CONTAINER (frame3), box2);
	
	channels = gtk_check_button_new_with_label ("Display");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (channels), chshow ? TRUE : FALSE);

	gtk_box_pack_start (GTK_BOX (box2), channels, TRUE, FALSE, 0);
	/** CHANNELS **/
	int l;
	for (l = 0; l < 6; l ++) {
		pbar_ch[l] = gtk_progress_bar_new ();
		gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (pbar_ch[l]), GTK_PROGRESS_BOTTOM_TO_TOP);
		gtk_box_pack_start (GTK_BOX (box2), pbar_ch[l], TRUE, FALSE, 0);
	}
	/** BUTTON **/
	/* Creates a new button with the label "Hello World". */
	button = gtk_button_new_with_label ("Start");

	/* When the button receives the "clicked" signal, it will call the
	* function start() passing it NULL as its argument.  The start()
	* function is defined above. */
	g_signal_connect (button, "clicked", G_CALLBACK (start), NULL);
	
	/* This packs the button into the window (a gtk container). */
	gtk_box_pack_start (GTK_BOX (box1), button, FALSE, FALSE, 5);
	
	/* The final step is to display this newly created widget. */
	if (pixbuf)
		gtk_widget_show (image);
	gtk_widget_show (device);
	gtk_widget_show (mix);
	gtk_widget_show (button);
	gtk_widget_show (channels);
	gtk_widget_show (frame3);
	gtk_widget_show (frame2);
	gtk_widget_show (frame);
	gtk_widget_show (box2);
	gtk_widget_show (box1);
	gtk_widget_show (window);

	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

	gdk_threads_init ();
	
	check_version (&outdate_version);
	
	/* All GTK applications must have a gtk_main(). Control ends here
	* and waits for an event to occur (like a key press or
	* mouse event). */
	gtk_main ();

	return 0;
}
