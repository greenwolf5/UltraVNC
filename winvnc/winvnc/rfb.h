/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// rfb.h
// This includes the rfb spec header, the port numbers,
// the CARD type definitions and various useful macros.
//

#ifndef RFB_H__
#define RFB_H__

// Define the CARD* types as used in X11/Xmd.h

typedef unsigned long CARD32;
typedef unsigned short CARD16;
typedef short INT16;
typedef unsigned char  CARD8;

// Define the port number offsets
#define FLASH_PORT_OFFSET 5400
#define INCOMING_PORT_OFFSET 5500
#define HTTP_PORT_OFFSET 5800	// we don't use this in Venice
#define RFB_PORT_OFFSET 5900

#define PORT_TO_DISPLAY(p)  ( (p) - RFB_PORT_OFFSET )
#define HPORT_TO_DISPLAY(p) ( (p) - HTTP_PORT_OFFSET )
#define DISPLAY_TO_PORT(d)  ( (d) + RFB_PORT_OFFSET )
#define DISPLAY_TO_HPORT(d) ( (d) + HTTP_PORT_OFFSET )

#ifdef _Gii
#include <stdint.h>
#include <rfb/gii.h>
#endif

// include the protocol spec
#include <rfb/rfbproto.h>

// define some quick endian conversions
// change this if necessary
#define LITTLE_ENDIAN_HOST

#ifdef LITTLE_ENDIAN_HOST

#define Swap16IfLE(s) \
    ((CARD16) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))
#define Swap32IfLE(l) \
    ((CARD32) ((((l) & 0xff000000) >> 24) | \
     (((l) & 0x00ff0000) >> 8)  | \
	 (((l) & 0x0000ff00) << 8)  | \
	 (((l) & 0x000000ff) << 24)))

#else

#define Swap16IfLE(s) (s)
#define Swap32IfLE(l) (l)

#endif

// unconditional swaps
#define Swap16(s) \
    ((CARD16) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))
#define Swap32(l) \
    ((CARD32) ((((l) & 0xff000000) >> 24) | \
     (((l) & 0x00ff0000) >> 8)  | \
	 (((l) & 0x0000ff00) << 8)  | \
	 (((l) & 0x000000ff) << 24)))

#ifdef _Gii
#define Swap64IfLE(ll) \
    (*(char *)&endianTest ? ((((ll) & 0xff00000000000000LL) >> 56) | \
			     (((ll) & 0x00ff000000000000LL) >> 40)  | \
			     (((ll) & 0x0000ff0000000000LL) >> 24)  | \
			     (((ll) & 0x000000ff00000000LL) >> 8) | \
			     (((ll) & 0x00000000ff000000LL) << 8) | \
			     (((ll) & 0x0000000000ff0000LL) << 24)  | \
			     (((ll) & 0x000000000000ff00LL) << 40)  | \
			     (((ll) & 0x00000000000000ffLL) << 56))  : (ll))


#endif


#endif
