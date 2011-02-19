/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  libfcltest.c
 *  File Cache Library Test
 *
 *  (C) Copyright 2010 Olivier Delhomme
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
/** @file libfcltest.c
 */

#include "config.h"

#include <stdio.h>
#include <glib/gi18n-lib.h>

#include <fcl.h>

#include "libfcltest.h"

static void print_message(gboolean success, const char *format, ...);
static void init_international_languages(void);

static void test_openning_and_closing_files(void);
static void test_openning_and_reading_files(void);


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
 * This function tests openning files and closing them with different modes
 */
static void test_openning_and_closing_files(void)
{
    fcl_file_t *my_test_file = NULL;

    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    if (my_test_file != NULL)
        {
            print_message(my_test_file != NULL, Q_("Openning a file in read mode (%Ld)"), my_test_file->real_size);
        }
    else
        {
            print_message(my_test_file != NULL, Q_("Openning a file in read mode."));
        }
    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_WRITE);
    print_message(my_test_file != NULL, Q_("Openning a file in write mode."));
    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_CREATE);
    print_message(my_test_file != NULL, Q_("Openning a file in create mode."));

    fcl_close_file(my_test_file);
}


/**
 * This function test openning, reading in the file and closing them
 */
static void test_openning_and_reading_files(void)
{
    fcl_file_t *my_test_file = NULL;
    guchar *buffer = NULL;


    /* A valid test. Should return ELF as this is the magic number for a compiled /bin/bash */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    buffer = fcl_read_bytes(my_test_file, 1, 3);

    if (buffer != NULL)
        {
            print_message(buffer != NULL, Q_("Read (%d bytes at %d) : '%s'"), 3, 1, buffer);
        }
    else
        {
            print_message(buffer != NULL, Q_("Reading 3 bytes in /bin/bash !"));
        }
    fcl_close_file(my_test_file);

    /* An Invalid test. Sould return NULL */
    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    buffer = fcl_read_bytes(my_test_file, 1, LIBFCL_MAX_BUF_SIZE + 1);
    print_message(buffer == NULL, Q_("Openning a file in create mode and reading more than allowed."));
    fcl_close_file(my_test_file);
}


int main(int argc, char **argv)
{
    /* Initializing the locales */
    init_international_languages();

    /* Initializing the library */
    fprintf(stdout, Q_("\nNow testing libfcl ...\n"));
    libfcl_initialize();

    /**** Tests ****/
    test_openning_and_closing_files();

    test_openning_and_reading_files();

    return 0;
}

