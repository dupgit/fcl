/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *  libfcltest.c
 *  File Cache Library Test
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
/** @file libfcltest.c
 */

#include <stdio.h>
#include <fcl.h>

#include "libfcltest.h"

static void test_openning_and_closing_files(void);


static void test_openning_and_closing_files(void)
{
    fcl_file_t *my_test_file = NULL;

    my_test_file = fcl_open_file("/bin/bash", LIBFCL_MODE_READ);
    if (my_test_file != NULL)
        {
           fprintf(stdout, "[ OK ] Openning in read mode seems ok : %Ld\n", my_test_file->real_size);
        }
    else
        {
           fprintf(stdout, "[FAIL] Openning in read mode seems wrong.\n");
        }

    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_WRITE);
    if (my_test_file != NULL)
        {
           fprintf(stdout, "[ OK ] Openning in write mode seems ok.\n");
        }
    else
        {
           fprintf(stdout, "[FAIL] Openning in write mode seems wrong.\n");
        }

    fcl_close_file(my_test_file);

    my_test_file = fcl_open_file("/tmp/test.libfcl", LIBFCL_MODE_WRITE);
    if (my_test_file != NULL)
        {
           fprintf(stdout, "[ OK ] Openning in create mode seems ok.\n");
        }
    else
        {
           fprintf(stdout, "[FAIL] Openning in create mode seems wrong.\n");
        }

    fcl_close_file(my_test_file);
}


int main(int argc, char **argv)
{

    fprintf(stdout, "Testing libfcl ...\n");
    libfcl_initialize();

    test_openning_and_closing_files();

    return 0;
}

