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
 * @struct fcl_file_t
 * Structure that contains all the definitions needed by the library for a
 * file
 */
typedef struct
{
    gchar *name;    /**< name for the file */
    guint64 size;   /**< file's size       */
} fcl_file_t;


endif /* _LIBFCL_H_ */
