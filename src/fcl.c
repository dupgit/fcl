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

/** Private intern functions (please have a look at fcl.h for the public API
 *  functions definitions)
 */
static fcl_buf_t *new_fcl_buf_t(void);
static void destroy_fcl_buf_t(gpointer data);

static fcl_file_t *new_fcl_file_t(gchar *path, gint mode);
static goffset get_gfile_file_size(GFile *the_file);
static gint cmp_offset_value(gconstpointer a, gconstpointer b, gpointer user_data);
static gint buffers_overlaps(fcl_buf_t *buffer1, fcl_buf_t *buffer2);

static goffset buf_number(goffset position);
static goffset position_in_buffer(goffset position);

static fcl_buf_t *read_buffer_at_position(fcl_file_t *a_file, goffset position);
static void overwrite_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer);
static void inserts_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize size);

static void insert_buffer_in_sequence(fcl_file_t *a_file, fcl_buf_t *a_buffer);
static gboolean is_in_sequence(GSequence *seq, GSequenceIter *begin, GSequenceIter *end, fcl_buf_t *a_buffer);

static void print_message(const char *format, ...);


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
            print_message("Closing the input stream \n");
            g_input_stream_close(G_INPUT_STREAM(a_file->in_stream), NULL, NULL);
        }

    if (a_file->out_stream != NULL)
        {
            print_message("Closing the output stream\n");
            g_output_stream_close(G_OUTPUT_STREAM(a_file->out_stream), NULL, NULL);
        }

    if (a_file->the_file != NULL)
        {
            print_message("Unreference the file\n");
            g_object_unref(a_file->the_file);
        }

    if (a_file->sequence != NULL)
        {
            print_message("Freeing the sequence\n");
            g_sequence_free(a_file->sequence);
        }

    g_free(a_file);

    print_message("The file is closed.\n");
}


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
guchar *fcl_read_bytes(fcl_file_t *a_file, goffset position, gsize *size_pointer)
{

    fcl_buf_t *a_buffer = NULL;  /** the fcl_buf_t structure that will be returned     */
    guchar *data = NULL;         /** The data that is claimed (size bytes at position) */
    guchar *next_data = NULL;
    goffset offset = 0;          /** The offset in the data buffer                     */
    gsize real_size = 0;         /** Real size returned by the recursive call          */
    gsize size = 0;

    size = *size_pointer;

    print_message("fcl_read_bytes(%p, %ld, %ld)\n", a_file, position, size);

    if (size > LIBFCL_MAX_BUF_SIZE)
        {
            return NULL;
        }

    data = (guchar *) g_malloc0 (size * sizeof(guchar));

    a_buffer = read_buffer_at_position(a_file, position);

    offset = (position - a_buffer->real_offset);

    print_message("offset : %ld; size : %ld\n", offset, size);

    if (offset >= 0 && offset <= a_buffer->size)
        {
            if (a_buffer->size >= offset + size) /* The claimed data is all in the buffer */
                {
                    data = (guchar *) g_memdup(a_buffer->data + offset, size);
                    print_message("size : %ld\n", size);
                }
            else if (a_buffer->size != LIBFCL_BUF_SIZE) /* Not all the buffer was filled */
                {
                    size = a_buffer->size - offset;
                    if (size  > 0)
                        {
                            data = (guchar *) g_memdup(a_buffer->data + offset, size);
                        }
                }
            else
                {
                    /* claimed data is located in two different buffers at least */
                    data = (guchar *) g_memdup(a_buffer->data + offset, a_buffer->size - offset);
                    real_size = size - (a_buffer->size - offset);
                    next_data = fcl_read_bytes(a_file, a_buffer->real_offset + a_buffer->size, &real_size);
                    size = real_size + (a_buffer->size - offset);
                    print_message("size : %ld; real_size : %ld\n", size, real_size);
                    memcpy(data + (a_buffer->size - offset), next_data, real_size);
                }
        }
    else
        {
            data = NULL;
        }

    if (a_buffer->in_seq == FALSE)
        {
            destroy_fcl_buf_t((gpointer) a_buffer);
        }

    *size_pointer = size;

    return data;
}


