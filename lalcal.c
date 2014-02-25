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

#define NUM_OPTS 10
static XrmOptionDescRec opt_table[] = {
    {"--font", "*font", XrmoptionSepArg, (XPointer) NULL},
    {"--fontsize", "*fontsize", XrmoptionSepArg, (XPointer) NULL},
    {"--calfont", "*calfont", XrmoptionSepArg, (XPointer) NULL},
    {"--calfontsize", "*calfontsize", XrmoptionSepArg, (XPointer) NULL},
    {"--color", "*color", XrmoptionSepArg, (XPointer) NULL},
    {"--hlcolor", "*hlcolor", XrmoptionSepArg, (XPointer) NULL},
    {"--bgcolor", "*bgcolor", XrmoptionSepArg, (XPointer) NULL},
    {"--width", "*width", XrmoptionSepArg, (XPointer) NULL},
    {"--format", "*format", XrmoptionSepArg, (XPointer) NULL},
    {"--help", "*help", XrmoptionNoArg, (XPointer) ""},
};

/*Calculates where to place calendar window*/
void locateCalendar(Display * display, int screen, Window dockapp,
		    Window root, Window calendar)
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

    if ((d_x < res_w / 2) && (d_y < res_h / 2)) {
	/*Top left */
	cal_x = d_x;
	cal_y = (d_y + d_h);
    } else if ((d_x >= res_w / 2) && (d_y < res_h / 2)) {
	/*Top right */
	cal_x = (d_x + d_w) - cal_w;
	cal_y = (d_y + d_h);
    } else if ((d_x < res_w / 2) && (d_y >= res_h / 2)) {
	/*Bottom left */
	cal_x = d_x;
	cal_y = d_y - cal_h;
    } else if ((d_x >= res_w / 2) && (d_y >= res_h / 2)) {
	/*Bottom right */
	cal_x = (d_x + d_w) - cal_w;
	cal_y = d_y - cal_h;
    }

    XMoveWindow(display, calendar, cal_x, cal_y);
}


static void gettime(char *strtime)
{
    time_t curtime;
    struct tm *curtime_tm;

    curtime = time(NULL);
    curtime_tm = localtime(&curtime);
    strftime(strtime, 49, timeformat, curtime_tm);
}

void shiftdate(int *month, int *year, int direction)
{
    *month += direction;
    if (*month == 0) {
	*month = 12;
	--*year;
    } else if (*month == 13) {
	*month = 1;
	++*year;
    }
}

int getday()
{
    time_t curtime;
    struct tm *curtime_tm;

    curtime = time(NULL);
    curtime_tm = localtime(&curtime);
    return curtime_tm->tm_mday;
}

int getmonth()
{
    time_t curtime;
    struct tm *curtime_tm;

    curtime = time(NULL);
    curtime_tm = localtime(&curtime);
    return curtime_tm->tm_mon + 1;
}

