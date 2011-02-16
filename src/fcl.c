/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  fcl.c
 *  File Cache Library
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
/** @file fcl.c
 * Main library file
 */
/**
 * @author Olivier DELHOMME,
 * @version 0.0.1
 * @date 2010
 */
#include "fcl.h"

static fcl_buf_t *new_fcl_buf_t(goffset position, gsize size, gint8 buf_type,  gint8 buf_where);
static void destroy_fcl_buf_t(fcl_buf_t *buffer);

static fcl_file_t *new_fcl_file_t(gchar *path, gint mode);
static goffset get_gfile_file_size(GFile *the_file);
static gint cmp_offset_value(gconstpointer a, gconstpointer b, gpointer user_data);
static gint buffers_overlaps(fcl_buf_t *buffer1, fcl_buf_t *buffer2);


/******************************************************************************/
/********************************* Public API *********************************/
/******************************************************************************/

/**
 * This function initializes the library it has to invoked first
 */
void libfcl_initialize(void)
{
    g_type_init();
}


/**
 * Opens a file. Nothing is performed on it.
 * @param path : the path of the file to be opened
 * @param mode : the mode to open the file (LIBFCL_MODE_READ, LIBFCL_MODE_WRITE,
 *               LIBFCL_MODE_CREATE).
 * @return a correctly filled fcl_file_t structure that represents the file
 */