/**
 * This function overwrites data in the file (size bytes of data at 'position'
 * in the file).
 * @warning it does do not writes to disk directly. It only overwrites correctly
 * the data into the file structure.
 * @param a_file : the fcl_file_t file to which we want to write size bytes
 * @param data : data to be overwritten in the file
 * @param position : position where to begin overwriting in the file
 * @param size : size of the data to overwrite
 * @return True If everything is ok, False Otherwise
 */
extern gboolean fcl_overwrite_bytes(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer)
{
    gsize size = 0;

    if (a_file->mode != LIBFCL_MODE_READ)
        {
            size = *size_pointer;

            overwrite_data_at_position(a_file, data, position, &size);

            *size_pointer = size;

            return TRUE;
        }
    else
        {
            fprintf(stderr, Q_("File is read-only, overwriting is prohibited\n"));
            return FALSE;
        }
}


/**
 * This function inserts data in the file (size bytes of data at 'position'
 * in the file).
 * @warning it does do not writes to disk directly. It only inserts correctly
 * the data into the file structure.
 * @param a_file : the fcl_file_t file to which we want to write size bytes
 * @param data : data to be overwritten in the file
 * @param position : position where to begin overwriting in the file
 * @param size : size of the data to overwrite
 * @return True If everything is ok, False Otherwise
 */
extern gboolean fcl_insert_bytes(fcl_file_t *a_file, guchar *data, goffset position, gsize size)
{
    if (a_file->mode != LIBFCL_MODE_READ)
        {
            inserts_data_at_position(a_file, data, position, size);
            return TRUE;
        }
    else
        {
            fprintf(stderr, Q_("File is read-only, overwriting is prohibited\n"));
            return FALSE;
        }
}


/******************************************************************************/
/*********************************** Buffers **********************************/

/**
 * Says wether the buffer structure exists and that the data buffer exists also
 * @param a_buffer the buffer to check
 */
extern gboolean fcl_buffer_exists(fcl_buf_t *a_buffer)
{
    if (a_buffer != NULL && a_buffer->data != NULL)
        {
            return TRUE;
        }
    else
        {
            return FALSE;
        }
}


/**
 * Prints a buffer data (exactly 'size' bytes)
 * @todo : print UTF8 encoded values
 * @param data : buffer data to be printed
 * @param size : number of bytes to prints (from data)
 * @param EOL : prints an End Of Line if TRUE, nothing if FALSE
 */
extern void fcl_print_data(guchar *data, gsize size, gboolean EOL)
{
    gsize i = 0;
    gunichar c;   /** a character to test */

    for (i = 0; i < size ; i++)
        {
            c = data[i];

            if (g_unichar_isprint(c))
                {
                    g_print("%c", c);
                }
            else
                {
                    g_print(".");
                }
        }

    if (EOL == TRUE)
        {
            g_print("\n");
        }

}


/******************************************************************************/
/****************************** Intern functions ******************************/
/******************************************************************************/

/**
 * Prints a message in case we are in debug stage
 * @param format the message to print in a vprintf format
 * @param ... the va_list : list of arguments to fill in the format
 */
static void print_message(const char *format, ...)
{
    va_list args;
    gchar *str = NULL;

    if (ENABLE_DEBUG)
        {
            va_start(args, format);
            str = g_strdup_vprintf(format, args);
            va_end(args);
            fprintf(stdout, "%s", str);
        }
}


/****************************** Buffers management ****************************/
/**
 * Calculates the number of the buffer which depends of the position in the file
 * @param position : position in the file
 * @return an offset that indicates the number of the buffer. The buffer itself
 * may then begin at LIBFCL_BUF_SIZE * returned number
 */
static goffset buf_number(goffset position)
{
    return (goffset) (position / LIBFCL_BUF_SIZE);
}

