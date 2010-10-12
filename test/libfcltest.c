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

#include <stdio.h>
#include <fcl.h>

#include "libfcltest.h"

static void test_openning_and_closing_files(void);
static void test_openning_and_reading_files(void);
static void print_message(gboolean success, const char *format, ...);

/**
 * Prints a message from a test
 * @param success indicates whether the test was sucessfull or not
 * @param message the message to print
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
            fprintf(stdout, "[ OK ] ");
            fprintf(stdout, "%s\n", str);
        }
    else
        {
            fprintf(stdout, "[FAIL] ");
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
            print_message(my_test_file != NULL, "Openning a file in read mode (%Ld)", my_test_file->real_size);
        }
    else
        {
            print_message(my_test_file != NULL, "Openning a file in read mode.");
        }
    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_WRITE);
    print_message(my_test_file != NULL, "Openning a file in write mode.");
    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_CREATE);
    print_message(my_test_file != NULL, "Openning a file in create mode.");

    fcl_close_file(my_test_file);
}


/**
 * This function test openning, reading in the file and closing them
 */
static void test_openning_and_reading_files(void)
{
    fcl_file_t *my_test_file = NULL;
    fcl_buf_t *buffer = NULL;

    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    buffer = fcl_read_bytes(my_test_file, 1, 3);

    if (buffer != NULL && buffer->buffer != NULL)
        {
            print_message(buffer != NULL, "Read : %s", buffer->buffer);
        }
    else
        {
            print_message(buffer != NULL, "Reading 3 bytes in /bin/bash !");
        }
}


int main(int argc, char **argv)
{

    fprintf(stdout, "Testing libfcl ...\n");
    libfcl_initialize();

    test_openning_and_closing_files();

    test_openning_and_reading_files();

    return 0;
}

