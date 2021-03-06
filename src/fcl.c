/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  fcl.c
 *  File Cache Library
 *
 *  (C) Copyright 2010 - 2012 Olivier Delhomme
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
/** @file fcl.c
 * Main library file
 * The idea is to virtually split a file into buffers that does exactly
 * LIBFCL_BUF_SIZE bytes (at first).
 *
 * Then it is "simply" a buffer management matter. When bytes are deleted in the
 * file, bytes are deleted in buffers which are memorized in the sequence thus
 * the buffer size might be lower than LIBFCL_BUF_SIZE bytes.
 * When inserting bytes into the file, the bytes are inserted within one single
 * buffer thus, the buffer size might be greater than LIBFCL_BUF_SIZE bytes.
 * Each time a buffer is modified it is memorized in the sequence associated
 * with the file.
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
static gboolean fcl_buffer_exists(fcl_buf_t *a_buffer);
static void print_buffer(gpointer data, gpointer user_data);
static void print_buffers_situation_in_sequence(GSequence *sequence);

static fcl_buf_t *read_buffer_at_position(fcl_file_t *a_file, goffset position, gsize *gap);
static guchar *read_bytes_at_position(fcl_file_t *a_file, goffset position, gsize *size_pointer, gsize *in_data);
static void overwrite_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer);
static void inserts_data_at_position(fcl_file_t *a_file, guchar *data, goffset position, gsize size);
static gboolean delete_bytes_at_position(fcl_file_t *a_file, goffset position, gsize *size_pointer);

static gboolean save_the_file(fcl_file_t *a_file);

static void insert_buffer_in_sequence(fcl_file_t *a_file, fcl_buf_t *a_buffer);

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
void fcl_close_file(fcl_file_t *a_file, gboolean save)
{

    /* printing statistics on the file and its sequence */
    print_buffers_situation_in_sequence(a_file->sequence);
    fcl_print_buffer_stats(a_file);

    g_free(a_file->name);

    if (save == TRUE)
        {
           /* save_the_file(a_file); */
        }

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
            g_sequence_free(a_file->sequence);   /* Here the buffers in the sequence are freed with destroy_fcl_buf_t */
        }

    g_free(a_file);

    print_message("The file is closed.\n");
}


/**
 * This function reads an fcl_buf_t buffer from an fcl_file_t
 * @param a_file : the fcl_file_t file from which we want to read size bytes
 * @param position : the position where we want to read bytes
 * @param[in,out] size_pointer : the number of bytes we want to read. The value
 *                               indicates thereal size of the data read hence,
 *                               the real size of the returned gchar * buffer
 * @return If everything is ok a filled guchar *buffer (may be less than
 *         requested)
 */