fcl_file_t *fcl_open_file(gchar *path, gint mode)
{

    fcl_file_t *a_file = NULL;

    switch (mode)
        {
            case LIBFCL_MODE_READ:
                a_file = new_fcl_file_t(path, mode);
                a_file->out_stream = NULL;
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                return a_file;
            break;

            case LIBFCL_MODE_WRITE:
                a_file = new_fcl_file_t(path, mode);
                a_file->out_stream = g_file_append_to(a_file->the_file, G_FILE_CREATE_NONE, NULL, NULL);
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                return a_file;
            break;

            case LIBFCL_MODE_CREATE:
                a_file = new_fcl_file_t(path, mode);
                a_file->out_stream = g_file_replace(a_file->the_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                return a_file;
            break;

            default:
                return NULL;
            break;
        }
}


/**
 * This function closes a fcl_file_t
 * @param the fcl_file_t to close
 */
void fcl_close_file(fcl_file_t *a_file)
{
    g_free(a_file->name);

    if (a_file->in_stream != NULL)
        {
            g_input_stream_close(G_INPUT_STREAM(a_file->in_stream), NULL, NULL);
        }

    if (a_file->out_stream != NULL)
        {
            g_output_stream_close(G_OUTPUT_STREAM(a_file->out_stream), NULL, NULL);
        }

    if (a_file->the_file != NULL)
        {
            g_object_unref(a_file->the_file);
        }

    if (a_file->sequence != NULL)
        {
            g_sequence_free(a_file->sequence);
        }

    g_free(a_file);
}


/**
 * This function reads an fcl_buf_t buffer from an fcl_file_t
 * @param a_file : the fcl_file_t file from which we want to read size bytes
 * @param position : the position where we want to read bytes
 * @param size : the number of bytes we want to read. If this value is higher
 *               than LIBFCL_MAX_BUF_SIZE the function returns NULL
 * @return If everything is ok a filled fcl_buf_t buffer, NULL otherwise !
 */
fcl_buf_t *fcl_read_bytes(fcl_file_t *a_file, goffset position, gsize size)
{

    fcl_buf_t *a_buffer = NULL;  /**< the fcl_buf_t structure that will be returned */
    gssize read  = 0;            /**< Number of bytes effectively read              */

    if (size > LIBFCL_MAX_BUF_SIZE)
        {
            return NULL;
        }

    /* Defining a new buffer */
    a_buffer = new_fcl_buf_t();

    if (a_buffer->buffer == NULL)
        {
            /* A memory problem ? */
            destroy_fcl_buf_t(a_buffer);
            return NULL;
        }


    if (a_file->mode == LIBFCL_MODE_READ)
        {
            /* direct read, no cache */
            g_seekable_seek(G_SEEKABLE(a_file->in_stream), position, G_SEEK_SET, NULL, NULL);
            read = g_input_stream_read(G_INPUT_STREAM(a_file->in_stream), a_buffer->data, size, NULL, NULL);

            if (read != size && read > 0)
                {
                    /* Not everything was read (end of file ?) */
                    a_buffer->buf_size = read;
                }
            else if (read < 0)
                {
                    /* something went wrong !! */
                    destroy_fcl_buf_t(a_buffer);
                    return NULL;
                }

            return a_buffer;
        }
    else if (a_file->mode == LIBFCL_MODE_CREATE)
        {
            /* all cache, no direct read */
            destroy_fcl_buf_t(a_buffer);
            return NULL;
        }
    else if (a_file->mode == LIBFCL_MODE_WRITE)
        {
            /* mixed mode : some are in the real file on disk and some are on the cache */
            destroy_fcl_buf_t(a_buffer);
            return NULL;
        }
    else
        {
            destroy_fcl_buf_t(a_buffer);
            return NULL;
        }
}


/**
 * This function writes a previously filled fcl_buf_t * buffer structure in the
 * file.
 * @warning it does do writes to disk directly. It only inserts correctly the
 * buffer into the file structure.
 * @param a_file : the fcl_file_t file to which we want to write size bytes
 * @param a_buffer : The fcl_buf_t * buffer to write to the file
 * @return True If everything is ok, False Otherwise
 */
extern gboolean fcl_write_bytes(fcl_file_t *a_file, fcl_buf_t *a_buffer)
{
    return FALSE;
}


/******************************************************************************/
/****************************** Intern functions ******************************/
/******************************************************************************/

/**
 * Creates a new empty buffer
 */
static fcl_buf_t *new_fcl_buf_t()
{
    fcl_buf_t *a_buffer = NULL;  /**< the fcl_buf_t structure that will be returned */

    a_buffer = (fcl_buf_t *) g_malloc0 (sizeof(fcl_buf_t));

    a_buffer->offset = 0;
    a_buffer->buf_size = LIBFCL_BUF_SIZE;
    a_buffer->data = (guchar *) g_malloc0 (a_buffer->buf_size * sizeof(guchar));

    return a_buffer;
}


/**
 * Destroys a buffer (and the data in it !)
 */
static void destroy_fcl_buf_t(fcl_buf_t *buffer)
{
    if (buffer != NULL)
        {
            if (buffer->data != NULL)
                {
                    g_free(buffer->data);
                }

            g_free(buffer);
        }
}


/**
 * Creates a new fcl_file_t structure from parameters
 * @param path : path to the file (filename included).
 * @param mode : mode in which one wants to open the file
 * @return a newly initialiazed empty fcl_file_t structure
 */
static fcl_file_t *new_fcl_file_t(gchar *path, gint mode)
{

    fcl_file_t *a_file = NULL;

    a_file = (fcl_file_t *) g_malloc0 (sizeof(fcl_file_t));

    a_file->the_file = g_file_new_for_path(path);
    a_file->name = g_strdup(path);
    a_file->mode = mode;
    a_file->real_size = get_gfile_file_size(a_file->the_file);
    a_file->in_stream = NULL;
    a_file->out_stream = NULL;
    a_file->sequence = g_sequence_new(NULL); /** @todo have a destroy function here */

    return a_file;
}


/**
 * Gets the file size of a Gfile object throught the GFileInfo query
 * @param the_file : a GFile object
 * @return a positive goffset number that represents the file size or -1 if
 *         something went wrong
 */
static goffset get_gfile_file_size(GFile *the_file)
{
    GFileInfo *file_info = NULL;
    goffset size = -1;

    if (the_file != NULL)
        {
            file_info = g_file_query_info(the_file, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
            size = g_file_info_get_size(file_info);
            g_object_unref(file_info);

            return size;
        }
    else
        {
            return -1;
        }
}


/**
 * Compare function for the sequence. Compares buffers a and b. In fact it
 * compares the offset of a and the offset of b
 * @param a : a fcl_but_t * buffer
 * @param b : a fcl_but_t * buffer
 * @param user_data : not used. Can be whatever the user want, even NULL
 */
static gint cmp_offset_value(gconstpointer a, gconstpointer b, gpointer user_data)
{
    fcl_buf_t *buf1 = (fcl_buf_t *) a;
    fcl_buf_t *buf2 = (fcl_buf_t *) b;


    if (buf1 != NULL && buf2 != NULL)
        {
            if (buf1->offset < buf2->offset)
                {
                    return -1;
                }
            if (buf1->offset > buf2->offset)
                {
                    return +1;
                }
            if (buf1->offset == buf2->offset)
                {
                    return 0;
                }
        }
    else if (buf1 == NULL && buf2 == NULL)
        {
            return 0;
        }
    else if (buf1 == NULL)
        {
            return -1;
        }
    else if (buf2 == NULL)
        {
            return +1;
        }
}


/**
 * Overlap function
 * An assertion is made such that buffer1->offset < buffer2->offset
 * @param buffer1 : a fcl_but_t * buffer
 * @param buffer2 : a fcl_but_t * buffer
 * @return 0 : the buffers does dot overlaps.
 *         1 : buffer overlaps as follow : 1111111111
 *                                             2222222222
 *         2 : buffer overlaps as follow : 1111111111
 *                                             2222
 *         3 : the buffer is at the limits : 11111222222
 */
static gint buffers_overlaps(fcl_buf_t *buffer1, fcl_buf_t *buffer2)
{

    if (buffer1 != NULL && buffer2 != NULL)
        {
            if ((buffer1->offset + buffer1->buf_size) > buffer2->offset && (buffer1->offset + buffer1->buf_size) < (buffer2->offset + buffer2->buf_size))
                {
                    return 1;
                }
            else if ((buffer1->offset + buffer1->buf_size) > buffer2->offset && (buffer1->offset + buffer1->buf_size) > (buffer2->offset + buffer2->buf_size))
                {
                    return 2;
                }
            else if ((buffer1->offset + buffer1->buf_size) == buffer2->offset)
                {
                    return 3;
                }
            else
                {
                    return 0;
                }
        }
    else
        {
            return FALSE;
        }
}
