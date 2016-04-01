/*
 * calednar.h
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
#ifndef _CALENDAR_H
#define _CALEDNAR_H

#include <time.h>

void first_day_of_month(struct tm *cal);

void add_month(struct tm *cal);
void sub_month(struct tm *cal);

int end_of_month(struct tm *cal);

#endif
