/*
Copyright (C) 2003 Cedric Cellier, Dominique Lavault

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "system.h"
#ifdef _WIN32
#include "windows.h"
#endif

static GLTV_WARN_LEVEL log_warn_level=GLTV_LOG_IMPORTANT;

void gltv_log_init(GLTV_WARN_LEVEL warn_level) {
	log_warn_level = warn_level;
}

void gltv_log_end() {
}

#ifdef _WIN32
int outputDebug(const char *format, va_list args) {
	char buffer[5120];
	int nbPrintedChar;
	
	nbPrintedChar = vsprintf(buffer,format,args);
	
	//assert(nbPrintedChar < sizeof(buffer)/sizeof(buffer[0]));
	OutputDebugString(buffer);
	return nbPrintedChar;
}
#endif

static void writeA(const char *msg, va_list args) {
#ifdef _WIN32
	outputDebug(msg, args);
#else
	vfprintf(stderr, msg, args);
#endif
}

static void writeB(const char *msg) {
#ifdef _WIN32
	OutputDebugString(msg);
#else
	fputs(msg, stderr);
#endif
}

void gltv_log_fatal(const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	writeB("FATAL : ");
	writeA(msg, args);
	writeB("\n");
	va_end(args);
	exit(1);
}

void gltv_log_warning(GLTV_WARN_LEVEL level, const char *msg, ...) {
	va_list args;
	if (level<=log_warn_level) {
		va_start(args, msg);
		writeB("Warning : ");
		writeA(msg, args);
		writeB("\n");
		va_end(args);
	}
}

