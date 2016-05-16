/*
 * lalcal.c
 *
 * lalcal; a small clock and calendar for the system tray.
 * Based on lalcal from Tom Cornall <t.cornall@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "calendar.h"

int running = 0;
int windowwidth = 64;
double fontsize = 12.0;
char *fontname = "Mono";
double calfontsize = 14.0;
char *calfontname = "Mono";
char *timeformat = "%T";
char *colorname = "white";
char *bgcolorname = "black";
char *hlcolorname = "grey30";
char curr_cal[8][30];

int glyph_w, glyph_h;

int cal_width; /* width of the calendar window */
struct tm draw_cal;
struct tm *curtime;

char draw_line[50];
char weekdays[7][2];
int startday=1;

unsigned char leftarrow[] = "⇐";
unsigned char rightarrow[] = "⇒";
#define leftarrowlen strlen((char*)leftarrow)
#define rightarrowlen strlen((char*)rightarrow)

#define NUM_OPTS 10
static XrmOptionDescRec opt_table[] = 	{
		{"--font", "*font", XrmoptionSepArg, (XPointer) NULL	},
		{"--fontsize", "*fontsize", XrmoptionSepArg, (XPointer) NULL	},
		{"--calfont", "*calfont", XrmoptionSepArg, (XPointer) NULL	},
		{"--calfontsize", "*calfontsize", XrmoptionSepArg, (XPointer) NULL	},
		{"--color", "*color", XrmoptionSepArg, (XPointer) NULL	},
		{"--hlcolor", "*hlcolor", XrmoptionSepArg, (XPointer) NULL	},
		{"--bgcolor", "*bgcolor", XrmoptionSepArg, (XPointer) NULL	},
		{"--width", "*width", XrmoptionSepArg, (XPointer) NULL	},
		{"--format", "*format", XrmoptionSepArg, (XPointer) NULL	},
		{"--help", "*help", XrmoptionNoArg, (XPointer) ""	},
	};

/*Calculates where to place calendar window*/
void locateCalendar(Display * display, int screen, Window dockapp, Window root, Window calendar)
	{
	Window tmp;
	XWindowAttributes attr;
	int d_x, d_y, d_w, d_h;
	int cal_x = 0, cal_y = 0, cal_w, cal_h;
	int res_w, res_h;

	res_w = XDisplayWidth(display, screen);
	res_h = XDisplayHeight(display, screen);
	XTranslateCoordinates(display, dockapp, root, 0, 0, &d_x, &d_y, &tmp);
	XGetWindowAttributes(display, dockapp, &attr);
	d_w = attr.width;
	d_h = attr.height;
	XGetWindowAttributes(display, calendar, &attr);
	cal_w = attr.width;
	cal_h = attr.height;

	if ((d_x < res_w / 2) && (d_y < res_h / 2))
		{ /*Top left */
		cal_x = d_x;
		cal_y = (d_y + d_h);
		}
	else if ((d_x >= res_w / 2) && (d_y < res_h / 2))
		{ /*Top right */
		cal_x = (d_x + d_w) - cal_w;
		cal_y = (d_y + d_h);
		}
	else if ((d_x < res_w / 2) && (d_y >= res_h / 2))
		{ /*Bottom left */
		cal_x = d_x;
		cal_y = d_y - cal_h;
		}
	else if ((d_x >= res_w / 2) && (d_y >= res_h / 2))
		{ /*Bottom right */
		cal_x = (d_x + d_w) - cal_w;
		cal_y = d_y - cal_h;
		}

	XMoveWindow(display, calendar, cal_x, cal_y);
	}

/** update curtime & format it */
static void gettime(char *strtime, int len)
	{
	time_t t = time(NULL);
	curtime = localtime(&t);
	strftime(strtime, len, timeformat, curtime);
	}

void handle_term(int signal)
	{
	running = 0;
	}

void die(const char *str)
	{
	fprintf(stderr, str);
	fflush(stderr);
	exit(1);
	}