/**
 * Returns the position in the buffer
 * @param position : position in the file
 * @return an offset in the buffer
 */
static goffset position_in_buffer(goffset position)
{
    return (goffset) (position - buf_number(position) * LIBFCL_BUF_SIZE);
}


/**
 * Creates a new empty buffer
 */
static fcl_buf_t *new_fcl_buf_t(void)
{
    fcl_buf_t *a_buffer = NULL;  /**< the fcl_buf_t structure that will be returned */

    a_buffer = (fcl_buf_t *) g_malloc0 (sizeof(fcl_buf_t));

    a_buffer->offset = 0;
    a_buffer->real_offset = 0;
    a_buffer->size = LIBFCL_BUF_SIZE;
    a_buffer->data = (guchar *) g_malloc0 (LIBFCL_BUF_SIZE * sizeof(guchar));
    a_buffer->in_seq = FALSE;

    return a_buffer;
}


/**
 * This function cares to calculate the buffer->real_offset and all the values
 * of the buffer which should be considerated as valid when generating a new
 * buffer.
 */
static fcl_buf_t *read_buffer_at_position(fcl_file_t *a_file, goffset position)
{
    fcl_buf_t *a_buffer = NULL;  /** Buffer to be read                                   */
    gssize read  = 0;            /** Number of bytes effectively read                    */
    goffset real_position = 0;   /** Position in the file (in number of LIBFCL_BUF_SIZE) */
    goffset gap = 0;             /** gap between the edited buffers and the file         */
    GSequenceIter *begin = NULL;
    GSequenceIter *end = NULL;
    fcl_buf_t * seq_buf = NULL;
    gboolean ok = TRUE;

    print_message("read_buffer_at_position(%p, %ld) : ", a_file, position);

    if (a_file->sequence == NULL)
        {
             a_buffer = new_fcl_buf_t();

            /* No offset correction here because the sequence is empty */
            /* calculating the aligned real_position in the file       */
            real_position = (goffset) position / LIBFCL_BUF_SIZE;

            a_buffer->offset = real_position;
            a_buffer->real_offset = real_position * LIBFCL_BUF_SIZE;

            g_seekable_seek(G_SEEKABLE(a_file->in_stream), a_buffer->real_offset, G_SEEK_SET, NULL, NULL);
            read = g_input_stream_read(G_INPUT_STREAM(a_file->in_stream), a_buffer->data, LIBFCL_BUF_SIZE, NULL, NULL);

            a_buffer->size = read; /* size of what was read (it may be less than LIBFCL_BUF_SIZE) */

            print_message("%ld", read);
        }
    else
        {
            /**
             * Calculate the gap beetween file and buffers in the sequence
             * begining at 0 until the corresponding buffer has been found or
             * the value is higher then the position we want to read
             */
            gap = 0;
            real_position = 0;
            ok = TRUE;
            begin = g_sequence_get_begin_iter(a_file->sequence);
            end = g_sequence_get_end_iter(a_file->sequence);

            seq_buf = g_sequence_get(begin);

            while ((real_position + seq_buf->size) < position && ok == TRUE)
                {
                    real_position = real_position + seq_buf->size;
                    gap = gap + (seq_buf->size - LIBFCL_BUF_SIZE);
                    if (begin == end)
                        {
                            ok = FALSE;
                        }
                    else
                        {
                            begin = g_sequence_iter_next(begin);
                            seq_buf = g_sequence_get(begin);
                        }
                }

            print_message("\nreal_position : %ld; seq_buf->size : %ld; position : %ld\n", real_position,  seq_buf->size, position);

            if ((real_position + seq_buf->size) > position)
                {
                    /* buffer exists */
                    print_message("%ld == %ld ?", seq_buf->real_offset, real_position);
                    seq_buf->real_offset = real_position;
                    a_buffer = seq_buf;
                }
            else
                {
                    /* buffer does not exists or is not found in the sequence */
                    a_buffer = new_fcl_buf_t();

                    a_buffer->offset = (position - gap) / LIBFCL_BUF_SIZE;
                    a_buffer->real_offset = (position - gap);

                    g_seekable_seek(G_SEEKABLE(a_file->in_stream), a_buffer->offset, G_SEEK_SET, NULL, NULL);
                    read = g_input_stream_read(G_INPUT_STREAM(a_file->in_stream), a_buffer->data, LIBFCL_BUF_SIZE, NULL, NULL);

                    a_buffer->size = read;
                }
        }
    print_message("\n");

    print_message("offset \t: %ld\nreal_offset \t: %ld\nsize \t: %ld\n", a_buffer->offset, a_buffer->real_offset, a_buffer->size);


    return a_buffer;
}


