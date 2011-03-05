/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  fcl.h
 *  File Cache Library header file
 *
 *  (C) Copyright 2010 - 2011 Olivier Delhomme
 *  e-mail : olivier.delhomme@free.fr
 *  URL    : https://gna.org/projects/fcl/
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
 *
 * This library is intented to manage the file at a level that allow one
 * to edit binary files directly without bothering with the memory issues
 * and such.
 */

#ifndef _LIBFCL_H_
#define _LIBFCL_H_

#include "config.h"

#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <string.h>

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
 * @def LIBFCL_MODE_READ
 * Mode to open the file. In this mode, the file will be opened in read only
 * The library is not supposed to manage any buffers in this mode
 *
 * @def LIBFCL_MODE_WRITE
 * Mode to open the file. In this mode, the file is opened for writing and
 * reading (sort of appending). The file may be created if it does not exists.
 * The library will manage buffers and such.
 *
 * @def LIBFCL_MODE_CREATE
 * Mode to open a file. In this mode, the file is created. If an existing file
 * already exists it is replaced by the new one.
 */
#define LIBFCL_MODE_READ 0
#define LIBFCL_MODE_WRITE 2
#define LIBFCL_MODE_CREATE 4


/**
 * @struct fcl_file_t
 * Structure that contains all the definitions needed by the library for a
 * file.
 *
 * The sequence in fcl_file_t is ordered. The order is done with the offset of
 * the fcl_buf_t structure. The buffers in the sequence are modified buffers
 * only (at first at least)
 */
typedef struct
{
    gchar *name;                   /**< Name for the file                 */
    gint mode;                     /**< Mode in which the file was opened */
    goffset real_size;             /**< Actual size of the file           */
    GFile *the_file;               /**< The corresponding GFile           */
    GFileInputStream *in_stream;   /**< Stream used for reading           */
    GFileOutputStream *out_stream; /**< Stream used for writing           */
    GSequence *sequence;           /**< Sequence of buffers (fcl_buf_t)   */
} fcl_file_t;


/**
 * @struct fcl_buf_t
 * Structure that acts as a buffer
 */
typedef struct
{
    goffset offset;      /** Offset of the buffer (aligned with LIBFCL_BUF_SIZE) */
    goffset real_offset; /** Real offset of the buffer (calculated each time)    */
    gsize size;          /** Size of the buffer                                  */
    guchar *data;        /** The buffer (if any)                                 */
    gboolean in_seq;     /** Says wether the buffer is in the sequence or not    */
} fcl_buf_t;


/**
 * @def LIBFCL_MAX_BUF_SIZE
 * Maximum buffer size that the library handles (This value is 2^20 as this was
 * the total amount of memory that my Atari 1040 ST had !). I don't know if this
 * is usefull in any manner... ?
 *
 * @def LIBFCL_BUF_SIZE
 * Default buffer size
 */
#define LIBFCL_MAX_BUF_SIZE 1048576
#define LIBFCL_BUF_SIZE 65536

/**
 * Public part of the library
 */

/**
 * This function initializes the library it has to invoked first
 */
extern void libfcl_initialize(void);


/**
 * Opens a file. Nothing is performed on it.
 * @param path : the path of the file to be opened
 * @param mode : the mode to open the file (LIBFCL_MODE_READ, LIBFCL_MODE_WRITE,
 *               LIBFCL_MODE_CREATE).
 * @return a correctly filled fcl_file_t structure that represents the file
 */
extern fcl_file_t *fcl_open_file(gchar *path, gint mode);


/**
 * This function closes a fcl_file_t
 * @param the fcl_file_t to close
 */
extern void fcl_close_file(fcl_file_t *a_file);


/**
 * This function reads an fcl_buf_t buffer from an fcl_file_t
 * @param a_file : the fcl_file_t file from which we want to read size bytes
 * @param position : the position where we want to read bytes
 * @param[in,out] size_pointer : the number of bytes we want to read. If this
 *                               value is higher than LIBFCL_MAX_BUF_SIZE the
 *                               function returns NULL. The value indicates the
 *                               real size of the data read hence, the real size
 *                               of the returned buffer
 * @return If everything is ok a filled guchar *buffer, NULL otherwise !
 */
extern guchar *fcl_read_bytes(fcl_file_t *a_file, goffset position, gsize *size_pointer);


/**
 * This function overwrites data in the file (size bytes of data at 'position'
 * in the file).
 * @warning it does do not writes to disk directly. It only overwrites correctly
 * the data into the file structure.
 * @param a_file : the fcl_file_t file to which we want to write size bytes
 * @param data : data to be overwritten in the file
 * @param position : position where to begin overwriting in the file
 * @param[in,out] size_pointer : size of the data to overwrite. returns the real
 *                               size of the data overwritten.
 * @return True If everything is ok, false otherwise
 */
extern gboolean fcl_overwrite_bytes(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer);


/**
 * This function inserts data in the file (size bytes of data at 'position'
 * in the file).
 * @warning it does do not writes to disk directly. It only inserts correctly
 * the data into the file structure.
 * @param a_file : the fcl_file_t file to which we want to write size bytes
 * @param data : data to be overwritten in the file
 * @param position : position where to begin overwriting in the file
 * @param size : size of the data to overwrite. As the insertion should never
 *               be less than size (or 0) one may consider that if the function
 *               retuned TRUE, size bytes were effectively inserted.
 * @return True If everything is ok, false otherwise
 */
extern gboolean fcl_insert_bytes(fcl_file_t *a_file, guchar *data, goffset position, gsize size);


/**
 * Deletes bytes in the buffers.
 * @param a_file : the fcl_file_t file to which we want to deleted size bytes
 * @param position : position where to delete size_pointer bytes in the file
 * @param[in,out] size_pointer : number of bytes to be deleted. Returns the
 * number of bytes effectively deleted (may be less than requested)
 * @return True if everything is ok, false otherwise.
 */
extern gboolean fcl_delete_bytes(fcl_file_t *a_file, goffset position, gsize *size_pointer);


/******************************************************************************/
/*********************************** Buffers **********************************/

/**
 * Prints a buffer data (exactly 'size' bytes)
 * @todo : print UTF8 encoded values
 * @param data : buffer data to be printed
 * @param size : number of bytes to prints (from data)
 * @param EOL : prints an End Of Line if TRUE, nothing if FALSE
 */
extern void fcl_print_data(guchar *data, gsize size, gboolean EOL);


#endif /* _LIBFCL_H_ */
