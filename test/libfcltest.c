/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  libfcltest.c
 *  File Cache Library Test
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
/** @file libfcltest.c
 */

#include "config.h"

#include <stdio.h>
#include <glib/gi18n-lib.h>

#include <fcl.h>

#include "libfcltest.h"

static void init_international_languages(void);

static void print_message(gboolean success, const char *format, ...);
static gchar *get_home_dir(void);

static guchar *fill_data_with_char(gint size, guchar car);

static void test_openning_and_closing_files(void);
static void test_openning_and_reading_files(void);
static void test_openning_and_overwriting_files(void);
static void test_openning_and_inserting_in_files(void);
static void test_openning_and_deleting_in_files(void);

/**
 *  Inits internationalisation
 */
static void init_international_languages(void)
{
    gchar *result = NULL;

    setlocale(LC_ALL, "");
    result = bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);

    fprintf(stdout, Q_("Gettext package : %s\n"), GETTEXT_PACKAGE);
    fprintf(stdout, Q_("Locale dir : %s\n"), LOCALEDIR);
    fprintf(stdout, Q_("Bindtextdomain : %s\n"), result);

    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
}


/**
 * Prints a message from a test
 * @param success indicates whether the test was sucessfull or not
 * @param format the message to print in a vprintf format
 * @param ... the va_list : list of arguments to fill in the format
 */
static void print_message(gboolean success, const char *format, ...)
{
    va_list args;
    gchar *str = NULL;

    va_start(args, format);
    str = g_strdup_vprintf(format, args);
    va_end(args);

    if (success)
        {
            fprintf(stdout, Q_("[ OK ] "));
            fprintf(stdout, "%s\n", str);
        }
    else
        {
            fprintf(stdout, Q_("[FAIL] "));
            fprintf(stdout, "%s\n", str);
        }
}


/**
 * Retrieves the home dir
 * @return the home dir in a gchar * that can be freed when no longer needed
 */
static gchar *get_home_dir(void)
{
    const char *homedir = NULL;

    homedir = g_getenv("HOME");

    if (homedir == NULL)
        {
            homedir = g_get_home_dir();
        }

    return g_strdup(homedir);
}


/**
 * Fills a buffer with a character
 * @param size : the size of the buffer that we want
 * @param car : character used to fill the buffer with
 * @return a guchar * buffer of the desired size filled with the desired
 *         character. It may be freed when no longer needed.
 */
static guchar *fill_data_with_char(gint size, guchar car)
{
    guchar *data = NULL;
    gint i = 0;

    data = (guchar *) g_malloc0(size * sizeof(guchar));

    for (i = 0; i < size; i++)
        {
            data[i] = car;
        }

    return data;
}


/**
 * This function tests openning files and closing them with different modes
 */
static void test_openning_and_closing_files(void)
{
    fcl_file_t *my_test_file = NULL;

    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    if (my_test_file != NULL)
        {
            print_message(my_test_file != NULL, Q_("Opening a file in read mode (%Ld)"), my_test_file->real_size);
        }
    else
        {
            print_message(my_test_file != NULL, Q_("Opening a file in read mode."));
        }
    fcl_close_file(my_test_file, FALSE);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_WRITE);
    print_message(my_test_file != NULL, Q_("Opening a file in write mode."));
    fcl_close_file(my_test_file, FALSE);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_CREATE);
    print_message(my_test_file != NULL, Q_("Opening a file in create mode."));
    fcl_close_file(my_test_file, FALSE);

    my_test_file = fcl_open_file("/tmp/test_doesnotexists", LIBFCL_MODE_READ);
    print_message(my_test_file == NULL, Q_("Opening a file that does not exists in read only mode."));
    fcl_close_file(my_test_file, FALSE);

}


/**
 * This function test openning, reading in the file and closing them
 */
static void test_openning_and_reading_files(void)
{
    fcl_file_t *my_test_file = NULL;
    guchar *buffer = NULL;
    gsize size = 0;


    /* A valid test. Should return ELF as this is the magic number for a compiled /bin/bash */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = 3;
    buffer = fcl_read_bytes(my_test_file, 1, &size);

    if (buffer != NULL)
        {
            print_message(buffer != NULL, Q_("Read (%d bytes at %d) : "), size, 1);
            fcl_print_data(buffer, 3, TRUE);
        }
    else
        {
            print_message(buffer != NULL, Q_("Reading 3 bytes in /bin/bash !"));
        }
    fcl_close_file(my_test_file, FALSE);


    /* This test will not return NULL anymore because I deleted the limit for
     * the global buffer (ie what may be claimed by an application
     */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = LIBFCL_MAX_BUF_SIZE + 1;
    buffer = fcl_read_bytes(my_test_file, 1, &size);
    print_message(buffer != NULL, Q_("Openning a file in create mode and reading more than allowed."));
    fcl_close_file(my_test_file, FALSE);


    /* Reading data at the limits of the buffer */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = 35;
    buffer = fcl_read_bytes(my_test_file, 65525, &size);
    if (buffer != NULL)
        {
            print_message(buffer != NULL, Q_("Read (%d bytes at %d) : "), size, 65525);
            fcl_print_data(buffer, size, TRUE);
        }
    else
        {
            print_message(buffer != NULL, Q_("Reading 35 bytes in /bin/bash !"));
        }
    fcl_close_file(my_test_file, FALSE);

    /* Reading data at the limits of the file */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = 336;
    buffer = fcl_read_bytes(my_test_file, my_test_file->real_size - 336, &size);
    if (buffer != NULL)
        {
            print_message(buffer != NULL, Q_("Read (%d bytes at %d) : "), size, my_test_file->real_size - 336);
            fcl_print_data(buffer, size, TRUE);
        }
    else
        {
            print_message(buffer != NULL, Q_("Reading 336 bytes in /bin/bash !"));
        }
    fcl_close_file(my_test_file, FALSE);


    /* Reading data beyond the limits of the file */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = 16384;
    buffer = fcl_read_bytes(my_test_file, my_test_file->real_size + 336, &size);
    if (buffer != NULL)
        {
            print_message(buffer == NULL, Q_("Read (%d bytes at %d) : "), size, my_test_file->real_size + 336);
            fcl_print_data(buffer, size, TRUE);
        }
    else
        {
            print_message(buffer == NULL, Q_("Reading %ld bytes in /bin/bash ! at %ld"), size, my_test_file->real_size + 336 );
        }
    fcl_close_file(my_test_file, FALSE);

}