/**
 * Says wether the buffer is in the sequence or not
 * @warning this function is recursive
 */
static gboolean is_in_sequence(GSequence *seq, GSequenceIter *begin, GSequenceIter *end, fcl_buf_t *a_buffer)
{
    fcl_buf_t *seq_buffer = NULL;
    GSequenceIter *mid_point = NULL;
    gint pos_begin = 0;
    gint pos_end = 0;

    pos_begin = g_sequence_iter_get_position(begin);
    pos_end = g_sequence_iter_get_position(end);

    if (pos_end < pos_begin)
        {
            return FALSE;
        }

    if (pos_begin == pos_end)
        {
            seq_buffer = g_sequence_get(begin);

            if (cmp_offset_value(a_buffer, seq_buffer, NULL) == 0)
                {
                    return TRUE;
                }
            else
                {
                    return FALSE;
                }
        }
    else
        {
            mid_point = g_sequence_range_get_midpoint(begin, end);
            seq_buffer = g_sequence_get(mid_point);

            if (cmp_offset_value(a_buffer, seq_buffer, NULL) == 0)
                {
                    return TRUE;
                }
            else if (cmp_offset_value(a_buffer, seq_buffer, NULL) == +1)
                {
                    return is_in_sequence(seq, mid_point, end, a_buffer);
                }
            else
                {
                    return is_in_sequence(seq, begin, mid_point, a_buffer);
                }
        }
}


/**
 * Inserts a buffer in the sequence (only if it is not already in it !)
 */
static void insert_buffer_in_sequence(fcl_file_t *a_file, fcl_buf_t *a_buffer)
{
    GSequenceIter *begin = NULL;
    GSequenceIter *end = NULL;


    if (a_file != NULL && a_buffer != NULL)
        {
            if (a_file->sequence == NULL)
                {
                    a_buffer->in_seq = TRUE;
                    a_file->sequence = g_sequence_new(destroy_fcl_buf_t);
                    g_sequence_append(a_file->sequence, a_buffer);
                    print_message("Inserted buffer : %p\n", a_buffer);
                }
            else
                {
                    begin = g_sequence_get_begin_iter(a_file->sequence);
                    end = g_sequence_get_end_iter(a_file->sequence);

                    if (is_in_sequence(a_file->sequence, begin, end, a_buffer) == FALSE)
                        {
                            a_buffer->in_seq = TRUE;
                            g_sequence_insert_sorted(a_file->sequence, a_buffer, cmp_offset_value, NULL);
                            print_message("Inserted buffer : %p\n", a_buffer);
                        }
                }
        }
}


/**
 * Overwite a buffer in place
 * @warning this function is recursive
 */
