/*      fileselector-bg - A black window. Ugh.
 *      
 *      Copyright 2010 Alex Ferguson <thoughtmonster@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <X11/Xlib.h>

int main(void)
{
	Display *display;
	Window window;
	XEvent event;

	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Error: Cannot open display!\n");
		return 1;
	}

	int width = XWidthOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
	int height = XHeightOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
	int blackColor = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(display, DefaultRootWindow(display), 
				0, 0, width, height, 0, blackColor, blackColor);

	Atom delwindow = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, window, &delwindow, 1);

	XMapWindow(display, window);

	for(;;) {
		XNextEvent(display, &event);
		if (event.type == ClientMessage)
			break;
	}

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}
