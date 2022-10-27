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
    int len;
} Dynamic_string;

typedef struct
{
    char *strings[MAX_DYN_STR_SIZE];
    int count;
} Dynamic_list;

/* Generate dynamic string structure by usual string */
Dynamic_string generate_dyn_str(const char *str);

/* free all allocated memory by generate_dyn_str function */
void free_dyn_str(Dynamic_string *pdyn_str);

/* Get len of provided dynamic string */
int strlen_dyn_str(const Dynamic_string *pdyn_str);

/* Fill str buffer with combined pdyn_str and words_list.
 * Every {num} will be changed with words_list[num] string.
 * words_list have to have enough elements to represent all numbers in the
 * dynamic string. */
int fill_dyn_str(char *str, Dynamic_string *pdyn_str, char **words_list,
                 int words_list_len);

/* Split string to blocks that separates by "' chars.
 * if str is a pointer to string than the function returns pointer to the first
 * block in the string, if str is NULL pointer then the function returns
 * pointer to the next block in previous string or returns NULL pointer if
 * no block found */
char *parse_splitted(char *str);

#endif