/**
 * This function test openning, overwriting in files and closing them
 */
static void test_openning_and_overwriting_files(void)
{
    fcl_file_t *my_test_file = NULL;
    guchar *buffer = NULL;
    guchar *data = NULL;
    gboolean result = TRUE;
    gsize size = 0;
    gchar *filename = NULL;

    buffer = (guchar *) g_strdup_printf("ABC");

    /* This test tries to overwrite a readonly file */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    size = 3;
    result = fcl_overwrite_bytes(my_test_file, buffer, 1, &size);

    print_message(result == FALSE, Q_("Trying to overwrite in a READ ONLY opened file"));

    fcl_close_file(my_test_file, FALSE);

    /* Testing to overwrite into a file (without saving) */
    filename = g_build_path(G_DIR_SEPARATOR_S, get_home_dir(), ".bashrc", NULL);
    my_test_file = fcl_open_file(filename, LIBFCL_MODE_WRITE);
    size = 3;
    result = fcl_overwrite_bytes(my_test_file, buffer, 2, &size);

    print_message(result == TRUE, Q_("Trying to overwrite in an opened file (%ld bytes)"), size);

    /* Verifiying that it worked out */
    size = 10;
    data = fcl_read_bytes(my_test_file, 0, &size);
    fcl_print_data(data, size, TRUE);

    fcl_close_file(my_test_file, FALSE);

    g_free(buffer);

}


/**
 * This function test openning, inserting bytes in files and closing them
 */
static void test_openning_and_inserting_in_files(void)
{
    fcl_file_t *my_test_file = NULL;
    guchar *buffer = NULL;
    guchar *data = NULL;
    gsize size = 0;

    buffer = (guchar *) g_strdup_printf("Is this inserted in the file ??");

    my_test_file = fcl_open_file("/tmp/createme", LIBFCL_MODE_CREATE);
    print_message(my_test_file != NULL, Q_("Opening a file in create mode."));

    /* Inserting bytes */
    fcl_insert_bytes(my_test_file, buffer, 0, 30);

    fprintf(stdout, "\nVerifying if everything is there !\n");
    /* Verifying this (double check here) */
    size = 200;
    data = fcl_read_bytes(my_test_file, 0, &size);
    fprintf(stdout, Q_("Size read : %d\n"), size);
    fcl_print_data(data, size, TRUE);

    fcl_close_file(my_test_file, FALSE);
}


/**
 * This function test openning, deleting bytes in files and closing them
 */
static void test_openning_and_deleting_in_files(void)
{
    fcl_file_t *my_test_file = NULL;
    guchar *data = NULL;
    gsize size = 0;
    gchar *filename = NULL;

    filename = g_build_path(G_DIR_SEPARATOR_S, get_home_dir(), ".bashrc", NULL);
    my_test_file = fcl_open_file(filename, LIBFCL_MODE_WRITE);
    print_message(my_test_file != NULL, Q_("Opening a file in create mode."));

    /* Deleting bytes */
    size = 30;
    fcl_delete_bytes(my_test_file, 5, &size);

    /* Verifying this */
    size = 200;
    data = fcl_read_bytes(my_test_file, 2, &size);
    fcl_print_data(data, size, TRUE);

    fcl_close_file(my_test_file, FALSE);
}




int main(int argc, char **argv)
{
    /* Initializing the locales */
    init_international_languages();

    /* Initializing the library */
    fprintf(stdout, Q_("\nNow testing libfcl ...\n"));
    libfcl_initialize();

    /**** Tests ****/
    fprintf(stdout, Q_("Testing opening and closing files :\n"));
    test_openning_and_closing_files();
    fprintf(stdout,"\n\n");

    fprintf(stdout, Q_("Testing opening and reading files :\n"));
    test_openning_and_reading_files();
    fprintf(stdout,"\n\n");

    fprintf(stdout, Q_("Testing opening and overwritting files :\n"));
    test_openning_and_overwriting_files();
    fprintf(stdout,"\n\n");

    fprintf(stdout, Q_("Testing opening and inserting in files :\n"));
    test_openning_and_inserting_in_files();
    fprintf(stdout,"\n\n");

    fprintf(stdout, Q_("Testing opening and deleting in files :\n"));
    test_openning_and_deleting_in_files();
    fprintf(stdout,"\n\n");


    return 0;
}

