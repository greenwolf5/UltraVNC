/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


//
// ZlibInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib on the fly.
//

#ifndef __RDR_ZLIBINSTREAM_H__
#define __RDR_ZLIBINSTREAM_H__

#pragma once

#include "InStream.h"

struct z_stream_s;

namespace rdr {

  class ZlibInStream : public InStream {

  public:

    ZlibInStream(int bufSize=0);
    virtual ~ZlibInStream();

    void setUnderlying(InStream* is, int bytesIn);
    void reset();
    int pos();

  private:

    int overrun(int itemSize, int nItems);
    void decompress();

    InStream* underlying;
    int bufSize;
    int offset;
    z_stream_s* zs;
    int bytesIn;
    U8* start;
  };

} // end of namespace rdr

#endif