/** parse parameter */
void get_params(Display * display, int argc, char *argv[])
	{
	XrmDatabase database;
	int nargc = argc;
	char **nargv = argv;
	char *type;
	XrmValue xrmval;
	char *xdefaults;

	XrmInitialize();
	database = XrmGetDatabase(display);

	/* merge in Xdefaults */
	xdefaults = malloc(strlen(getenv("HOME")) + strlen("/.Xresources") + 1);
	sprintf(xdefaults, "%s/.Xresources", getenv("HOME"));
	XrmCombineFileDatabase(xdefaults, &database, True);
	free(xdefaults);

	/* parse command line */
	XrmParseCommand(&database, opt_table, NUM_OPTS, "lalcal", &nargc, nargv);

	/* check for help option */
	if (XrmGetResource(database, "lalcal.help", "lalcal.help", &type, &xrmval) == True)
		{
		printf("lalcal v0.1\n");
		printf("Usage: lalcal [options]\n\n");
		printf("Options:\n");
		printf("--help:		This text\n");
		printf("--font:		The font face to use.  Default is Mono.\n");
		printf("--fontsize:	The font size to use.  Default is 12.0.\n");
		printf("--calfont:	 The font face to use for the calendar.  Default is Mono.\n");
		printf("--calfontsize: The font size to use for the calendar.  Default is 14.0.\n");
		printf("--color:	   The font color to use.  Default is white.\n");
		printf("--bgcolor:	 The background color to use.  Default is black.\n");
		printf("--hlcolor:	 The highlight color to use.  Default is grey30.\n");
		printf("--width:	   The width of the dockapp window.  Default is 64.\n");
		printf("--format:	  The time format to use.  Default is %%T.  You can\n");
		printf("			   get format codes from the man page for strftime.\n\n");
		printf("You can also put these in your ~/.Xdefaults file:\n");
		printf("lalcal*font\n");
		printf("lalcal*fontsize\n");
		printf("lalcal*calfont\n");
		printf("lalcal*calfontsize\n");
		printf("lalcal*color\n");
		printf("lalcal*bgcolor\n");
		printf("lalcal*hlcolor\n");
		printf("lalcal*width\n");
		printf("lalcal*format\n\n");
		exit(0);
		}

	/* get options out of it */
	if (XrmGetResource(database, "lalcal.font", "lalcal.font", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"font\" was in an unexpected format!");
		fontname = xrmval.addr;
		}

	if (XrmGetResource(database, "lalcal.fontsize", "lalcal.fontsize", &type,&xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"fontsize\" was in an unexpected format!");
		fontsize = strtod(xrmval.addr, NULL);
		}

	if(XrmGetResource(database, "lalcal.calfont", "lalcal.calfont", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"calfont\" was in an unexpected format!");
		calfontname = xrmval.addr;
		}

	if (XrmGetResource(database, "lalcal.calfontsize", "lalcal.calfontsize", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"calfontsize\" was in an unexpected format!");
		calfontsize = strtod(xrmval.addr, NULL);
		}

	if (XrmGetResource(database, "lalcal.color", "lalcal.color", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"color\" was in an unexpected format!");
		colorname = xrmval.addr;
		}

	if (XrmGetResource(database, "lalcal.bgcolor", "lalcal.bgcolor", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"color\" was in an unexpected format!");
		bgcolorname = xrmval.addr;
		}


	if (XrmGetResource(database, "lalcal.hlcolor", "lalcal.hlcolor", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"hlcolor\" was in an unexpected format!");
		hlcolorname = xrmval.addr;
		}

	if (XrmGetResource(database, "lalcal.width", "lalcal.width", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"width\" was in an unexpected format!");
		windowwidth = strtod(xrmval.addr, NULL);
		}

	if (XrmGetResource(database, "lalcal.format", "lalcal.format", &type, &xrmval) == True)
		{
		if (strcmp(type, "String"))
			die("Option \"format\" was in an unexpected format!");
		timeformat = xrmval.addr;
		}
	}

