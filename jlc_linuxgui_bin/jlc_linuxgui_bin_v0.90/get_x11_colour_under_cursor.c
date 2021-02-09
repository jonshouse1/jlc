// get_x11_colour_under_cursor.c
// Return the RGB value of the pixel under pixel x,y

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdlib.h>


void getcolor_under_cursor(int x, int y, unsigned int* R, unsigned int* G, unsigned int *B)
{
	XColor c;
	static Display *d = NULL;
	XImage *image;

	if (d==NULL)
	{
		d=XOpenDisplay((char *) NULL);
		if (d==NULL)
			exit(1);
	}


	image = XGetImage (d, XRootWindow (d, XDefaultScreen (d)), x, y, 1, 1, AllPlanes, XYPixmap);
	c.pixel = XGetPixel (image, 0, 0);
	XFree (image);
	XQueryColor (d, XDefaultColormap(d, XDefaultScreen (d)), &c);
	*R=c.red/256;
	*G=c.green/256;
	*B=c.blue/256;
}

