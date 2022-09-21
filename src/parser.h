/* xob - A lightweight overlay volume/anything bar for the X Window System.
 * Copyright (C) 2022 Smoliarchuk Illia
 *
 * xob is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xob is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xob.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#define MAX_DYN_STR_SIZE 10

typedef struct
{
    int indexes[MAX_DYN_STR_SIZE];
    char *strings[MAX_DYN_STR_SIZE];
    int inserts;
    int count_strings;
} Dynamic_string;

/* Generate dynamic string structure by usual string */
Dynamic_string generate_dyn_str(const char *str);

/* free all allocated memory by generate_dyn_str function */
void free_dyn_str(Dynamic_string *pdyn_str);

#endif