guchar *fcl_read_bytes(fcl_file_t *a_file, goffset position, gsize *size_pointer)
{
    gsize in_data = 0;
    guchar *data = NULL;

    if (a_file != NULL && position >= 0 && *size_pointer > 0)
        {
            data = read_bytes_at_position(a_file, position, size_pointer, &in_data);
            *size_pointer = in_data;
        }

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
 * @return True If everything is ok, false otherwise
 */
extern gboolean fcl_overwrite_bytes(fcl_file_t *a_file, guchar *data, goffset position, gsize *size_pointer)
{
    gsize size = 0;  /** Because I do not like *size_pointer everywhere !  */

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
 * @param size : size of the data to overwrite. As the insertion should never
 *               be less than size (or 0) one may consider that if the function
 *               retuned TRUE, size bytes were effectively inserted.
 * @return True If everything is ok, false otherwise
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



/**
 * Deletes bytes in the buffers.
 * @param a_file : the fcl_file_t file to which we want to deleted size bytes
 * @param position : position where to delete size_pointer bytes in the file
 * @param[in,out] size_pointer : number of bytes to be deleted. Returns the
 * number of bytes effectively deleted (may be less than requested)
 * @return True if everything is ok, false otherwise.
 */
extern gboolean fcl_delete_bytes(fcl_file_t *a_file, goffset position, gsize *size_pointer)
{
    gsize size = 0;  /** Because I do not like *size_pointer everywhere !  */

    /* we can not delete bytes in a read-only file ! */
    if (a_file->mode != LIBFCL_MODE_READ)
        {
            size = *size_pointer;

            return delete_bytes_at_position(a_file, position, &size);

            *size_pointer = size;
        }
    else
        {
            fprintf(stderr, Q_("File is read-only, deleting is prohibited\n"));
            return FALSE;
        }
}




/******************************************************************************/
/*********************************** Buffers **********************************/


/**
 * Prints a buffer data (exactly 'size' bytes)
 * @todo : print UTF8 encoded values ?
 * @param data : buffer data to be printed
 * @param size : number of bytes to prints (from data)
 * @param EOL : prints an End Of Line if TRUE, nothing if FALSE
 */
extern void fcl_print_data(guchar *data, gsize size, gboolean EOL)
{
    gsize i = 0;
    gunichar c;   /** a character to test */

    if (data != NULL)
        {
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


/**
 * Prints the buffer state (if debug is enabled)
 * @param data a fcl_buf_t buffer is expected
 * @param user_data : not used so may be NULL
 */
static void print_buffer(gpointer data, gpointer user_data)
{
    fcl_buf_t *a_buffer = (fcl_buf_t *) data;

    if (ENABLE_DEBUG)
        {
            fprintf(stdout, "Offset      : %Ld\n", a_buffer->offset);
            fprintf(stdout, "Real offset : %Ld\n", a_buffer->real_offset);
            fprintf(stdout, "Size        : %d\n", a_buffer->size);

            if (a_buffer->in_seq == TRUE)
                {
                    fprintf(stdout, "In sequence : TRUE\n\n");
                }
            else
                {
                     fprintf(stdout, "In sequence : FALSE\n\n");
                }
        }
}


static void print_buffers_situation_in_sequence(GSequence *sequence)
{

    if (ENABLE_DEBUG && sequence != NULL)
        {
            fprintf(stdout, "\nBuffers in the sequence :\n");
            g_sequence_foreach(sequence, print_buffer, NULL);

        }
}



/****************************** Buffers management ****************************/

/**
 * Says wether the buffer structure exists and that the data buffer exists also
 * @param a_buffer the buffer to check
 */
static gboolean fcl_buffer_exists(fcl_buf_t *a_buffer)
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
static fcl_buf_t *read_buffer_at_position(fcl_file_t *a_file, goffset position, gsize *the_gap)
{
    fcl_buf_t *a_buffer = NULL;  /** Buffer to be read                                   */
    gssize read  = 0;            /** Number of bytes effectively read                    */
    goffset real_position = 0;   /** Position in the file (in number of LIBFCL_BUF_SIZE) */
    gsize gap = 0;               /** gap between the edited buffers and the file         */
    GSequenceIter *begin = NULL; /** to iterate over the sequence, from the begining     */
    fcl_buf_t * seq_buf = NULL;
    gboolean ok = TRUE;

    print_message("read_buffer_at_position(%p, %ld, %ld) : ", a_file, position, *the_gap);

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

            print_message("%ld\n", read);
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

            seq_buf = g_sequence_get(begin);

            print_message("\nreal_position + seq_buf->size < position ? %ld + %ld < %ld\n", real_position,  seq_buf->size, position);

            while ((real_position + seq_buf->size) <= position && ok == TRUE)
                {
                    real_position = real_position + seq_buf->size;
                    gap = gap + (seq_buf->size - LIBFCL_BUF_SIZE);

                    print_message("real_position : %ld ; gap : %ld\n", real_position, gap);

                    if (g_sequence_iter_is_end(begin) == TRUE)
                        {
                            ok = FALSE;
                        }
                    else
                        {
                            begin = g_sequence_iter_next(begin);

                            if (g_sequence_iter_is_end(begin) != TRUE)
                                {
                                    seq_buf = g_sequence_get(begin);
                                }
                            else
                                {
                                    ok = FALSE;
                                }
                        }
                }

            print_message("real_position : %ld; seq_buf->size : %ld; position : %ld; gap : %ld\n", real_position,  seq_buf->size, position, gap);

            *the_gap = gap;
            if ((ok == TRUE) && ((real_position + seq_buf->size) > position))
                {
                    /* buffer exists */
                    print_message("%ld == %ld ?\n", seq_buf->real_offset, real_position);
                    seq_buf->real_offset = real_position;
                    a_buffer = seq_buf;
                }
            else
                {
                    /* buffer does not exists or is not found in the sequence */
                    a_buffer = new_fcl_buf_t();

                    a_buffer->offset = (position - gap) / LIBFCL_BUF_SIZE;
                    a_buffer->real_offset = (position - gap);

                    g_seekable_seek(G_SEEKABLE(a_file->in_stream), a_buffer->real_offset, G_SEEK_SET, NULL, NULL);
                    read = g_input_stream_read(G_INPUT_STREAM(a_file->in_stream), a_buffer->data, LIBFCL_BUF_SIZE, NULL, NULL);

                    a_buffer->size = read;
                }
        }

    print_buffer(a_buffer, NULL);

    return a_buffer;
}


/**
 * This function reads an fcl_buf_t buffer from an fcl_file_t
 * @warning this function is recursive.
 * @param a_file : the fcl_file_t file from which we want to read size bytes
 * @param position : the position where we want to read bytes
 * @param[in,out] size_pointer : the number of bytes we want to read. The value
 *                               indicates thereal size of the data read hence,
 *                               the real size of the returned gchar * buffer
 * @param[in,out] in_data : number of bytes in the buffer to be returned. It
 *                          also indicates the position in the buffer.
 * @return If everything is ok a filled guchar *buffer (may be less than
 *         requested)
 */
static guchar *read_bytes_at_position(fcl_file_t *a_file, goffset position, gsize *size_pointer, gsize *in_data)
{
    fcl_buf_t *a_buffer = NULL;  /** the fcl_buf_t structure that will be returned     */
    guchar *data = NULL;         /** The data that is claimed (size bytes at position) */
    guchar *next_data = NULL;
    guchar *new_data = NULL;
    goffset offset = 0;          /** The offset in the data buffer                     */
    gsize real_size = 0;         /** Real size returned by the recursive call          */
    gsize size = 0;              /** Because I do not like *size_pointer everywhere !  */
    gsize gap = 0;

    size = *size_pointer;

    print_message("read_bytes_at_position(%p, %ld, %ld, %ld)\n", a_file, position, size, *in_data);

    data = (guchar *) g_malloc0 (size * sizeof(guchar));

    a_buffer = read_buffer_at_position(a_file, position, &gap);

    /* offset is viewed as the offset in the buffer a_buffer just read above */
    offset = position - a_buffer->real_offset;

    print_message("offset : %ld; size : %ld\n", offset, size);

    /* If the offset is below 0 or upper than a_buffer->size we need to correct it with gap */
    if (offset < 0 || offset >= a_buffer->size)
        {
            offset = offset - gap;
        }

    print_message("offset : %ld; size : %ld\n", offset, size);

    if (offset >= 0 && offset <= a_buffer->size) /* The offset is within the buffer data */
        {
            if (a_buffer->size >= offset + size) /* The claimed data is all in the buffer */
                {
                    print_message("1. g_mem_dup(%p, %ld)\n", a_buffer->data + offset, size);
                    data = (guchar *) g_memdup(a_buffer->data + offset, size);
                    print_message("size : %ld\n", size);
                    *in_data = *in_data + size;
                }
            else if (a_buffer->size < LIBFCL_BUF_SIZE && a_buffer->in_seq == FALSE)
                { /* Not all the buffer was filled but the buffer is not in the sequence. The buffer was read in the file hence this is the end of the file */

                    size = a_buffer->size - offset;
                    if (size  > 0)
                        {
                            print_message("2. g_mem_dup(%p, %ld)\n", a_buffer->data + offset, size);
                            data = (guchar *) g_memdup(a_buffer->data + offset, size);
                            *in_data = *in_data + size;
                        }
                }
            else
                {
                    /* claimed data is located in two different buffers at least */
                    /** @todo may be a bug here in memory allocations ?? */
                    print_message("3. g_mem_dup(%p, %ld)\n", a_buffer->data + offset, a_buffer->size - offset);

                    new_data = (guchar *) g_memdup(a_buffer->data + offset, a_buffer->size - offset);

                    real_size = size - (a_buffer->size - offset);
                    *in_data = *in_data + (a_buffer->size - offset);

                    next_data = read_bytes_at_position(a_file, position + (a_buffer->size - offset), &real_size, in_data);

                    if (next_data != NULL)
                        {
                            size = real_size + (a_buffer->size - offset);

                            print_message("size : %ld; real_size : %ld; in_data : %ld\n", size, real_size, *in_data);

                            data = (guchar *) g_malloc0(size * sizeof(guchar));
                            memcpy(data, new_data, a_buffer->size - offset);
                            memcpy(data + (a_buffer->size - offset), next_data, real_size);

                            g_free(new_data);
                            g_free(next_data);
                        }
                    else
                        {
                            data = (guchar *) g_malloc0(size * sizeof(guchar));
                            memcpy(data, new_data, a_buffer->size - offset);
                            g_free(new_data);
                        }
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
 * Inserts a buffer in the sequence (only if it is not already in it !)
 */
static void insert_buffer_in_sequence(fcl_file_t *a_file, fcl_buf_t *a_buffer)
{

    if (a_file != NULL && a_buffer != NULL)
        {
            if (a_file->sequence == NULL)
                {
                    a_buffer->in_seq = TRUE;
                    a_file->sequence = g_sequence_new(destroy_fcl_buf_t);
                    g_sequence_append(a_file->sequence, a_buffer);
                    print_message("Inserted buffer : %p (%ld, %ld, %ld)\n", a_buffer, a_buffer->offset, a_buffer->real_offset, a_buffer->size);
                }
            else
                {
                    if (a_buffer->in_seq == FALSE)
                        {
                            a_buffer->in_seq = TRUE;
                            g_sequence_insert_sorted(a_file->sequence, a_buffer, cmp_offset_value, NULL);
                            print_message("Inserted buffer : %p (%ld, %ld, %ld)\n", a_buffer, a_buffer->offset, a_buffer->real_offset, a_buffer->size);
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
    gsize gap = 0;

    size = *size_pointer;

    print_message("overwrite_data_at_position(%p, %p, %ld, %ld)\n", a_file, data, position, size);

    a_buffer = read_buffer_at_position(a_file, position, &gap);

    buf_position = (position - a_buffer->real_offset);
    print_message("buf_position : %ld (position : %ld, real_offset : %ld)\n", buf_position, position, a_buffer->real_offset);

    if (buf_position >= 0 && (buf_position + size) <= a_buffer->size)
        {
            memcpy(a_buffer->data + buf_position, data, size);
            insert_buffer_in_sequence(a_file, a_buffer);
        }
    else if (buf_position + size > a_buffer->size && a_buffer->size < LIBFCL_BUF_SIZE) /* This last test is here to detect the end of the file */
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
    fcl_buf_t *a_buffer = NULL;  /** Buffer where to insert datas             */
    goffset buf_position = 0;    /** Position in the buffer                   */
    guchar *new_data = NULL;     /** new buffer that will replace the old one */
    gsize new_size = 0;          /** new size for the buffer                  */
    gsize gap = 0;

    a_buffer = read_buffer_at_position(a_file, position, &gap);

    buf_position = (position - a_buffer->real_offset);

    if (buf_position >= 0 && buf_position <= a_buffer->size)
        {
            new_size = size + a_buffer->size;
            new_data = (guchar *) g_malloc0(new_size * sizeof(guchar));

            memcpy(new_data, a_buffer->data, buf_position);
            memcpy(new_data + buf_position, data, size);
            memcpy(new_data + buf_position + size, a_buffer->data + buf_position, a_buffer->size - buf_position);

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
 * Deletes bytes at position in the file
 * @warning this function is recursive
 */
static gboolean delete_bytes_at_position(fcl_file_t *a_file, goffset position, gsize *size_pointer)
{

    fcl_buf_t *a_buffer = NULL;  /** Buffer where to insert datas             */
    goffset buf_position = 0;    /** Position in the buffer                   */
    guchar *new_data = NULL;
    gsize size = 0;
    gsize old_buffer_size = 0;
    gsize to_delete_size = 0;
    gboolean result = TRUE;
    gsize gap = 0;

    size = *size_pointer;

    print_message("delete_bytes_at_position(%p, %ld, %ld)\n", a_file, position, size);

    a_buffer = read_buffer_at_position(a_file, position, &gap);

    buf_position = (position - a_buffer->real_offset);

    print_message("buf_position : %ld <? %ld : a_buffer->size\n", buf_position, a_buffer->size);

    /* Adding this because there is no reason that the behavior will be different
     * than read_bytes_at_position. If the offset is below 0 or upper than
     * a_buffer->size we need to correct it with gap
     */
    if (buf_position < 0 || buf_position >= a_buffer->size)
        {
            buf_position = buf_position - gap;
        }

    print_message("buf_position : %ld <? %ld : a_buffer->size\n", buf_position, a_buffer->size);

    if (buf_position >= 0 && buf_position <= a_buffer->size)
        {
            /* Is this always the case ? ie may we fall in the case that the
             * buffer size is so small that the position is not in the
             * retrieved buffer but in the next one ?
             */

            if (buf_position + size <= a_buffer->size)
                {
                    /* All bytes to be deleted are in the same buffer */
                    new_data = (guchar *) g_malloc0((a_buffer->size - size) * sizeof(guchar));

                    memcpy(new_data, a_buffer->data, buf_position);
                    memcpy(new_data + buf_position, a_buffer->data + (buf_position + size), a_buffer->size - (buf_position + size) );

                    g_free(a_buffer->data);
                    a_buffer->data = new_data;
                    a_buffer->size = a_buffer->size - size;
                }
            else if (buf_position + size > a_buffer->size && a_buffer->size < LIBFCL_BUF_SIZE) /* This last test is here to detect the end of the file */
                {
                    fprintf(stderr, Q_("Deleting bytes outside of the file is not possible !\n"));
                    new_data = (guchar *) g_malloc0((buf_position) * sizeof(guchar));
                    memcpy(new_data, a_buffer->data, (buf_position));
                    g_free(a_buffer->data);
                    a_buffer->data = new_data;
                    a_buffer->size = buf_position;

                    /* Here the _real_ size is different ! */
                    size = a_buffer->size - buf_position;
                }
            else
                {
                    /* saving read buffer size */
                    old_buffer_size = a_buffer->size;

                    /* Bytes to be deleted are in at least two buffers */
                    new_data = (guchar *) g_malloc0((buf_position) * sizeof(guchar));
                    memcpy(new_data, a_buffer->data, (buf_position));
                    g_free(a_buffer->data);
                    a_buffer->data = new_data;
                    a_buffer->size = buf_position;

                    /* new size to be deleted in the next buffer */
                    to_delete_size = size - (LIBFCL_BUF_SIZE - buf_position);

                    /* deletes bytes in the other buffers */
                    result = delete_bytes_at_position(a_file, position + (old_buffer_size - buf_position), &to_delete_size);

                    /* to reflect what was effectively deleted */
                    size = to_delete_size + (LIBFCL_BUF_SIZE - buf_position);

                }

            /* The buffer has been modified we must put it in the sequence (if it is not allready in it) */

             if (a_buffer->in_seq == FALSE)
                {
                    insert_buffer_in_sequence(a_file, a_buffer);
                }

            *size_pointer = size;

            return result;
        }
    else
        {
            *size_pointer = size;
            return FALSE;
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

/**
 * Sums the stats within a foreach function
 * @param data must be a buffer as found in a sequence
 * @param user_data must be the fcl_stat_buf_t* statistics structure
 */
static void sum_stats(gpointer data, gpointer user_data)
{
    fcl_buf_t *seq_buf = (fcl_buf_t *) data;
    fcl_stat_buf_t *stats = (fcl_stat_buf_t *) user_data;
    gssize gap = 0;

    if (seq_buf != NULL && stats != NULL)
        {

            /* Number of buffers in the sequence */
            stats->n_bufs = stats->n_bufs + 1;

            /* Maximum size of one buffer */
            if (seq_buf->size > stats->max_buf_size)
                {
                    stats->max_buf_size = seq_buf->size;
                }

            /* Minimum size of one buffer */
            if (seq_buf->size < stats->min_buf_size)
                {
                    stats->min_buf_size = seq_buf->size;
                }

            /* gap represents additions or deletions in the buffers */
            gap = seq_buf->size - LIBFCL_BUF_SIZE;

            /* Total edition size */
            stats->real_edit_size = stats->real_edit_size + gap;

            /* Here we do track additions only */
            if (gap > 0)
                {
                    stats->add_size = stats->add_size + gap;
                }
        }
}


/**
 * Inits the statistics structure to default values. The structure must be freed
 * when no longer needed
 * @return an newly allocated and default values populated fcl_stat_buf_t*
 *         structure
 */
fcl_stat_buf_t *fcl_init_buffer_stats()
{
    fcl_stat_buf_t *stats = NULL;

    stats = (fcl_stat_buf_t *) g_malloc0 (sizeof(fcl_file_t));

    stats->min_buf_size = G_MAXSSIZE;
    stats->max_buf_size = 0;
    stats->add_size = 0;
    stats->real_edit_size = 0;
    stats->n_bufs = 0;

    return stats;
}


/**
 * Gets the statistics of the buffers of a fcl_file_t file.
 * @param a_file : an openned fcl_file_t file.
 * @return A newly allocated fcl_stat_buf_t filled with the statistics about
 *         the sequence structure of the fcl_file_t structure. Returns NULL if
 *         the structure does not exists or does not have any buffers.
 */
fcl_stat_buf_t *fcl_get_buffer_stats(fcl_file_t *a_file)
{
    fcl_stat_buf_t *stats = NULL;

    if (a_file != NULL && a_file->sequence != NULL)
        {
            stats = fcl_init_buffer_stats();

            g_sequence_foreach(a_file->sequence, sum_stats, stats);
        }

    return stats;
}


/**
 * Prints stats of the buffers (if any) on an fcl_file_t file
 * @param an openned fcl_file_t*
 */
void fcl_print_buffer_stats(fcl_file_t *a_file)
{
    fcl_stat_buf_t *stats = NULL;

    if (a_file != NULL)
        {
            stats = fcl_get_buffer_stats(a_file);

            if (stats != NULL)
                {
                    fprintf(stdout, "\n");
                    fprintf(stdout, "Buffer statistics on %s :\n", a_file->name);
                    fprintf(stdout, " Number of buffers : %Ld\n", stats->n_bufs);
                    fprintf(stdout, " Min buffer size   : %d\n", stats->min_buf_size);
                    fprintf(stdout, " Max buffer size   : %d\n", stats->max_buf_size);
                    fprintf(stdout, " Additions size    : %d\n", stats->add_size);
                    fprintf(stdout, " Deletion size     : %d\n", stats->add_size - stats->real_edit_size);
                    fprintf(stdout, " Real buffer edition sizes : %d\n", stats->real_edit_size);
                    fprintf(stdout, "\n");

                    g_free(stats);
                }
        }
}



/**
 * @warning Still to be built !
 */
static gboolean save_the_file(fcl_file_t *a_file)
{
    fcl_buf_t *a_buffer = NULL;  /** Buffer to be read                                   */
    gssize read = 0;             /** Number of bytes effectively read                    */
    goffset real_position = 0;   /** Position in the file (in number of LIBFCL_BUF_SIZE) */
    goffset gap = 0;             /** gap between the edited buffers and the file         */
    GSequenceIter *iter = NULL;
    GSequenceIter *end = NULL;
    fcl_buf_t * seq_buf = NULL;
    gboolean ok = TRUE;          /** TRUE until we reach the end */
    guchar *in_gap = NULL;
    guchar *to_write = NULL;



    if (a_file->mode != LIBFCL_MODE_READ)
        {
            if (a_file->sequence != NULL)
                {
                    /* Saving the file in place */
                    gap = 0;
                    real_position = 0;
                    ok = TRUE;
                    iter = g_sequence_get_begin_iter(a_file->sequence); /* Begin of the first modification of the file */
                    end = g_sequence_get_end_iter(a_file->sequence);     /* end of the modification */

                    seq_buf = g_sequence_get(iter);

                    while (ok == TRUE)
                        {
                            gap = gap + (seq_buf->size - LIBFCL_BUF_SIZE);

                            if (gap > 0)
                                {
                                    /* reading the gap */
                                    in_gap = (guchar *) g_malloc0(gap * sizeof(guchar));

                                    g_seekable_seek(G_SEEKABLE(a_file->in_stream), seq_buf->real_offset + LIBFCL_BUF_SIZE, G_SEEK_SET, NULL, NULL);
                                    read = g_input_stream_read(G_INPUT_STREAM(a_file->in_stream), in_gap, gap, NULL, NULL);
                                }

                            /* Writing the buffer */
                            g_seekable_seek(G_SEEKABLE(a_file->in_stream), seq_buf->real_offset, G_SEEK_SET, NULL, NULL);
                            g_output_stream_write(G_OUTPUT_STREAM(a_file->out_stream), seq_buf->data, seq_buf->size, NULL, NULL);

                            if (gap > 0)
                                {
                                    to_write = g_memdup(in_gap, gap);
                                }

                            if (iter == end)
                                {
                                    ok = FALSE;
                                }
                            else
                                {
                                    iter = g_sequence_iter_next(iter);
                                    seq_buf = g_sequence_get(iter);
                                    /* if the offset of this buffer is not the
                                     * following of the previous buffer, we have
                                     * to "iterate" into the file to "report" the
                                     * gap
                                     */


                                }
                        }

                    /* now we should go to the end of the file  ... */



                }

            return TRUE;
        }
    else
        {
            fprintf(stderr, Q_("File is read-only, saving it prohibited\n"));
            return FALSE;
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