static void overwrite_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer)
{
    fcl_buf_t *a_buffer = NULL;  /** Buffer to be overwritten  */
    goffset buf_position = 0;    /** Position in the buffer    */
    gsize reste = 0;
    gsize size = 0;


    size = *size_pointer;

    print_message("overwrite_data_at_position(%p, %p, %ld, %ld)\n", a_file, data, position, size);

    a_buffer = read_buffer_at_position(a_file, position);

    buf_position = (position - a_buffer->real_offset);
    print_message("buf_position : %ld (position : %ld, real_offset : %ld)\n", buf_position, position, a_buffer->real_offset);

    if (buf_position >= 0 && (buf_position + size) <= a_buffer->size)
        {
            memcpy(a_buffer->data + buf_position, data, size);
            insert_buffer_in_sequence(a_file, a_buffer);
        }
    else if (buf_position + size > a_buffer->size && a_buffer->size < LIBFCL_BUF_SIZE)
        {
            fprintf(stderr, Q_("Overwritting outside of the file is not possible !\n"));
            memcpy(a_buffer->data + buf_position, data, a_buffer->size - buf_position);
            insert_buffer_in_sequence(a_file, a_buffer);
            size = a_buffer->size - buf_position;

        }
    else if (buf_position + size > a_buffer->size)
        {
            /* we are at the end of the buffer and only want to overwrite bytes */
            memcpy(a_buffer->data + buf_position, data, a_buffer->size - buf_position);
            insert_buffer_in_sequence(a_file, a_buffer);

            /* so overwrite the next buffer ! */
            reste = size - (a_buffer->size - buf_position);
            overwrite_data_at_position(a_file, data + (a_buffer->size - buf_position), 0, &reste);

            size = a_buffer->size - buf_position + reste;
        }

    *size_pointer = size;

    if (a_buffer->in_seq == FALSE)
        {
            destroy_fcl_buf_t((gpointer) a_buffer);
        }
}


/**
 * Inserts data into a buffer in place
 * @warning this function is recursive
 */
static void inserts_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize size)
{
    fcl_buf_t *a_buffer = NULL;  /** Buffer to be overwritten                 */
    goffset buf_position = 0;    /** Position in the buffer                   */
    guchar *new_data = NULL;         /** new buffer that will replace the old one */
    gsize new_size = 0;          /** new size for the buffer                  */

    a_buffer = read_buffer_at_position(a_file, position);

    buf_position = (position - a_buffer->real_offset);

    if (buf_position >= 0 && buf_position <= a_buffer->size)
        {
            new_size = size + a_buffer->size;
            new_data = (guchar *) g_malloc0(new_size * sizeof(guchar));

            memcpy(new_data, a_buffer->data, buf_position);
            memcpy(new_data + buf_position, data, size);
            memcpy(new_data + buf_position + size, a_buffer->data + buf_position, a_buffer->size - buf_position);

            fcl_print_data(new_data, new_size, TRUE);

            g_free(a_buffer->data);
            a_buffer->data = new_data;
            a_buffer->size = new_size;

            if (a_buffer->in_seq == FALSE)
                {
                    insert_buffer_in_sequence(a_file, a_buffer);
                }
        }
}


/**
 * Destroys a buffer (and the data in it !)
 */
static void destroy_fcl_buf_t(gpointer data)
{
    fcl_buf_t *buffer = (fcl_buf_t *) data ;

    if (buffer != NULL)
        {
            print_message("Destroyed buffer : %p\n", buffer);
            if (buffer->data != NULL)
                {
                    g_free(buffer->data);
                }

            g_free(buffer);
        }
}


/****************************** File management *******************************/

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
    a_file->sequence = NULL;

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
            if (file_info != NULL)
                {
                    size = g_file_info_get_size(file_info);
                    g_object_unref(file_info);
                    return size;
                }
            else
                {
                    return -1;
                }
        }
    else
        {
            return -1;
        }
}



/****************************** Comparison functions **************************/

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
            if (buf1->offset == buf2->offset)
                {
                    return 0;
                }
            else if (buf1->offset < buf2->offset)
                {
                    return -1;
                }
            else
                {
                    return +1;
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
    else
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
            if ((buffer1->offset + buffer1->size) > buffer2->offset && (buffer1->offset + buffer1->size) < (buffer2->offset + buffer2->size))
                {
                    return 1;
                }
            else if ((buffer1->offset + buffer1->size) > buffer2->offset && (buffer1->offset + buffer1->size) > (buffer2->offset + buffer2->size))
                {
                    return 2;
                }
            else if ((buffer1->offset + buffer1->size) == buffer2->offset)
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