int getyear()
{
    time_t curtime;
    struct tm *curtime_tm;

    curtime = time(NULL);
    curtime_tm = localtime(&curtime);
    return curtime_tm->tm_year + 1900;
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
    XrmParseCommand(&database, opt_table, NUM_OPTS, "lalcal", &nargc,
		    nargv);

    /* check for help option */
    if (XrmGetResource
	(database, "lalcal.help", "lalcal.help", &type, &xrmval) == True) {
	printf("lalcal v0.1\n");
	printf("Usage: lalcal [options]\n\n");
	printf("Options:\n");
	printf("--help:        This text\n");
	printf("--font:        The font face to use.  Default is Mono.\n");
	printf("--fontsize:    The font size to use.  Default is 12.0.\n");
	printf
	    ("--calfont:     The font face to use for the calendar.  Default is Mono.\n");
	printf
	    ("--calfontsize: The font size to use for the calendar.  Default is 14.0.\n");
	printf
	    ("--color:       The font color to use.  Default is white.\n");
	printf
	    ("--bgcolor:     The background color to use.  Default is black.\n");
	printf
	    ("--hlcolor:     The highlight color to use.  Default is grey30.\n");
	printf
	    ("--width:       The width of the dockapp window.  Default is 64.\n");
	printf
	    ("--format:      The time format to use.  Default is %%T.  You can\n");
	printf
	    ("               get format codes from the man page for strftime.\n\n");
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
    if (XrmGetResource
	(database, "lalcal.font", "lalcal.font", &type, &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"font\" was in an unexpected format!");
	fontname = xrmval.addr;
    }

    if (XrmGetResource
	(database, "lalcal.fontsize", "lalcal.fontsize", &type,
	 &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"fontsize\" was in an unexpected format!");
	fontsize = strtod(xrmval.addr, NULL);
    }

    if (XrmGetResource
	(database, "lalcal.calfont", "lalcal.calfont", &type,
	 &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"calfont\" was in an unexpected format!");
	calfontname = xrmval.addr;
    }

    if (XrmGetResource
	(database, "lalcal.calfontsize", "lalcal.calfontsize", &type,
	 &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"calfontsize\" was in an unexpected format!");
	calfontsize = strtod(xrmval.addr, NULL);
    }

    if (XrmGetResource(database, "lalcal.color", "lalcal.color", &type,
		       &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"color\" was in an unexpected format!");
	colorname = xrmval.addr;
    }

    if (XrmGetResource(database, "lalcal.bgcolor", "lalcal.bgcolor", &type,
		       &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"color\" was in an unexpected format!");
	bgcolorname = xrmval.addr;
    }


    if (XrmGetResource(database, "lalcal.hlcolor", "lalcal.hlcolor", &type,
		       &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"hlcolor\" was in an unexpected format!");
	hlcolorname = xrmval.addr;
    }

    if (XrmGetResource(database, "lalcal.width", "lalcal.width", &type,
		       &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"width\" was in an unexpected format!");
	windowwidth = strtod(xrmval.addr, NULL);
    }

    if (XrmGetResource(database, "lalcal.format", "lalcal.format", &type,
		       &xrmval) == True) {
	if (strcmp(type, "String"))
	    die("Option \"format\" was in an unexpected format!");
	timeformat = xrmval.addr;
    }
}

void get_calendar(char line[8][30], int month, int year)
{
    FILE *fp, *file;
    int i = 0;
    char command[30];
    char buff[8][30];

    sprintf(command, "ncal -hMC %d %d", month, year);

    fp = popen(command, "r");
    while (fgets(buff[i], sizeof buff[i], fp))
	i++;
    pclose(fp);

	memset(line, 0, 8*30);
    for (i = 0; i < 8; i++)
		snprintf(line[i], 21, "%s", buff[i]);
}

void findtoday(int *todayi, int *todayj, int cal_m, int cal_y)
{
    char needle[3];
    char *pos_n;
	char *pos_h;
	int i=2;
    *todayi = -1;
    *todayj = -1;

    if (cal_m == getmonth() && cal_y == getyear()) {
		sprintf(needle, "%d", getday());

		pos_h = curr_cal[2];
		do {
			pos_n = strstr(curr_cal[i++], needle);
		} while(pos_n==0);

		*todayi = (int)(pos_n - pos_h) / sizeof curr_cal[2] + 2;
		*todayj = (int)(pos_n - pos_h) % sizeof curr_cal[2];

		if (getday() < 10)
		    -- * todayj;
    }
}

void paintCalendar(Display * display, GC gc, Window calendar, int cal_m,
		   int cal_y, XftDraw * caldraw, XftFont * font,
		   XftColor * xftcolor, XftColor * xfthlcolor)
{
    int i = 0, j = 0;
    int p_x = 0, p_y = 0;
    int todayi = 0, todayj = 0;
    char leftarrow[] = "⇐";
    char rightarrow[] = "⇒";
    char buff[2];
    XGlyphInfo extents;
    int glyph_w, glyph_h;

    XftTextExtents8(display, font, "0", strlen("0"), &extents);
    glyph_w = extents.xOff;
    glyph_h = extents.height * 1.8;

    get_calendar(curr_cal, cal_m, cal_y);
    findtoday(&todayi, &todayj, cal_m, cal_y);

	if(todayi>=0 && todayj>=0) {
		p_x=todayj*glyph_w-2;
		p_y=todayi*glyph_h+5;
		XftDrawRect(caldraw, xfthlcolor, p_x, p_y, glyph_w*2+6, extents.height+4);
		}


	XftDrawStringUtf8(caldraw, xftcolor, font, 1, glyph_h, curr_cal[0], strlen(curr_cal[0])-1);
    for (i = 1; i < 8; i++) {
			for (j = 0; j < 21; j++) {
				p_x = j * glyph_w + 1;
				p_y = (i + 1) * glyph_h;
				sprintf(buff, "%c", curr_cal[i][j]);
				if ((int) buff[0] != 10) {
				/*make sure not to print newline chars */
				XftDrawStringUtf8(caldraw,
					  xftcolor, font,
					  p_x, p_y, buff, strlen(buff));
				}
			}
	}

    XftDrawStringUtf8(caldraw, xftcolor, font, 1, glyph_h, leftarrow, strlen(leftarrow));
    XftDrawStringUtf8(caldraw, xftcolor, font, glyph_w * 19, glyph_h, rightarrow, strlen(rightarrow));

    XFlush(display);
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
    int cal_m, cal_y;
    int m_x = 0, m_y = 0;
    int glyph_w, glyph_h;

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

    cal_m = getmonth();
    cal_y = getyear();

    display = XOpenDisplay(NULL);
    if (!display) {
	fprintf(stderr, "Couldn't open X\n");
	fflush(stderr);
	exit(1);
    }

    /* command line/xresources: sets fonts etc */
    get_params(display, argc, argv);

    xfd = ConnectionNumber(display);
    if (xfd == -1) {
	fprintf(stderr, "Couldn't get X filedescriptor\n");
	fflush(stderr);
	exit(1);
    }

    screen = DefaultScreen(display);
    colormap = DefaultColormap(display, screen);
    root = DefaultRootWindow(display);
    vis = DefaultVisual(display, screen);
    dockapp =
	XCreateSimpleWindow(display, root, 0, 0, windowwidth, 24, 0, 0, 0);

    wm_hints.initial_state = WithdrawnState;
    wm_hints.icon_window = wm_hints.window_group = dockapp;
    wm_hints.flags = StateHint | IconWindowHint;
    XSetWMHints(display, dockapp, &wm_hints);
    XSetCommand(display, dockapp, argv, argc);

    XSetWindowBackgroundPixmap(display, dockapp, ParentRelative);

    gettime(strtime);
    calfont = XftFontOpen(display, screen,
			  XFT_FAMILY, XftTypeString, calfontname,
			  XFT_PIXEL_SIZE, XftTypeDouble, calfontsize,
			  NULL);
    font =
	XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, fontname,
		    XFT_PIXEL_SIZE, XftTypeDouble, fontsize, NULL);
    XftTextExtents8(display, calfont, "0", strlen("0"), &extents);
    glyph_w = extents.xOff /* 1.4*/;
    glyph_h = extents.height * 1.8;
    XftTextExtents8(display, font, (unsigned char *) strtime,
		    strlen(strtime), &extents);
    x = (windowwidth - extents.width) / 2 + extents.x;
    y = (24 - extents.height) / 2 + extents.y;
    if (x < 0 || y < 0) {
	fprintf(stderr, "Uh oh, text doesn't fit inside window\n");
	fflush(stderr);
    }
    draw = XftDrawCreate(display, dockapp, vis, colormap);
    if (XParseColor(display, colormap, colorname, &c) == None) {
	fprintf(stderr, "Couldn't parse color '%s'\n", colorname);
	fflush(stderr);
	color.red = 0xffff;
	color.green = 0xffff;
	color.blue = 0xffff;
    } else {
	color.red = c.red;
	color.blue = c.blue;
	color.green = c.green;
    }
    color.alpha = 0xffff;
    XftColorAllocValue(display, vis, colormap, &color, &xftcolor);
    
    if (XParseColor(display, colormap, bgcolorname, &c) == None) {
	fprintf(stderr, "Couldn't parse color '%s'\n", bgcolorname);
	fflush(stderr);
	color.red = 0xffff;
	color.green = 0xffff;
	color.blue = 0xffff;
    } else {
	color.red = c.red;
	color.blue = c.blue;
	color.green = c.green;
    }
    color.alpha = 0xffff;
    XftColorAllocValue(display, vis, colormap, &color, &xftbgcolor);

    if (XParseColor(display, colormap, hlcolorname, &c) == None) {
	fprintf(stderr, "Couldn't parse color '%s'\n", hlcolorname);
	fflush(stderr);
	color.red = 0xffff;
	color.green = 0xffff;
	color.blue = 0xffff;
    } else {
	color.red = c.red;
	color.blue = c.blue;
	color.green = c.green;
    }
    color.alpha = 0xffff;
    XftColorAllocValue(display, vis, colormap, &color, &xfthlcolor);

    XSelectInput(display, dockapp, ExposureMask | StructureNotifyMask |
		 ButtonPressMask | ButtonReleaseMask);
    XMapWindow(display, dockapp);
    XFlush(display);

    /*Set up calendar window */
    int loc_x = 0;
    int loc_y = 0;

    calendar =
	XCreateSimpleWindow(display, root, loc_x, loc_y,
			    glyph_w * 21 + 2,
			    glyph_h * 8 -
			    1, 1, WhitePixel(display,
					     DefaultScreen(display)),
			    xftbgcolor.pixel);
    attributes.override_redirect = True;
    XChangeWindowAttributes(display, calendar, CWOverrideRedirect,
			    &attributes);
    XSelectInput(display, calendar, ExposureMask | ButtonPressMask);
    gc = XCreateGC(display, calendar, 0, NULL);
    caldraw = XftDrawCreate(display, calendar, vis, colormap);

    timeout.tv_sec = timeout.tv_usec = 0;
    while (running) {
	FD_ZERO(&fds);
	FD_SET(xfd, &fds);

	selectret = select(xfd + 1, &fds, NULL, NULL, &timeout);

	switch (selectret) {
	case -1:
	    continue;
	case 0:
	    gettime(strtime);
	    XClearWindow(display, dockapp);
	    XftDrawStringUtf8(draw, &xftcolor, font, x, y,
			   (unsigned char *) strtime, strlen(strtime));
	    XFlush(display);
	    timeout.tv_sec = 1;
	    timeout.tv_usec = 0;
	case 1:
	    if (FD_ISSET(xfd, &fds)) {
		XNextEvent(display, &event);
		switch (event.type) {
		case ButtonPress:
		    if (event.xbutton.window == dockapp
			&& event.xbutton.button == 1) {
			if (calopen) {
			    XUnmapWindow(display, calendar);
			    calopen = 0;
			} else {
			    cal_m = getmonth();
			    cal_y = getyear();
			    get_calendar(curr_cal, cal_m, cal_y);
			    locateCalendar(display, screen, dockapp, root,
					   calendar);
			    XMapWindow(display, calendar);
			    calopen = 1;
			}
		    } else if (event.xbutton.button == 1) {
			m_x = event.xbutton.x;
			m_y = event.xbutton.y;
			if (m_x >= 0 && m_x < (glyph_w * 20 + 2) / 2) {
			    shiftdate(&cal_m, &cal_y, -1);
			    XClearWindow(display, calendar);
			    paintCalendar
				(display, gc,
				 calendar,
				 cal_m, cal_y,
				 caldraw, calfont, &xftcolor, &xfthlcolor);
			} else if (m_x >=
				   (glyph_w * 20 +
				    2) / 2 && m_x <= glyph_w * 20 + 2) {
			    shiftdate(&cal_m, &cal_y, 1);
			    XClearWindow(display, calendar);
			    paintCalendar
				(display, gc,
				 calendar,
				 cal_m, cal_y,
				 caldraw, calfont, &xftcolor, &xfthlcolor);
			}
		    }
		    XFlush(display);
		    break;
		case Expose:
		    XClearWindow(display, dockapp);
		    XClearWindow(display, calendar);
		    paintCalendar(display, gc,
				  calendar, cal_m,
				  cal_y, caldraw, calfont,
				  &xftcolor, &xfthlcolor);
		    XftDrawStringUtf8(draw, &xftcolor,
				   font, x, y, (unsigned char *)
				   strtime, strlen(strtime));
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
