/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  fcl.c
 *  File Cache Library
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
/** @file fcl.c
 * Main library file
 */
/**
 * @author Olivier DELHOMME,
 * @version 0.0.1
 * @date 2010
 */
#include "fcl.h"

static fcl_file_t *new_fcl_file_t(void);
static goffset get_gfile_file_size(GFile *the_file);

/**
 * @return a newly initialiazed empty fcl_file_t structure
 */
static fcl_file_t *new_fcl_file_t(void)
{

    fcl_file_t *a_file = NULL;

    a_file = (fcl_file_t *) g_malloc0 (sizeof(fcl_file_t));

    a_file->name = NULL;
    a_file->mode = -1;
    a_file->real_size = 0;
    a_file->the_file = NULL;
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
                a_file = new_fcl_file_t();
                a_file->the_file = g_file_new_for_path(path);
                a_file->mode = mode;
                a_file->name = g_strdup(path);
                a_file->out_stream = NULL;
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                a_file->real_size = get_gfile_file_size(a_file->the_file);
                return a_file;
            break;
            case LIBFCL_MODE_WRITE:
                a_file = new_fcl_file_t();
                a_file->the_file = g_file_new_for_path(path);
                a_file->mode = mode;
                a_file->name = g_strdup(path);
                a_file->out_stream = g_file_append_to(a_file->the_file, G_FILE_CREATE_NONE, NULL, NULL);
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                a_file->real_size = get_gfile_file_size(a_file->the_file);
                return a_file;
            break;
            case LIBFCL_MODE_CREATE:
                a_file = new_fcl_file_t();
                a_file->the_file = g_file_new_for_path(path);
                a_file->mode = mode;
                a_file->name = g_strdup(path);
                a_file->out_stream = g_file_replace(a_file->the_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
                a_file->in_stream = g_file_read(a_file->the_file, NULL, NULL);
                a_file->real_size = get_gfile_file_size(a_file->the_file);
            break;
            default:
                return NULL;
            break;
        }
}



