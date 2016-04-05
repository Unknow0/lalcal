/*
 * calendar.c
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

#include "calendar.h"

void first_day_of_month(struct tm *cal)
	{
	if(cal->tm_mday==1)
		return;
	cal->tm_wday=cal->tm_wday-(cal->tm_mday%7)+1;
	if(cal->tm_wday<0)
		cal->tm_wday+=7;
	cal->tm_mday=1;
	}

void add_month(struct tm *cal)
	{
	int days=end_of_month(cal);

	cal->tm_mon++;
	if(cal->tm_mon==12)
		{
		cal->tm_mon=0;
		cal->tm_year++;
		}
	cal->tm_wday=(cal->tm_wday+days%7)%7;
	}

void sub_month(struct tm *cal)
	{
	int days;
	cal->tm_mon--;
	if(cal->tm_mon<0)
		{
		cal->tm_year--;
		cal->tm_mon=11;
		}
	days=end_of_month(cal);
	cal->tm_wday-=days%7;
	if(cal->tm_wday<0)
		cal->tm_wday+=7;
	}

int end_of_month(struct tm *cal)
	{
	if(cal->tm_mon==1) // febuary
		{ // TODO check leapyear
		int y=cal->tm_year;
		if((y%4==0&&y%100!=0)||y%400==0)
			return 29;
		else
			return 28;
		}
	else if(cal->tm_mon<7)
		return cal->tm_mon%2==0?31:30;
	else
		return cal->tm_mon%2==0?30:31;
	}