/** draw the calendar */
void paintCalendar(Display * display, GC gc, Window calendar, XftDraw * caldraw, XftFont * font, XftColor * xftcolor, XftColor * xfthlcolor)
	{
	int i = 0;
	int p_x = 0, p_y = 0;
	int eom=end_of_month(&draw_cal);
	int fdw=draw_cal.tm_wday-startday;
	if(fdw<0)
		fdw+=7;

	if(draw_cal.tm_year==curtime->tm_year&&draw_cal.tm_mon==curtime->tm_mon)
		{
		int curw=curtime->tm_wday-startday;
		if(curw<0)
			curw+=7;

		p_x=curw*glyph_w*3-1;
		i=(curtime->tm_mday+fdw-1)/7+2;
	    p_y=i*glyph_h+8;
	    XftDrawRect(caldraw, xfthlcolor, p_x, p_y, glyph_w*2+6, glyph_h);
		}

	strftime(draw_line, 50, "%B %Y", &draw_cal);
	draw_line[0]=toupper(draw_line[0]);
	i=strlen(draw_line);
	p_x=(cal_width-i*glyph_w)/2;
	p_y=glyph_h;

	XftDrawStringUtf8(caldraw, xftcolor, font, p_x, p_y, (unsigned char*)draw_line, i);
	XftDrawStringUtf8(caldraw, xftcolor, font, 1, p_y, leftarrow, leftarrowlen);
	XftDrawStringUtf8(caldraw, xftcolor, font, glyph_w * 19, p_y, rightarrow, rightarrowlen);

	p_x=2;
	p_y+=glyph_h+2;

	// print weekdays
	i=startday;
	do
		{
		XftDrawStringUtf8(caldraw, xftcolor, font, p_x, p_y, (unsigned char*)weekdays[i], 2);
		p_x+=glyph_w*3;
		i=(i+1)%7;
		}
	while(i!=startday);

	i=1;
	p_x=2+fdw*glyph_w*3;
	p_y+=glyph_h+2;
	while(i<=eom)
		{
		snprintf(draw_line, 50, "%2d", i++);
		XftDrawStringUtf8(caldraw, xftcolor, font, p_x, p_y, (unsigned char*)draw_line, 2);
		p_x+=glyph_w*3;
		if(p_x>cal_width)
			{
			p_x=2;
			p_y+=glyph_h+1;
			}
		}

	XFlush(display);
	}

/** initialize week day */
void initweekdays()
	{
	char buf[50];
	struct tm tm;
	int i;

	for(i=0; i<7; i++)
		{
		tm.tm_wday=i;
		strftime(buf, 50, "%a", &tm);
		weekdays[i][0]=buf[0];
		weekdays[i][1]=buf[1];
		}
	}

