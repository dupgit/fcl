/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  fcl.h
 *  File Cache Library header file
 *
 *  (C) Copyright 2010 Olivier Delhomme
 *  e-mail : olivier.delhomme@free.fr
 *  URL    : http://
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or  (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY  or  FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/** @file fcl.h
 * Header for the library
 */
#ifndef _LIBFCL_H_
#define _LIBFCL_H_

#include <glib.h>
#include <stdio.h>

#include "config.h"

/**
 * @def LIBFCL_DATE
 * defines fcl's creation date
 *
 * @def LIBFCL_AUTHORS
 * defines fcl's library main authors
 *
 * @def LIBFCL_LICENSE
 * defines fcl library's license (at least GPL v2)
 *
 * @def LIBFCL_VERSION
 * defines fcl library's current version and release date of this version
 * (00.00.20XX means a development version)
 */
#define LIBFCL_AUTHORS "Olivier DELHOMME"
#define LIBFCL_DATE "07 09 2010"
#define LIBFCL_LICENSE N_("GPL v2 or later")
#define LIBFCL_VERSION "0.0.0 (00.00.0000)"


/**
 * @def LIBFCL_BUF_DELETE
 * Used in buffers to indicate that the buffer is a deletion buffer
 *
 * @def LIBFCL_BUF_INSERT
 * Used to indicate that the buffer is to be inserted in the file at the given
 * position
 *
 * @def LIBFCL_BUF_OVERWRITE
 * Used to indicate that the buffer will overwrite the space in the file at the
 * given position
 */
#define LIBFCL_BUF_DELETE 16
#define LIBFCL_BUF_INSERT 32
#define LIBFCL_BUF_OVERWRITE 48


/**
 * @struct fcl_file_t
 * Structure that contains all the definitions needed by the library for a
 * file
 */
typedef struct
{
    gchar *name;               /**< Name for the file               */
    goffset real_size;         /**< Actual size of the file         */
    GFile *the_file;           /**< The corresponding GFile         */
    GInputStream *in_stream;   /**< Stream used for reading         */
    GOutputStream *out_stream; /**< Stream used for writing         */
    GSequence *sequence;       /**< Sequence of buffers (fcl_buf_t) */
} fcl_file_t;


/**
 * @struct buf_t
 * Structure that acts as buffer
 */
typedef struct
{
    goffset offset;     /**< Offset of the buffer */
    goffset buf_size;   /**< Size of the buffer   */
    gint8 buf_type;     /**< Type of the buffer   */
    guchar *buffer;     /**< The buffer (if any)  */
} fcl_buf_t;

endif /* _LIBFCL_H_ */
