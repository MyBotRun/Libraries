/*
AutoHotkey

Copyright 2003-2007 Chris Mallett (support@autohotkey.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#ifndef util_h
#define util_h

#include "stdafx.h" // pre-compiled headers
//#include "defines.h"

HBITMAP LoadPicture(char *aFilespec, int aWidth, int aHeight, int &aImageType, int aIconNumber
	, bool aUseGDIPlusIfAvailable);

char* WINAPI ImageSearch(int aLeft, int aTop, int aRight, int aBottom, char *aImageFile);

#endif