int main(int argc, char *argv[])
	{
	/* X stuff */
	Display *display;
	Window dockapp;
	Window calendar;
	GC gc;
	XSetWindowAttributes attributes;
	XWMHints wm_hints;
	XftFont *font;
	XftFont *calfont;
	XftDraw *draw;
	XftDraw *caldraw;
	XftColor xftcolor;
	XftColor xfthlcolor;
	XftColor xftbgcolor;
	XRenderColor color;
	XColor c;			/* ... */
	int screen;
	Colormap colormap;
	Window root;
	Visual *vis;
	XGlyphInfo extents;
	XEvent event;
	int x, y;
	int m_x = 0;

	/* Time stuff */
	int xfd, selectret = 0;
	fd_set fds;
	struct timeval timeout;
	char strtime[50];
	setlocale(LC_ALL, "");

	struct sigaction act;

	act.sa_handler = &handle_term;
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	running = 1;
	int calopen = 0;

	display = XOpenDisplay(NULL);
	if (!display)
		die("Couldn't open X\n");

	/* command line/xresources: sets fonts etc */
	get_params(display, argc, argv);

	xfd = ConnectionNumber(display);
	if (xfd == -1)
		die("Couldn't get X filedescriptor\n");

	screen = DefaultScreen(display);
	colormap = DefaultColormap(display, screen);
	root = DefaultRootWindow(display);
	vis = DefaultVisual(display, screen);
	dockapp = XCreateSimpleWindow(display, root, 0, 0, windowwidth, 24, 0, 0, 0);

	wm_hints.initial_state = WithdrawnState;
	wm_hints.icon_window = wm_hints.window_group = dockapp;
	wm_hints.flags = StateHint | IconWindowHint;
	XSetWMHints(display, dockapp, &wm_hints);
	XSetCommand(display, dockapp, argv, argc);

	XSetWindowBackgroundPixmap(display, dockapp, ParentRelative);

	gettime(strtime, 50);
	calfont = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, calfontname, XFT_PIXEL_SIZE, XftTypeDouble, calfontsize, NULL);
	font = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, fontname, XFT_PIXEL_SIZE, XftTypeDouble, fontsize, NULL);
	XftTextExtents8(display, calfont, (unsigned char*)"0", strlen("0"), &extents);
	glyph_w = extents.xOff /* 1.4*/;
	glyph_h = extents.height * 1.8;
	XftTextExtents8(display, font, (unsigned char *) strtime, strlen(strtime), &extents);
	x = (windowwidth - extents.width) / 2 + extents.x;
	y = (24 - extents.height) / 2 + extents.y;
	if (x < 0 || y < 0)
		{
		fprintf(stderr, "Uh oh, text doesn't fit inside window\n");
		fflush(stderr);
		}
	draw = XftDrawCreate(display, dockapp, vis, colormap);
	if (XParseColor(display, colormap, colorname, &c) == None)
		{
		fprintf(stderr, "Couldn't parse color '%s'\n", colorname);
		fflush(stderr);
		color.red = 0xffff;
		color.green = 0xffff;
		color.blue = 0xffff;
		}
	else
		{
		color.red = c.red;
		color.blue = c.blue;
		color.green = c.green;
		}
	color.alpha = 0xffff;
	XftColorAllocValue(display, vis, colormap, &color, &xftcolor);
	
	if (XParseColor(display, colormap, bgcolorname, &c) == None)
		{
		fprintf(stderr, "Couldn't parse color '%s'\n", bgcolorname);
		fflush(stderr);
		color.red = 0xffff;
		color.green = 0xffff;
		color.blue = 0xffff;
		}
	else
		{
		color.red = c.red;
		color.blue = c.blue;
		color.green = c.green;
		}
	color.alpha = 0xffff;
	XftColorAllocValue(display, vis, colormap, &color, &xftbgcolor);

	if (XParseColor(display, colormap, hlcolorname, &c) == None)
		{
		fprintf(stderr, "Couldn't parse color '%s'\n", hlcolorname);
		fflush(stderr);
		color.red = 0xffff;
		color.green = 0xffff;
		color.blue = 0xffff;
		}
	else
		{
		color.red = c.red;
		color.blue = c.blue;
		color.green = c.green;
		}
	color.alpha = 0xffff;
	XftColorAllocValue(display, vis, colormap, &color, &xfthlcolor);

	XSelectInput(display, dockapp, ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask);
	XMapWindow(display, dockapp);
	XFlush(display);

	/*Set up calendar window */
	int loc_x = 0;
	int loc_y = 0;

	initweekdays();

	cal_width=glyph_w*21+1;
	calendar = XCreateSimpleWindow(display, root, loc_x, loc_y, cal_width, (glyph_h +2)* 8, 1, WhitePixel(display, DefaultScreen(display)),	xftbgcolor.pixel);
	attributes.override_redirect = True;
	XChangeWindowAttributes(display, calendar, CWOverrideRedirect, &attributes);
	XSelectInput(display, calendar, ExposureMask | ButtonPressMask);
	gc = XCreateGC(display, calendar, 0, NULL);
	caldraw = XftDrawCreate(display, calendar, vis, colormap);

	timeout.tv_sec = timeout.tv_usec = 0;

	while (running)
		{
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		selectret = select(xfd + 1, &fds, NULL, NULL, &timeout);

		switch (selectret)
			{
			case -1:
				continue;
			case 0:
				gettime(strtime, 50);
				XClearWindow(display, dockapp);
				XftDrawStringUtf8(draw, &xftcolor, font, x, y, (unsigned char *) strtime, strlen(strtime));
				XFlush(display);
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;
			case 1:
				if (FD_ISSET(xfd, &fds))
					{
					XNextEvent(display, &event);
					switch (event.type)
						{
						case ButtonPress:
							if (event.xbutton.window == dockapp && event.xbutton.button == 1)
								{ // toggle calendar
								if (calopen)
									{
									XUnmapWindow(display, calendar);
									calopen = 0;
									}
								else
									{
									time_t curtime=time(NULL);
									localtime_r(&curtime, &draw_cal);
									
									first_day_of_month(&draw_cal);

									locateCalendar(display, screen, dockapp, root, calendar);
									XMapWindow(display, calendar);
									calopen = 1;
									}
								}
							else if(event.xbutton.button == 1)
								{
								m_x = event.xbutton.x;
								if (m_x >= 0 && m_x < (glyph_w * 20 + 2) / 2)
									sub_month(&draw_cal);
								else if (m_x >= (glyph_w * 20 +	2) / 2 && m_x <= glyph_w * 20 + 2)
									add_month(&draw_cal);
								XClearWindow(display, calendar);
								paintCalendar(display, gc, calendar, caldraw, calfont, &xftcolor, &xfthlcolor);
								}
							XFlush(display);
							break;
						case Expose:
							XClearWindow(display, dockapp);
							XClearWindow(display, calendar);
							paintCalendar(display, gc, calendar, caldraw, calfont, &xftcolor, &xfthlcolor);
							XftDrawStringUtf8(draw, &xftcolor, font, x, y, (unsigned char *)strtime, strlen(strtime));
							XFlush(display);
						}
					}
				}
			}

	XftFontClose(display, font);
	XftFontClose(display, calfont);
	XftDrawDestroy(draw);
	XftDrawDestroy(caldraw);
	XCloseDisplay(display);

	return 0;
	}
