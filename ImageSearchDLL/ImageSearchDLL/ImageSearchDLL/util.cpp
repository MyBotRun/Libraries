/*
AutoHotkey

Copyright 2003-2007 Chris Mallett (support@autohotkey.com)
DLL conversion 2008: kangkengkingkong@hotmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "stdafx.h" // pre-compiled headers
#include <olectl.h> // for OleLoadPicture()
#include <Gdiplus.h> // Used by LoadPicture().
#include <windef.h>
#include <windows.h>
#include <winuser.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>


#define CLR_DEFAULT 0x808080
#define ToWideChar(source, dest, dest_size_in_wchars) MultiByteToWideChar(CP_ACP, 0, source, -1, dest, dest_size_in_wchars)

#define CLR_NONE 0xFFFFFFFF
#define IS_SPACE_OR_TAB(c) (c == ' ' || c == '\t')

char answer[50];

HINSTANCE g_hInstance;

#define SET_COLOR_RANGE \
{\
	red_low = (aVariation > search_red) ? 0 : search_red - aVariation;\
	green_low = (aVariation > search_green) ? 0 : search_green - aVariation;\
	blue_low = (aVariation > search_blue) ? 0 : search_blue - aVariation;\
	red_high = (aVariation > 0xFF - search_red) ? 0xFF : search_red + aVariation;\
	green_high = (aVariation > 0xFF - search_green) ? 0xFF : search_green + aVariation;\
	blue_high = (aVariation > 0xFF - search_blue) ? 0xFF : search_blue + aVariation;\
}
#define bgr_to_rgb(aBGR) rgb_to_bgr(aBGR)

inline COLORREF rgb_to_bgr(DWORD aRGB)
// Fancier methods seem prone to problems due to byte alignment or compiler issues.
{
	return RGB(GetBValue(aRGB), GetGValue(aRGB), GetRValue(aRGB));
}

COLORREF ColorNameToBGR(char *aColorName)
// These are the main HTML color names.  Returns CLR_NONE if a matching HTML color name can't be found.
// Returns CLR_DEFAULT only if aColorName is the word Default.
{
	if (!aColorName || !*aColorName) return CLR_NONE;
	if (!_stricmp(aColorName, "Black"))  return 0x000000;  // These colors are all in BGR format, not RGB.
	if (!_stricmp(aColorName, "Silver")) return 0xC0C0C0;
	if (!_stricmp(aColorName, "Gray"))   return 0x808080;
	if (!_stricmp(aColorName, "White"))  return 0xFFFFFF;
	if (!_stricmp(aColorName, "Maroon")) return 0x000080;
	if (!_stricmp(aColorName, "Red"))    return 0x0000FF;
	if (!_stricmp(aColorName, "Purple")) return 0x800080;
	if (!_stricmp(aColorName, "Fuchsia"))return 0xFF00FF;
	if (!_stricmp(aColorName, "Green"))  return 0x008000;
	if (!_stricmp(aColorName, "Lime"))   return 0x00FF00;
	if (!_stricmp(aColorName, "Olive"))  return 0x008080;
	if (!_stricmp(aColorName, "Yellow")) return 0x00FFFF;
	if (!_stricmp(aColorName, "Navy"))   return 0x800000;
	if (!_stricmp(aColorName, "Blue"))   return 0xFF0000;
	if (!_stricmp(aColorName, "Teal"))   return 0x808000;
	if (!_stricmp(aColorName, "Aqua"))   return 0xFFFF00;
	if (!_stricmp(aColorName, "Default"))return CLR_DEFAULT;
	return CLR_NONE;
}

inline char *StrChrAny(char *aStr, char *aCharList)
// Returns the position of the first char in aStr that is of any one of the characters listed in aCharList.
// Returns NULL if not found.
// Update: Yes, this seems identical to strpbrk().  However, since the corresponding code would
// have to be added to the EXE regardless of which was used, there doesn't seem to be much
// advantage to switching (especially since if the two differ in behavior at all, things might
// get broken).  Another reason is the name "strpbrk()" is not as easy to remember.
{
	if (aStr == NULL || aCharList == NULL) return NULL;
	if (!*aStr || !*aCharList) return NULL;
	// Don't use strchr() because that would just find the first occurrence
	// of the first search-char, which is not necessarily the first occurrence
	// of *any* search-char:
	char *look_for_this_char, char_being_analyzed;
	for (; *aStr; ++aStr)
		// If *aStr is any of the search char's, we're done:
		for (char_being_analyzed = *aStr, look_for_this_char = aCharList; *look_for_this_char; ++look_for_this_char)
			if (char_being_analyzed == *look_for_this_char)
				return aStr;  // Match found.
	return NULL; // No match.
}
inline char *omit_leading_whitespace(char *aBuf) // 10/17/2006: __forceinline didn't help significantly.
// While aBuf points to a whitespace, moves to the right and returns the first non-whitespace
// encountered.
{
	for (; IS_SPACE_OR_TAB(*aBuf); ++aBuf);
	return aBuf;
}

inline bool IsHex(char *aBuf) // 10/17/2006: __forceinline worsens performance, but physically ordering it near ATOI64() [via /ORDER] boosts by 3.5%.
// Note: AHK support for hex ints reduces performance by only 10% for decimal ints, even in the tightest
// of math loops that have SetBatchLines set to -1.
{
	// For whatever reason, omit_leading_whitespace() benches consistently faster (albeit slightly) than
	// the same code put inline (confirmed again on 10/17/2006, though the difference is hardly anything):
	//for (; IS_SPACE_OR_TAB(*aBuf); ++aBuf);
	aBuf = omit_leading_whitespace(aBuf); // i.e. caller doesn't have to have ltrimmed.
	if (!*aBuf)
		return false;
	if (*aBuf == '-' || *aBuf == '+')
		++aBuf;
	// The "0x" prefix must be followed by at least one hex digit, otherwise it's not considered hex:
	#define IS_HEX(buf) (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X') && isxdigit(*(buf + 2)))
	return IS_HEX(aBuf);
}

inline int ATOI(char *buf)
{
	// Below has been updated because values with leading zeros were being intepreted as
	// octal, which is undesirable.
	// Formerly: #define ATOI(buf) strtol(buf, NULL, 0) // Use zero as last param to support both hex & dec.
	return IsHex(buf) ? strtol(buf, NULL, 16) : atoi(buf); // atoi() has superior performance, so use it when possible.
}

void strlcpy(char *aDst, const char *aSrc, size_t aDstSize) // Non-inline because it benches slightly faster that way.
// Caller must ensure that aDstSize is greater than 0.
// Caller must ensure that the entire capacity of aDst is writable, EVEN WHEN it knows that aSrc is much shorter
// than the aDstSize.  This is because the call to strncpy (which is used for its superior performance) zero-fills
// any unused portion of aDst.
// Description:
// Same as strncpy() but guarantees null-termination of aDst upon return.
// No more than aDstSize - 1 characters will be copied from aSrc into aDst
// (leaving room for the zero terminator, which is always inserted).
// This function is defined in some Unices but is not standard.  But unlike
// other versions, this one uses void for return value for reduced code size
// (since it's called in so many places).
{
	// Disabled for performance and reduced code size:
	//if (!aDst || !aSrc || !aDstSize) return aDstSize;  // aDstSize must not be zero due to the below method.
	// It might be worthwhile to have a custom char-copying-loop here someday so that number of characters
	// actually copied (not including the zero terminator) can be returned to callers who want it.
	--aDstSize; // Convert from size to length (caller has ensured that aDstSize > 0).
	strncpy(aDst, aSrc, aDstSize); // NOTE: In spite of its zero-filling, strncpy() benchmarks considerably faster than a custom loop, probably because it uses 32-bit memory operations vs. 8-bit.
	aDst[aDstSize] = '\0';
}

LPCOLORREF getbits(HBITMAP ahImage, HDC hdc, LONG &aWidth, LONG &aHeight, bool &aIs16Bit, int aMinColorDepth = 8)
// Helper function used by PixelSearch below.
// Returns an array of pixels to the caller, which it must free when done.  Returns NULL on failure,
// in which case the contents of the output parameters is indeterminate.
{
	HDC tdc = CreateCompatibleDC(hdc);
	if (!tdc)
		return NULL;

	// From this point on, "goto end" will assume tdc is non-NULL, but that the below
	// might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	// label can detect them:
	HGDIOBJ tdc_orig_select = NULL;
	LPCOLORREF image_pixel = NULL;
	bool success = false;

	// Confirmed:
	// Needs extra memory to prevent buffer overflow due to: "A bottom-up DIB is specified by setting
	// the height to a positive number, while a top-down DIB is specified by setting the height to a
	// negative number. THE BITMAP COLOR TABLE WILL BE APPENDED to the BITMAPINFO structure."
	// Maybe this applies only to negative height, in which case the second call to GetDIBits()
	// below uses one.
	struct BITMAPINFO3
	{
		BITMAPINFOHEADER    bmiHeader;
		RGBQUAD             bmiColors[260];  // v1.0.40.10: 260 vs. 3 to allow room for color table when color depth is 8-bit or less.
	} bmi;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount = 0; // i.e. "query bitmap attributes" only.
	if (!GetDIBits(tdc, ahImage, 0, 0, NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS)
		|| bmi.bmiHeader.biBitCount < aMinColorDepth) // Relies on short-circuit boolean order.
		goto end;

	// Set output parameters for caller:
	aIs16Bit = (bmi.bmiHeader.biBitCount == 16);
	aWidth = bmi.bmiHeader.biWidth;
	aHeight = bmi.bmiHeader.biHeight;

	int image_pixel_count = aWidth * aHeight;
	if (   !(image_pixel = (LPCOLORREF)malloc(image_pixel_count * sizeof(COLORREF)))   )
		goto end;

	// v1.0.40.10: To preserve compatibility with callers who check for transparency in icons, don't do any
	// of the extra color table handling for 1-bpp images.  Update: For code simplification, support only
	// 8-bpp images.  If ever support lower color depths, use something like "bmi.bmiHeader.biBitCount > 1
	// && bmi.bmiHeader.biBitCount < 9";
	bool is_8bit = (bmi.bmiHeader.biBitCount == 8);
	if (!is_8bit)
		bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight; // Storing a negative inside the bmiHeader struct is a signal for GetDIBits().

	// Must be done only after GetDIBits() because: "The bitmap identified by the hbmp parameter
	// must not be selected into a device context when the application calls GetDIBits()."
	// (Although testing shows it works anyway, perhaps because GetDIBits() above is being
	// called in its informational mode only).
	// Note that this seems to return NULL sometimes even though everything still works.
	// Perhaps that is normal.
	tdc_orig_select = SelectObject(tdc, ahImage); // Returns NULL when we're called the second time?

	// Appparently there is no need to specify DIB_PAL_COLORS below when color depth is 8-bit because
	// DIB_RGB_COLORS also retrieves the color indices.
	if (   !(GetDIBits(tdc, ahImage, 0, aHeight, image_pixel, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS))   )
		goto end;

	if (is_8bit) // This section added in v1.0.40.10.
	{
		// Convert the color indicies to RGB colors by going through the array in reverse order.
		// Reverse order allows an in-place conversion of each 8-bit color index to its corresponding
		// 32-bit RGB color.
		LPDWORD palette = (LPDWORD)_alloca(256 * sizeof(PALETTEENTRY));
		GetSystemPaletteEntries(tdc, 0, 256, (LPPALETTEENTRY)palette); // Even if failure can realistically happen, consequences of using uninitialized palette seem acceptable.
		// Above: GetSystemPaletteEntries() is the only approach that provided the correct palette.
		// The following other approaches didn't give the right one:
		// GetDIBits(): The palette it stores in bmi.bmiColors seems completely wrong.
		// GetPaletteEntries()+GetCurrentObject(hdc, OBJ_PAL): Returned only 20 entries rather than the expected 256.
		// GetDIBColorTable(): I think same as above or maybe it returns 0.

		// The following section is necessary because apparently each new row in the region starts on
		// a DWORD boundary.  So if the number of pixels in each row isn't an exact multiple of 4, there
		// are between 1 and 3 zero-bytes at the end of each row.
		int remainder = aWidth % 4;
		int empty_bytes_at_end_of_each_row = remainder ? (4 - remainder) : 0;

		// Start at the last RGB slot and the last color index slot:
		BYTE *byte = (BYTE *)image_pixel + image_pixel_count - 1 + (aHeight * empty_bytes_at_end_of_each_row); // Pointer to 8-bit color indices.
		DWORD *pixel = image_pixel + image_pixel_count - 1; // Pointer to 32-bit RGB entries.

		int row, col;
		for (row = 0; row < aHeight; ++row) // For each row.
		{
			byte -= empty_bytes_at_end_of_each_row;
			for (col = 0; col < aWidth; ++col) // For each column.
				*pixel-- = rgb_to_bgr(palette[*byte--]); // Caller always wants RGB vs. BGR format.
		}
	}
	
	// Since above didn't "goto end", indicate success:
	success = true;

end:
	if (tdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
		SelectObject(tdc, tdc_orig_select); // Probably necessary to prevent memory leak.
	DeleteDC(tdc);
	if (!success && image_pixel)
	{
		free(image_pixel);
		image_pixel = NULL;
	}
	return image_pixel;
}

HBITMAP LoadPicture(char *aFilespec, int aWidth, int aHeight, int &aImageType, int aIconNumber
	, bool aUseGDIPlusIfAvailable)
// Returns NULL on failure.
// If aIconNumber > 0, an HICON or HCURSOR is returned (both should be interchangeable), never an HBITMAP.
// However, aIconNumber==1 is treated as a special icon upon which LoadImage is given preference over ExtractIcon
// for .ico/.cur/.ani files.
// Otherwise, .ico/.cur/.ani files are normally loaded as HICON (unless aUseGDIPlusIfAvailable is true or
// something else unusual happened such as file contents not matching file's extension).  This is done to preserve
// any properties that HICONs have but HBITMAPs lack, namely the ability to be animated and perhaps other things.
//
// Loads a JPG/GIF/BMP/ICO/etc. and returns an HBITMAP or HICON to the caller (which it may call
// DeleteObject()/DestroyIcon() upon, though upon program termination all such handles are freed
// automatically).  The image is scaled to the specified width and height.  If zero is specified
// for either, the image's actual size will be used for that dimension.  If -1 is specified for one,
// that dimension will be kept proportional to the other dimension's size so that the original aspect
// ratio is retained.
{
	HBITMAP hbitmap = NULL;
	aImageType = -1; // The type of image currently inside hbitmap.  Set default value for output parameter as "unknown".

	if (!*aFilespec) // Allow blank filename to yield NULL bitmap (and currently, some callers do call it this way).
		return NULL;
	if (aIconNumber < 0) // Allowed to be called this way by GUI and others (to avoid need for validation of user input there).
		aIconNumber = 0; // Use the default behavior, which is "load icon or bitmap, whichever is most appropriate".

	char *file_ext = strrchr(aFilespec, '.');
	if (file_ext)
		++file_ext;

  
	// v1.0.43.07: If aIconNumber is zero, caller didn't specify whether it wanted an icon or bitmap.  Thus,
	// there must be some kind of detection for whether ExtractIcon is needed instead of GDIPlus/OleLoadPicture.
	// Although this could be done by attempting ExtractIcon only after GDIPlus/OleLoadPicture fails (or by
	// somehow checking the internal nature of the file), for performance and code size, it seems best to not
	// to incur this extra I/O and instead make only one attempt based on the file's extension.
	// Must use ExtractIcon() if either of the following is true:
	// 1) Caller gave an icon index of the second or higher icon in the file.  Update for v1.0.43.05: There
	//    doesn't seem to be any reason to allow a caller to explicitly specify ExtractIcon as the method of
	//    loading the *first* icon from a .ico file since LoadImage is likely always superior.  This is
	//    because unlike ExtractIcon/Ex, LoadImage: 1) Doesn't distort icons, especially 16x16 icons; 2) is
	//    capable of loading icons other than the first by means of width and height parameters.
	// 2) The target file is of type EXE/DLL/ICL/CPL/etc. (LoadImage() is documented not to work on those file types).
	//    ICL files (v1.0.43.05): Apparently ICL files are an unofficial file format. Someone on the newsgroups
	//    said that an ICL is an "ICon Library... a renamed 16-bit Windows .DLL (an NE format executable) which
	//    typically contains nothing but a resource section. The ICL extension seems to be used by convention."
	bool ExtractIcon_was_used = aIconNumber > 1 || (file_ext && (
		   !_stricmp(file_ext, "exe")
		|| !_stricmp(file_ext, "dll")
		|| !_stricmp(file_ext, "icl") // Icon library: Unofficial dll container, see notes above.
		|| !_stricmp(file_ext, "cpl") // Control panel extension/applet (ExtractIcon is said to work on these).
		|| !_stricmp(file_ext, "scr") // Screen saver (ExtractIcon should work since these are really EXEs).
		// v1.0.44: Below are now omitted to reduce code size and improve performance. They are still supported
		// indirectly because ExtractIcon is attempted whenever LoadImage() fails further below.
		//|| !_stricmp(file_ext, "drv") // Driver (ExtractIcon is said to work on these).
		//|| !_stricmp(file_ext, "ocx") // OLE/ActiveX Control Extension
		//|| !_stricmp(file_ext, "vbx") // Visual Basic Extension
		//|| !_stricmp(file_ext, "acm") // Audio Compression Manager Driver
		//|| !_stricmp(file_ext, "bpl") // Delphi Library (like a DLL?)
		// Not supported due to rarity, code size, performance, and uncertainty of whether ExtractIcon works on them.
		// Update for v1.0.44: The following are now supported indirectly because ExtractIcon is attempted whenever
		// LoadImage() fails further below.
		//|| !_stricmp(file_ext, "nil") // Norton Icon Library 
		//|| !_stricmp(file_ext, "wlx") // Total/Windows Commander Lister Plug-in
		//|| !_stricmp(file_ext, "wfx") // Total/Windows Commander File System Plug-in
		//|| !_stricmp(file_ext, "wcx") // Total/Windows Commander Plug-in
		//|| !_stricmp(file_ext, "wdx") // Total/Windows Commander Plug-in
		));
	if (ExtractIcon_was_used)
	{
		aImageType = IMAGE_ICON;
		hbitmap = (HBITMAP)ExtractIcon(g_hInstance, aFilespec, aIconNumber > 0 ? aIconNumber - 1 : 0);
		// Above: Although it isn't well documented at MSDN, apparently both ExtractIcon() and LoadIcon()
		// scale the icon to the system's large-icon size (usually 32x32) regardless of the actual size of
		// the icon inside the file.  For this reason, callers should call us in a way that allows us to
		// give preference to LoadImage() over ExtractIcon() (unless the caller needs to retain backward
		// compatibility with existing scripts that explicitly specify icon #1 to force the ExtractIcon
		// method to be used).
		if (hbitmap < (HBITMAP)2) // i.e. it's NULL or 1. Return value of 1 means "incorrect file type".
			return NULL; // v1.0.44: Fixed to return NULL vs. hbitmap, since 1 is an invalid handle (perhaps rare since no known bugs caused by it).
		//else continue on below so that the icon can be resized to the caller's specified dimensions.
	}
	else if (aIconNumber > 0) // Caller wanted HICON, never HBITMAP, so set type now to enforce that.
		aImageType = IMAGE_ICON; // Should be suitable for cursors too, since they're interchangeable for the most part.
	else if (file_ext) // Make an initial guess of the type of image if the above didn't already determine the type.
	{
		if (!_stricmp(file_ext, "ico"))
			aImageType = IMAGE_ICON;
		else if (!_stricmp(file_ext, "cur") || !_stricmp(file_ext, "ani"))
			aImageType = IMAGE_CURSOR;
		else if (!_stricmp(file_ext, "bmp"))
			aImageType = IMAGE_BITMAP;
		//else for other extensions, leave set to "unknown" so that the below knows to use IPic or GDI+ to load it.
	}
	//else same comment as above.

	if ((aWidth == -1 || aHeight == -1) && (!aWidth || !aHeight))
		aWidth = aHeight = 0; // i.e. One dimension is zero and the other is -1, which resolves to the same as "keep original size".
	bool keep_aspect_ratio = (aWidth == -1 || aHeight == -1);

	// Caller should ensure that aUseGDIPlusIfAvailable==false when aIconNumber > 0, since it makes no sense otherwise.
	HINSTANCE hinstGDI = NULL;
	if (aUseGDIPlusIfAvailable && !(hinstGDI = LoadLibrary("gdiplus"))) // Relies on short-circuit boolean order for performance.
		aUseGDIPlusIfAvailable = false; // Override any original "true" value as a signal for the section below.

	if (!hbitmap && aImageType > -1 && !aUseGDIPlusIfAvailable)
	{
		// Since image hasn't yet be loaded and since the file type appears to be one supported by
		// LoadImage() [icon/cursor/bitmap], attempt that first.  If it fails, fall back to the other
		// methods below in case the file's internal contents differ from what the file extension indicates.
		int desired_width, desired_height;
		if (keep_aspect_ratio) // Load image at its actual size.  It will be rescaled to retain aspect ratio later below.
		{
			desired_width = 0;
			desired_height = 0;
		}
		else
		{
			desired_width = aWidth;
			desired_height = aHeight;
		}
		// For LoadImage() below:
		// LR_CREATEDIBSECTION applies only when aImageType == IMAGE_BITMAP, but seems appropriate in that case.
		// Also, if width and height are non-zero, that will determine which icon of a multi-icon .ico file gets
		// loaded (though I don't know the exact rules of precedence).
		// KNOWN LIMITATIONS/BUGS:
		// LoadImage() fails when requesting a size of 1x1 for an image whose orig/actual size is small (e.g. 1x2).
		// Unlike CopyImage(), perhaps it detects that division by zero would occur and refuses to do the
		// calculation rather than providing more code to do a correct calculation that doesn't divide by zero.
		// For example:
		// LoadImage() Success:
		//   Gui, Add, Pic, h2 w2, bitmap 1x2.bmp
		//   Gui, Add, Pic, h1 w1, bitmap 4x6.bmp
		// LoadImage() Failure:
		//   Gui, Add, Pic, h1 w1, bitmap 1x2.bmp
		// LoadImage() also fails on:
		//   Gui, Add, Pic, h1, bitmap 1x2.bmp
		// And then it falls back to GDIplus, which in the particular case above appears to traumatize the
		// parent window (or its picture control), because the GUI window hangs (but not the script) after
		// doing a FileSelectFolder.  For example:
		//   Gui, Add, Button,, FileSelectFile
		//   Gui, Add, Pic, h1, bitmap 1x2.bmp  ; Causes GUI window to hang after FileSelectFolder (due to LoadImage failing then falling back to GDIplus; i.e. GDIplus is somehow triggering the problem).
		//   Gui, Show
		//   return
		//   ButtonFileSelectFile:
		//   FileSelectFile, outputvar
		//   return
		//printf("\nloading image %s",(LPCTSTR)aFilespec);
		if (hbitmap = (HBITMAP)LoadImage(NULL, aFilespec, aImageType, desired_width, desired_height
			, LR_LOADFROMFILE | LR_CREATEDIBSECTION))
		{
			// The above might have loaded an HICON vs. an HBITMAP (it has been confirmed that LoadImage()
			// will return an HICON vs. HBITMAP is aImageType is IMAGE_ICON/CURSOR).  Note that HICON and
			// HCURSOR are identical for most/all Windows API uses.  Also note that LoadImage() will load
			// an icon as a bitmap if the file contains an icon but IMAGE_BITMAP was passed in (at least
			// on Windows XP).
			//printf("\n got here");
			if (!keep_aspect_ratio) // No further resizing is needed.
				return hbitmap;
			// Otherwise, continue on so that the image can be resized via a second call to LoadImage().
		}
		// v1.0.40.10: Abort if file doesn't exist so that GDIPlus isn't even attempted. This is done because
		// loading GDIPlus apparently disrupts the color palette of certain games, at least old ones that use
		// DirectDraw in 256-color depth.
		else if (GetFileAttributes(aFilespec) == 0xFFFFFFFF) // For simplicity, we don't check if it's a directory vs. file, since that should be too rare.
			return NULL;
		// v1.0.43.07: Also abort if caller wanted an HICON (not an HBITMAP), since the other methods below
		// can't yield an HICON.
		else if (aIconNumber > 0)
		{
			// UPDATE for v1.0.44: Attempt ExtractIcon in case its some extension that's
			// was recognized as an icon container (such as AutoHotkeySC.bin) and thus wasn't handled higher above.
			hbitmap = (HBITMAP)ExtractIcon(g_hInstance, aFilespec, aIconNumber - 1);
			if (hbitmap < (HBITMAP)2) // i.e. it's NULL or 1. Return value of 1 means "incorrect file type".
				return NULL;
			ExtractIcon_was_used = true;
		}
		//else file exists, so continue on so that the other methods are attempted in case file's contents
		// differ from what the file extension indicates, or in case the other methods can be successful
		// even when the above failed.
	}

	IPicture *pic = NULL; // Also used to detect whether IPic method was used to load the image.

	if (!hbitmap) // Above hasn't loaded the image yet, so use the fall-back methods.
	{
		// At this point, regardless of the image type being loaded (even an icon), it will
		// definitely be converted to a Bitmap below.  So set the type:
		aImageType = IMAGE_BITMAP;
		// Find out if this file type is supported by the non-GDI+ method.  This check is not foolproof
		// since all it does is look at the file's extension, not its contents.  However, it doesn't
		// need to be 100% accurate because its only purpose is to detect whether the higher-overhead
		// calls to GdiPlus can be avoided.
		if (aUseGDIPlusIfAvailable || !file_ext || (_stricmp(file_ext, "jpg")
			&& _stricmp(file_ext, "jpeg") && _stricmp(file_ext, "gif"))) // Non-standard file type (BMP is already handled above).
			if (!hinstGDI) // We don't yet have a handle from an earlier call to LoadLibary().
				hinstGDI = LoadLibrary("gdiplus");
		// If it is suspected that the file type isn't supported, try to use GdiPlus if available.
		// If it's not available, fall back to the old method in case the filename doesn't properly
		// reflect its true contents (i.e. in case it really is a JPG/GIF/BMP internally).
		// If the below LoadLibrary() succeeds, either the OS is XP+ or the GdiPlus extensions have been
		// installed on an older OS.
		if (hinstGDI)
		{
			// LPVOID and "int" are used to avoid compiler errors caused by... namespace issues?
			typedef int (WINAPI *GdiplusStartupType)(ULONG_PTR*, LPVOID, LPVOID);
			typedef VOID (WINAPI *GdiplusShutdownType)(ULONG_PTR);
			typedef int (WINGDIPAPI *GdipCreateBitmapFromFileType)(LPVOID, LPVOID);
			typedef int (WINGDIPAPI *GdipCreateHBITMAPFromBitmapType)(LPVOID, LPVOID, DWORD);
			typedef int (WINGDIPAPI *GdipDisposeImageType)(LPVOID);
			GdiplusStartupType DynGdiplusStartup = (GdiplusStartupType)GetProcAddress(hinstGDI, "GdiplusStartup");
  			GdiplusShutdownType DynGdiplusShutdown = (GdiplusShutdownType)GetProcAddress(hinstGDI, "GdiplusShutdown");
  			GdipCreateBitmapFromFileType DynGdipCreateBitmapFromFile = (GdipCreateBitmapFromFileType)GetProcAddress(hinstGDI, "GdipCreateBitmapFromFile");
  			GdipCreateHBITMAPFromBitmapType DynGdipCreateHBITMAPFromBitmap = (GdipCreateHBITMAPFromBitmapType)GetProcAddress(hinstGDI, "GdipCreateHBITMAPFromBitmap");
  			GdipDisposeImageType DynGdipDisposeImage = (GdipDisposeImageType)GetProcAddress(hinstGDI, "GdipDisposeImage");

			ULONG_PTR token;
			Gdiplus::GdiplusStartupInput gdi_input;
			Gdiplus::GpBitmap *pgdi_bitmap;
			if (DynGdiplusStartup && DynGdiplusStartup(&token, &gdi_input, NULL) == Gdiplus::Ok)
			{
				WCHAR filespec_wide[MAX_PATH];
				ToWideChar(aFilespec, filespec_wide, MAX_PATH); // Dest. size is in wchars, not bytes.
				if (DynGdipCreateBitmapFromFile(filespec_wide, &pgdi_bitmap) == Gdiplus::Ok)
				{
					if (DynGdipCreateHBITMAPFromBitmap(pgdi_bitmap, &hbitmap, CLR_DEFAULT) != Gdiplus::Ok)
						hbitmap = NULL; // Set to NULL to be sure.
					DynGdipDisposeImage(pgdi_bitmap); // This was tested once to make sure it really returns Gdiplus::Ok.
				}
				// The current thought is that shutting it down every time conserves resources.  If so, it
				// seems justified since it is probably called infrequently by most scripts:
				DynGdiplusShutdown(token);
			}
			FreeLibrary(hinstGDI);
		}
		else // Using old picture loading method.
		{
			// Based on code sample at http://www.codeguru.com/Cpp/G-M/bitmap/article.php/c4935/
			HANDLE hfile = CreateFile(aFilespec, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hfile == INVALID_HANDLE_VALUE)
				return NULL;
			DWORD size = GetFileSize(hfile, NULL);
			HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE, size);
			if (!hglobal)
			{
				CloseHandle(hfile);
				return NULL;
			}
			LPVOID hlocked = GlobalLock(hglobal);
			if (!hlocked)
			{
				CloseHandle(hfile);
				GlobalFree(hglobal);
				return NULL;
			}
			// Read the file into memory:
			ReadFile(hfile, hlocked, size, &size, NULL);
			GlobalUnlock(hglobal);
			CloseHandle(hfile);
			LPSTREAM stream;
			if (FAILED(CreateStreamOnHGlobal(hglobal, FALSE, &stream)) || !stream)  // Relies on short-circuit boolean order.
			{
				GlobalFree(hglobal);
				return NULL;
			}
			// Specify TRUE to have it do the GlobalFree() for us.  But since the call might fail, it seems best
			// to free the mem ourselves to avoid uncertainy over what it does on failure:
			if (FAILED(OleLoadPicture(stream, 0, FALSE, IID_IPicture, (void **)&pic)))
				pic = NULL;
			stream->Release();
			GlobalFree(hglobal);
			if (!pic)
				return NULL;
			pic->get_Handle((OLE_HANDLE *)&hbitmap);
			// Above: MSDN: "The caller is responsible for this handle upon successful return. The variable is set
			// to NULL on failure."
			if (!hbitmap)
			{
				pic->Release();
				return NULL;
			}
			// Don't pic->Release() yet because that will also destroy/invalidate hbitmap handle.
		} // IPicture method was used.
	} // IPicture or GDIPlus was used to load the image, not a simple LoadImage() or ExtractIcon().

	// Above has ensured that hbitmap is now not NULL.
	// Adjust things if "keep aspect ratio" is in effect:
	if (keep_aspect_ratio)
	{
		HBITMAP hbitmap_to_analyze;
		ICONINFO ii; // Must be declared at this scope level.
		if (aImageType == IMAGE_BITMAP)
			hbitmap_to_analyze = hbitmap;
		else // icon or cursor
		{
			if (GetIconInfo((HICON)hbitmap, &ii)) // Works on cursors too.
				hbitmap_to_analyze = ii.hbmMask; // Use Mask because MSDN implies hbmColor can be NULL for monochrome cursors and such.
			else
			{
				DestroyIcon((HICON)hbitmap);
				return NULL; // No need to call pic->Release() because since it's an icon, we know IPicture wasn't used (it only loads bitmaps).
			}
		}
		// Above has ensured that hbitmap_to_analyze is now not NULL.  Find bitmap's dimensions.
		BITMAP bitmap;
		GetObject(hbitmap_to_analyze, sizeof(BITMAP), &bitmap); // Realistically shouldn't fail at this stage.
		if (aHeight == -1)
		{
			// Caller wants aHeight calculated based on the specified aWidth (keep aspect ratio).
			if (bitmap.bmWidth) // Avoid any chance of divide-by-zero.
				aHeight = (int)(((double)bitmap.bmHeight / bitmap.bmWidth) * aWidth + .5); // Round.
		}
		else
		{
			// Caller wants aWidth calculated based on the specified aHeight (keep aspect ratio).
			if (bitmap.bmHeight) // Avoid any chance of divide-by-zero.
				aWidth = (int)(((double)bitmap.bmWidth / bitmap.bmHeight) * aHeight + .5); // Round.
		}
		if (aImageType != IMAGE_BITMAP)
		{
			// It's our reponsibility to delete these two when they're no longer needed:
			DeleteObject(ii.hbmColor);
			DeleteObject(ii.hbmMask);
			// If LoadImage() vs. ExtractIcon() was used originally, call LoadImage() again because
			// I haven't found any other way to retain an animated cursor's animation (and perhaps
			// other icon/cursor attributes) when resizing the icon/cursor (CopyImage() doesn't
			// retain animation):
			if (!ExtractIcon_was_used)
			{
				DestroyIcon((HICON)hbitmap); // Destroy the original HICON.
				// Load a new one, but at the size newly calculated above.
				// Due to an apparent bug in Windows 9x (at least Win98se), the below call will probably
				// crash the program with a "divide error" if the specified aWidth and/or aHeight are
				// greater than 90.  Since I don't know whether this affects all versions of Windows 9x, and
				// all animated cursors, it seems best just to document it here and in the help file rather
				// than limiting the dimensions of .ani (and maybe .cur) files for certain operating systems.
				return (HBITMAP)LoadImage(NULL, aFilespec, aImageType, aWidth, aHeight, LR_LOADFROMFILE);
			}
		}
	}

	;int test=LR_COPYRETURNORG;

	HBITMAP hbitmap_new; // To hold the scaled image (if scaling is needed).
	if (pic) // IPicture method was used.
	{
		// The below statement is confirmed by having tested that DeleteObject(hbitmap) fails
		// if called after pic->Release():
		// "Copy the image. Necessary, because upon pic's release the handle is destroyed."
		// MSDN: CopyImage(): "[If either width or height] is zero, then the returned image will have the
		// same width/height as the original."
		// Note also that CopyImage() seems to provide better scaling quality than using MoveWindow()
		// (followed by redrawing the parent window) on the static control that contains it:
		hbitmap_new = (HBITMAP)CopyImage(hbitmap, IMAGE_BITMAP, aWidth, aHeight // We know it's IMAGE_BITMAP in this case.
			, (aWidth || aHeight) ? 0 : LR_COPYRETURNORG); // Produce original size if no scaling is needed.
		pic->Release();
		// No need to call DeleteObject(hbitmap), see above.
	}
	else // GDIPlus or a simple method such as LoadImage or ExtractIcon was used.
	{
		if (!aWidth && !aHeight) // No resizing needed.
			return hbitmap;
		// The following will also handle HICON/HCURSOR correctly if aImageType == IMAGE_ICON/CURSOR.
		// Also, LR_COPYRETURNORG|LR_COPYDELETEORG is used because it might allow the animation of
		// a cursor to be retained if the specified size happens to match the actual size of the
		// cursor.  This is because normally, it seems that CopyImage() omits cursor animation
		// from the new object.  MSDN: "LR_COPYRETURNORG returns the original hImage if it satisfies
		// the criteria for the copy—that is, correct dimensions and color depth—in which case the
		// LR_COPYDELETEORG flag is ignored. If this flag is not specified, a new object is always created."
		// KNOWN BUG: Calling CopyImage() when the source image is tiny and the destination width/height
		// is also small (e.g. 1) causes a divide-by-zero exception.
		// For example:
		//   Gui, Add, Pic, h1 w-1, bitmap 1x2.bmp  ; Crash (divide by zero)
		//   Gui, Add, Pic, h1 w-1, bitmap 2x3.bmp  ; Crash (divide by zero)
		// However, such sizes seem too rare to document or put in an exception handler for.
		hbitmap_new = (HBITMAP)CopyImage(hbitmap, aImageType, aWidth, aHeight, LR_COPYRETURNORG | LR_COPYDELETEORG);
		// Above's LR_COPYDELETEORG deletes the original to avoid cascading resource usage.  MSDN's
		// LoadImage() docs say:
		// "When you are finished using a bitmap, cursor, or icon you loaded without specifying the
		// LR_SHARED flag, you can release its associated memory by calling one of [the three functions]."
		// Therefore, it seems best to call the right function even though DeleteObject might work on
		// all of them on some or all current OSes.  UPDATE: Evidence indicates that DestroyIcon()
		// will also destroy cursors, probably because icons and cursors are literally identical in
		// every functional way.  One piece of evidence:
		//> No stack trace, but I know the exact source file and line where the call
		//> was made. But still, it is annoying when you see 'DestroyCursor' even though
		//> there is 'DestroyIcon'.
		// "Can't be helped. Icons and cursors are the same thing" (Tim Robinson (MVP, Windows SDK)).
		//
		// Finally, the reason this is important is that it eliminates one handle type
		// that we would otherwise have to track.  For example, if a gui window is destroyed and
		// and recreated multiple times, its bitmap and icon handles should all be destroyed each time.
		// Otherwise, resource usage would cascade upward until the script finally terminated, at
		// which time all such handles are freed automatically.
	}
	return hbitmap_new;
}


HBITMAP IconToBitmap(HICON ahIcon, bool aDestroyIcon)
// Converts HICON to an HBITMAP that has ahIcon's actual dimensions.
// The incoming ahIcon will be destroyed if the caller passes true for aDestroyIcon.
// Returns NULL on failure, in which case aDestroyIcon will still have taken effect.
// If the icon contains any transparent pixels, they will be mapped to CLR_NONE within
// the bitmap so that the caller can detect them.
{
	if (!ahIcon)
		return NULL;

	HBITMAP hbitmap = NULL;  // Set default.  This will be the value returned.

	HDC hdc_desktop = GetDC(HWND_DESKTOP);
	HDC hdc = CreateCompatibleDC(hdc_desktop); // Don't pass NULL since I think that would result in a monochrome bitmap.
	if (hdc)
	{
		ICONINFO ii;
		if (GetIconInfo(ahIcon, &ii))
		{
			BITMAP icon_bitmap;
			// Find out how big the icon is and create a bitmap compatible with the desktop DC (not the memory DC,
			// since its bits per pixel (color depth) is probably 1.
			if (GetObject(ii.hbmColor, sizeof(BITMAP), &icon_bitmap)
				&& (hbitmap = CreateCompatibleBitmap(hdc_desktop, icon_bitmap.bmWidth, icon_bitmap.bmHeight))) // Assign
			{
				// To retain maximum quality in case caller needs to resize the bitmap we return, convert the
				// icon to a bitmap that matches the icon's actual size:
				HGDIOBJ old_object = SelectObject(hdc, hbitmap);
				if (old_object) // Above succeeded.
				{
					// Use DrawIconEx() vs. DrawIcon() because someone said DrawIcon() always draws 32x32
					// regardless of the icon's actual size.
					// If it's ever needed, this can be extended so that the caller can pass in a background
					// color to use in place of any transparent pixels within the icon (apparently, DrawIconEx()
					// skips over transparent pixels in the icon when drawing to the DC and its bitmap):
					RECT rect = {0, 0, icon_bitmap.bmWidth, icon_bitmap.bmHeight}; // Left, top, right, bottom.
					HBRUSH hbrush = CreateSolidBrush(CLR_DEFAULT);
					FillRect(hdc, &rect, hbrush);
					DeleteObject(hbrush);
					// Probably something tried and abandoned: FillRect(hdc, &rect, (HBRUSH)GetStockObject(NULL_BRUSH));
					DrawIconEx(hdc, 0, 0, ahIcon, icon_bitmap.bmWidth, icon_bitmap.bmHeight, 0, NULL, DI_NORMAL);
					// Debug: Find out properties of new bitmap.
					//BITMAP b;
					//GetObject(hbitmap, sizeof(BITMAP), &b);
					SelectObject(hdc, old_object); // Might be needed (prior to deleting hdc) to prevent memory leak.
				}
			}
			// It's our reponsibility to delete these two when they're no longer needed:
			DeleteObject(ii.hbmColor);
			DeleteObject(ii.hbmMask);
		}
		DeleteDC(hdc);
	}
	ReleaseDC(HWND_DESKTOP, hdc_desktop);
	if (aDestroyIcon)
		DestroyIcon(ahIcon);
	return hbitmap;
}



int WINAPI ImageTest(int a)
{
	return a + a;
}
// ResultType Line::ImageSearch(int aLeft, int aTop, int aRight, int aBottom, char *aImageFile)
char* WINAPI ImageSearch(int aLeft, int aTop, int aRight, int aBottom, char *aImageFile)
// Author: ImageSearch was created by Aurelian Maga.
{
	// Many of the following sections are similar to those in PixelSearch(), so they should be
	// maintained together.
	//Var *output_var_x = ARGVAR1;  // Ok if NULL. RAW wouldn't be safe because load-time validation actually
	//Var *output_var_y = ARGVAR2;  // requires a minimum of zero parameters so that the output-vars can be optional.

	// Set default results, both ErrorLevel and output variables, in case of early return:
	//g_ErrorLevel->Assign(ERRORLEVEL_ERROR2);  // 2 means error other than "image not found".
	//if (output_var_x)
	//	output_var_x->Assign();  // Init to empty string regardless of whether we succeed here.
	//if (output_var_y)
	//	output_var_y->Assign(); // Same.
	RECT rect = {0}; // Set default (for CoordMode == "screen").
	//if (!(g.CoordMode & COORD_MODE_PIXEL)) // Using relative vs. screen coordinates.
	//{
	//	if (!GetWindowRect(GetForegroundWindow(), &rect))
	//		return OK; // Let ErrorLevel tell the story.
	//	aLeft   += rect.left;
	//	aTop    += rect.top;
	//	aRight  += rect.left;  // Add left vs. right because we're adjusting based on the position of the window.
	//	aBottom += rect.top;   // Same.
	//}

	// Options are done as asterisk+option to permit future expansion.
	// Set defaults to be possibly overridden by any specified options:
	int aVariation = 0;  // This is named aVariation vs. variation for use with the SET_COLOR_RANGE macro.
	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
	int icon_number = 0; // Zero means "load icon or bitmap (doesn't matter)".
	int width = 0, height = 0;
	// For icons, override the default to be 16x16 because that is what is sought 99% of the time.
	// This new default can be overridden by explicitly specifying w0 h0:
	char *cp = strrchr(aImageFile, '.');
	if (cp)
	{
		++cp;
		if (!(_stricmp(cp, "ico") && _stricmp(cp, "exe") && _stricmp(cp, "dll")))
			width = GetSystemMetrics(SM_CXSMICON), height = GetSystemMetrics(SM_CYSMICON);
	}

	char color_name[32], *dp;
	cp = omit_leading_whitespace(aImageFile); // But don't alter aImageFile yet in case it contains literal whitespace we want to retain.
	while (*cp == '*')
	{
		++cp;
		switch (toupper(*cp))
		{
		case 'W': width = ATOI(cp + 1); break;
		case 'H': height = ATOI(cp + 1); break;
		default:
			if (!_strnicmp(cp, "Icon", 4))
			{
				cp += 4;  // Now it's the character after the word.
				icon_number = ATOI(cp); // LoadPicture() correctly handles any negative value.
			}
			else if (!_strnicmp(cp, "Trans", 5))
			{
				cp += 5;  // Now it's the character after the word.
				// Isolate the color name/number for ColorNameToBGR():
				strlcpy(color_name, cp, sizeof(color_name));
				if (dp = StrChrAny(color_name, " \t")) // Find space or tab, if any.
					*dp = '\0';
				// Fix for v1.0.44.10: Treat trans_color as containing an RGB value (not BGR) so that it matches
				// the documented behavior.  In older versions, a specified color like "TransYellow" was wrong in
				// every way (inverted) and a specified numeric color like "Trans0xFFFFAA" was treated as BGR vs. RGB.
				trans_color = ColorNameToBGR(color_name);
				if (trans_color == CLR_NONE) // A matching color name was not found, so assume it's in hex format.
					// It seems strtol() automatically handles the optional leading "0x" if present:
					trans_color = strtol(color_name, NULL, 16);
					// if color_name did not contain something hex-numeric, black (0x00) will be assumed,
					// which seems okay given how rare such a problem would be.
				else
					trans_color = bgr_to_rgb(trans_color); // v1.0.44.10: See fix/comment above.

			}
			else // Assume it's a number since that's the only other asterisk-option.
			{
				aVariation = ATOI(cp); // Seems okay to support hex via ATOI because the space after the number is documented as being mandatory.
				if (aVariation < 0)
					aVariation = 0;
				if (aVariation > 255)
					aVariation = 255;
				// Note: because it's possible for filenames to start with a space (even though Explorer itself
				// won't let you create them that way), allow exactly one space between end of option and the
				// filename itself:
			}
		} // switch()
		if (   !(cp = StrChrAny(cp, " \t"))   ) // Find the first space or tab after the option.
			return "0"; //new
		//	return OK; // Bad option/format.  Let ErrorLevel tell the story.
		// Now it's the space or tab (if there is one) after the option letter.  Advance by exactly one character
		// because only one space or tab is considered the delimiter.  Any others are considered to be part of the
		// filename (though some or all OSes might simply ignore them or tolerate them as first-try match criteria).
		aImageFile = ++cp; // This should now point to another asterisk or the filename itself.
		// Above also serves to reset the filename to omit the option string whenever at least one asterisk-option is present.
		cp = omit_leading_whitespace(cp); // This is done to make it more tolerant of having more than one space/tab between options.
	}

	// Update: Transparency is now supported in icons by using the icon's mask.  In addition, an attempt
	// is made to support transparency in GIF, PNG, and possibly TIF files via the *Trans option, which
	// assumes that one color in the image is transparent.  In GIFs not loaded via GDIPlus, the transparent
	// color might always been seen as pure white, but when GDIPlus is used, it's probably always black
	// like it is in PNG -- however, this will not relied upon, at least not until confirmed.
	// OLDER/OBSOLETE comment kept for background:
	// For now, images that can't be loaded as bitmaps (icons and cursors) are not supported because most
	// icons have a transparent background or color present, which the image search routine here is
	// probably not equipped to handle (since the transparent color, when shown, typically reveals the
	// color of whatever is behind it; thus screen pixel color won't match image's pixel color).
	// So currently, only BMP and GIF seem to work reliably, though some of the other GDIPlus-supported
	// formats might work too.
	int image_type;
	HBITMAP hbitmap_image = LoadPicture(aImageFile, width, height, image_type, icon_number, false);
	// The comment marked OBSOLETE below is no longer true because the elimination of the high-byte via
	// 0x00FFFFFF seems to have fixed it.  But "true" is still not passed because that should increase
	// consistency when GIF/BMP/ICO files are used by a script on both Win9x and other OSs (since the
	// same loading method would be used via "false" for these formats across all OSes).
	// OBSOLETE: Must not pass "true" with the above because that causes bitmaps and gifs to be not found
	// by the search.  In other words, nothing works.  Obsolete comment: Pass "true" so that an attempt
	// will be made to load icons as bitmaps if GDIPlus is available.
	if (!hbitmap_image)
		return "0"; // new
	//	return OK; // Let ErrorLevel tell the story.

	HDC hdc = GetDC(NULL);
	if (!hdc)
	{
		DeleteObject(hbitmap_image);
		return "0"; // new
		// return OK; // Let ErrorLevel tell the story.
	}

	// From this point on, "goto end" will assume hdc and hbitmap_image are non-NULL, but that the below
	// might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	// label can detect them:
	HDC sdc = NULL;
	HBITMAP hbitmap_screen = NULL;
	LPCOLORREF image_pixel = NULL, screen_pixel = NULL, image_mask = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	bool found = false; // Must init here for use by "goto end".
    
	bool image_is_16bit;
	LONG image_width, image_height;

	if (image_type == IMAGE_ICON)
	{
		// Must be done prior to IconToBitmap() since it deletes (HICON)hbitmap_image:
		ICONINFO ii;
		if (GetIconInfo((HICON)hbitmap_image, &ii))
		{
			// If the icon is monochrome (black and white), ii.hbmMask will contain twice as many pixels as
			// are actually in the icon.  But since the top half of the pixels are the AND-mask, it seems
			// okay to get all the pixels given the rarity of monochrome icons.  This scenario should be
			// handled properly because: 1) the variables image_height and image_width will be overridden
			// further below with the correct icon dimensions; 2) Only the first half of the pixels within
			// the image_mask array will actually be referenced by the transparency checker in the loops,
			// and that first half is the AND-mask, which is the transparency part that is needed.  The
			// second half, the XOR part, is not needed and thus ignored.  Also note that if width/height
			// required the icon to be scaled, LoadPicture() has already done that directly to the icon,
			// so ii.hbmMask should already be scaled to match the size of the bitmap created later below.
			image_mask = getbits(ii.hbmMask, hdc, image_width, image_height, image_is_16bit, 1);
			DeleteObject(ii.hbmColor); // DeleteObject() probably handles NULL okay since few MSDN/other examples ever check for NULL.
			DeleteObject(ii.hbmMask);
		}
		if (   !(hbitmap_image = IconToBitmap((HICON)hbitmap_image, true))   )
			return "0"; //new
		//	return OK; // Let ErrorLevel tell the story.
	}

	if (   !(image_pixel = getbits(hbitmap_image, hdc, image_width, image_height, image_is_16bit))   )
		goto end;

	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft + 1;
	int search_height = aBottom - aTop + 1;
	if (   !(sdc = CreateCompatibleDC(hdc)) || !(hbitmap_screen = CreateCompatibleBitmap(hdc, search_width, search_height))   )
		goto end;

	if (   !(sdc_orig_select = SelectObject(sdc, hbitmap_screen))   )
		goto end;

	// Copy the pixels in the search-area of the screen into the DC to be searched:
	if (   !(BitBlt(sdc, 0, 0, search_width, search_height, hdc, aLeft, aTop, SRCCOPY))   )
		goto end;

	LONG screen_width, screen_height;
	bool screen_is_16bit;
	if (   !(screen_pixel = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit))   )
		goto end;

	LONG image_pixel_count = image_width * image_height;
	LONG screen_pixel_count = screen_width * screen_height;
	int i, j, k, x, y; // Declaring as "register" makes no performance difference with current compiler, so let the compiler choose which should be registers.

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if (image_is_16bit || screen_is_16bit)
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
		for (i = 0; i < screen_pixel_count; ++i)
			screen_pixel[i] &= 0x00F8F8F8; // Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
		for (i = 0; i < image_pixel_count; ++i)
			image_pixel[i] &= 0x00F8F8F8;  // Same.
	}

	// v1.0.44.03: The below is now done even for variation>0 mode so its results are consistent with those of
	// non-variation mode.  This is relied upon by variation=0 mode but now also by the following line in the
	// variation>0 section:
	//     || image_pixel[j] == trans_color
	// Without this change, there are cases where variation=0 would find a match but a higher variation
	// (for the same search) wouldn't. 
	for (i = 0; i < image_pixel_count; ++i)
		image_pixel[i] &= 0x00FFFFFF;

	// Search the specified region for the first occurrence of the image:
	if (aVariation < 1) // Caller wants an exact match.
	{
		// Concerning the following use of 0x00FFFFFF, the use of 0x00F8F8F8 above is related (both have high order byte 00).
		// The following needs to be done only when shades-of-variation mode isn't in effect because
		// shades-of-variation mode ignores the high-order byte due to its use of macros such as GetRValue().
		// This transformation incurs about a 15% performance decrease (percentage is fairly constant since
		// it is proportional to the search-region size, which tends to be much larger than the search-image and
		// is therefore the primary determination of how long the loops take). But it definitely helps find images
		// more successfully in some cases.  For example, if a PNG file is displayed in a GUI window, this
		// transformation allows certain bitmap search-images to be found via variation==0 when they otherwise
		// would require variation==1 (possibly the variation==1 success is just a side-effect of it
		// ignoring the high-order byte -- maybe a much higher variation would be needed if the high
		// order byte were also subject to the same shades-of-variation analysis as the other three bytes [RGB]).
		for (i = 0; i < screen_pixel_count; ++i)
			screen_pixel[i] &= 0x00FFFFFF;

		for (i = 0; i < screen_pixel_count; ++i)
		{
			// Unlike the variation-loop, the following one uses a first-pixel optimization to boost performance
			// by about 10% because it's only 3 extra comparisons and exact-match mode is probably used more often.
			// Before even checking whether the other adjacent pixels in the region match the image, ensure
			// the image does not extend past the right or bottom edges of the current part of the search region.
			// This is done for performance but more importantly to prevent partial matches at the edges of the
			// search region from being considered complete matches.
			// The following check is ordered for short-circuit performance.  In addition, image_mask, if
			// non-NULL, is used to determine which pixels are transparent within the image and thus should
			// match any color on the screen.
			if ((screen_pixel[i] == image_pixel[0] // A screen pixel has been found that matches the image's first pixel.
				|| image_mask && image_mask[0]     // Or: It's an icon's transparent pixel, which matches any color.
				|| image_pixel[0] == trans_color)  // This should be okay even if trans_color==CLR_NONE, since CLR_NONE should never occur naturally in the image.
				&& image_height <= screen_height - i/screen_width // Image is short enough to fit in the remaining rows of the search region.
				&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.
			{
				// Check if this candidate region -- which is a subset of the search region whose height and width
				// matches that of the image -- is a pixel-for-pixel match of the image.
				for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
				{
					if (!(found = (screen_pixel[k] == image_pixel[j] // At least one pixel doesn't match, so this candidate is discarded.
						|| image_mask && image_mask[j]      // Or: It's an icon's transparent pixel, which matches any color.
						|| image_pixel[j] == trans_color))) // This should be okay even if trans_color==CLR_NONE, since CLR none should never occur naturally in the image.
						break;
					if (++x < image_width) // We're still within the same row of the image, so just move on to the next screen pixel.
						++k;
					else // We're starting a new row of the image.
					{
						x = 0; // Return to the leftmost column of the image.
						++y;   // Move one row downward in the image.
						// Move to the next row within the current-candiate region (not the entire search region).
						// This is done by moving vertically downward from "i" (which is the upper-left pixel of the
						// current-candidate region) by "y" rows.
						k = i + y*screen_width; // Verified correct.
					}
				}
				if (found) // Complete match found.
					break;
			}
		}
	}
	else // Allow colors to vary by aVariation shades; i.e. approximate match is okay.
	{
		// The following section is part of the first-pixel-check optimization that improves performance by
		// 15% or more depending on where and whether a match is found.  This section and one the follows
		// later is commented out to reduce code size.
		// Set high/low range for the first pixel of the image since it is the pixel most often checked
		// (i.e. for performance).
		//BYTE search_red1 = GetBValue(image_pixel[0]);  // Because it's RGB vs. BGR, the B value is fetched, not R (though it doesn't matter as long as everything is internally consistent here).
		//BYTE search_green1 = GetGValue(image_pixel[0]);
		//BYTE search_blue1 = GetRValue(image_pixel[0]); // Same comment as above.
		//BYTE red_low1 = (aVariation > search_red1) ? 0 : search_red1 - aVariation;
		//BYTE green_low1 = (aVariation > search_green1) ? 0 : search_green1 - aVariation;
		//BYTE blue_low1 = (aVariation > search_blue1) ? 0 : search_blue1 - aVariation;
		//BYTE red_high1 = (aVariation > 0xFF - search_red1) ? 0xFF : search_red1 + aVariation;
		//BYTE green_high1 = (aVariation > 0xFF - search_green1) ? 0xFF : search_green1 + aVariation;
		//BYTE blue_high1 = (aVariation > 0xFF - search_blue1) ? 0xFF : search_blue1 + aVariation;
		// Above relies on the fact that the 16-bit conversion higher above was already done because like
		// in PixelSearch, it seems more appropriate to do the 16-bit conversion prior to setting the range
		// of high and low colors (vs. than applying 0xF8 to each of the high/low values individually).

		BYTE red, green, blue;
		BYTE search_red, search_green, search_blue;
		BYTE red_low, green_low, blue_low, red_high, green_high, blue_high;

		// The following loop is very similar to its counterpart above that finds an exact match, so maintain
		// them together and see above for more detailed comments about it.
		for (i = 0; i < screen_pixel_count; ++i)
		{
			// The following is commented out to trade code size reduction for performance (see comment above).
			//red = GetBValue(screen_pixel[i]);   // Because it's RGB vs. BGR, the B value is fetched, not R (though it doesn't matter as long as everything is internally consistent here).
			//green = GetGValue(screen_pixel[i]);
			//blue = GetRValue(screen_pixel[i]);
			//if ((red >= red_low1 && red <= red_high1
			//	&& green >= green_low1 && green <= green_high1
			//	&& blue >= blue_low1 && blue <= blue_high1 // All three color components are a match, so this screen pixel matches the image's first pixel.
			//		|| image_mask && image_mask[0]         // Or: It's an icon's transparent pixel, which matches any color.
			//		|| image_pixel[0] == trans_color)      // This should be okay even if trans_color==CLR_NONE, since CLR none should never occur naturally in the image.
			//	&& image_height <= screen_height - i/screen_width // Image is short enough to fit in the remaining rows of the search region.
			//	&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.
			
			// Instead of the above, only this abbreviated check is done:
			if (image_height <= screen_height - i/screen_width    // Image is short enough to fit in the remaining rows of the search region.
				&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.
			{
				// Since the first pixel is a match, check the other pixels.
				for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
				{
   					search_red = GetBValue(image_pixel[j]);
	   				search_green = GetGValue(image_pixel[j]);
		   			search_blue = GetRValue(image_pixel[j]);
					SET_COLOR_RANGE
   					red = GetBValue(screen_pixel[k]);
	   				green = GetGValue(screen_pixel[k]);
		   			blue = GetRValue(screen_pixel[k]);

					if (!(found = red >= red_low && red <= red_high
						&& green >= green_low && green <= green_high
                        && blue >= blue_low && blue <= blue_high
							|| image_mask && image_mask[j]     // Or: It's an icon's transparent pixel, which matches any color.
							|| image_pixel[j] == trans_color)) // This should be okay even if trans_color==CLR_NONE, since CLR_NONE should never occur naturally in the image.
						break; // At least one pixel doesn't match, so this candidate is discarded.
					if (++x < image_width) // We're still within the same row of the image, so just move on to the next screen pixel.
						++k;
					else // We're starting a new row of the image.
					{
						x = 0; // Return to the leftmost column of the image.
						++y;   // Move one row downward in the image.
						k = i + y*screen_width; // Verified correct.
					}
				}
				if (found) // Complete match found.
					break;
			}
		}
	}

	//if (!found) // Must override ErrorLevel to its new value prior to the label below.
	//	g_ErrorLevel->Assign(ERRORLEVEL_ERROR); // "1" indicates search completed okay, but didn't find it.

end:
	// If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
	// clean up then return.
	ReleaseDC(NULL, hdc);
	DeleteObject(hbitmap_image);
	if (sdc)
	{
		if (sdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
			SelectObject(sdc, sdc_orig_select); // Probably necessary to prevent memory leak.
		DeleteDC(sdc);
	}
	if (hbitmap_screen)
		DeleteObject(hbitmap_screen);
	if (image_pixel)
		free(image_pixel);
	if (image_mask)
		free(image_mask);
	if (screen_pixel)
		free(screen_pixel);

	if (!found) // Let ErrorLevel, which is either "1" or "2" as set earlier, tell the story.
			return "0";

	// Otherwise, success.  Calculate xpos and ypos of where the match was found and adjust
	// coords to make them relative to the position of the target window (rect will contain
	// zeroes if this doesn't need to be done):
	//if (output_var_x && !output_var_x->Assign((aLeft + i%screen_width) - rect.left))
	//	return FAIL;
	//if (output_var_y && !output_var_y->Assign((aTop + i/screen_width) - rect.top))
	//	return FAIL;

    int locx,locy;

	//return g_ErrorLevel->Assign(ERRORLEVEL_NONE); // Indicate success.
	if (found)
	{
		locx = (aLeft + i%screen_width) - rect.left;
		locy = (aTop + i/screen_width) - rect.top;
//		printf("\nFOUND!!!!%d   %d",locx,locy);
		sprintf_s(answer,"1|%d|%d|%d|%d",locx,locy,image_width,image_height);
		return answer;
		//return "ZZ";
	}
	return "0";

}



