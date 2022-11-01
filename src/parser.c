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

#include "parser.h"
#include "log.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Dynamic_string generate_dyn_str(const char *str)
{
    Dynamic_string dyn_str;
    char *start;
    char *end = strchr(str, '\0');
    const char *lead = str;
    int index;
    int s_result;
    int loop = 0;
    char buffer[strlen(str) + 1];
    int buffer_size = 0;
    buffer[0] = '\0';

    dyn_str.inserts = 0;
    dyn_str.count_strings = 0;

    while (loop == 0)
    {
        start = strchr(lead, '{');
        if (start == NULL)
        {
            loop = 1;
            break;
        }
        else if (end - start > 1)
        {
            if (isdigit(start[1])) // Define if the next symbol
            {                      // after { is digit
                s_result = sscanf(&start[1], "%d", &index);

                if (start[s_result + 1] != '}') // Check if } exists after
                {                               // number
                    loop = 1;
                    break;
                }

                dyn_str.indexes[dyn_str.inserts] = index; // set indexes
                dyn_str.inserts++;

                /* Copy text before { to the buffer */
                strncat(buffer, lead, start - lead);
                buffer[buffer_size + start - lead + 1] = '\0';
                buffer_size += start - lead;

                lead = start + s_result + 2;

                dyn_str.strings[dyn_str.count_strings] =
                    (char *)malloc(sizeof(char) * strlen(buffer) + 1);
                strncpy(dyn_str.strings[dyn_str.count_strings], buffer,
                        strlen(buffer) + 1);
                dyn_str.count_strings++;
                buffer[0] = '\0';
                buffer_size = 0;

                continue;
            }
            else if (start[1] == '{')
            {
                strncat(buffer, lead, start - lead);
                buffer[buffer_size + start - lead + 1] = '\0';
                buffer_size += start - lead;
                buffer[buffer_size - 0] = '{';

                lead = start + 2;
                continue;
            }
            else
            {
                fprintf(stderr, "Error: Not correct syntax, ignore the end of"
                                " the string.\n");
                loop = 1;
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error: something goes wrong.\n");
        }
    }

    switch (loop)
    {
    case 0:
        break;
    case 1:
        strncat(buffer, lead, end - lead);
        buffer[buffer_size + end - lead + 1] = '\0';
        buffer_size += end - lead;

        dyn_str.strings[dyn_str.count_strings] =
            (char *)malloc(sizeof(char) * strlen(buffer) + 1);
        strncpy(dyn_str.strings[dyn_str.count_strings], buffer,
                strlen(buffer) + 1);
        dyn_str.count_strings++;
        buffer[0] = '\0';
        buffer_size = 0;

        break;
    default:
        break;
    }

    int dyn_str_len = 0;
    int i;
    /* Count length of dynamic_str */
    for (i = 0; i < dyn_str.count_strings; i++)
    {
        dyn_str_len += strlen(dyn_str.strings[i]);
    }
    dyn_str.len = dyn_str_len;

    return dyn_str;
}

void free_dyn_str(Dynamic_string *pdyn_str)
{
    int i;
    for (i = 0; i < pdyn_str->count_strings; i++)
        free(pdyn_str->strings[i]);
    return;
}

int strlen_dyn_str(const Dynamic_string *pdyn_str) { return pdyn_str->len; }

int fill_dyn_str(char *str, Dynamic_string *pdyn_str, char **words_list,
                 int words_list_len)
{
    int i;
    int status = 0;
    str[0] = '\0';

    /* Fill dynamic string */
    for (i = 0; i < pdyn_str->count_strings - 1; i++)
    {
        if (pdyn_str->indexes[i] >= words_list_len)
            return 0;
        strcat(str, pdyn_str->strings[i]);
        strcat(str, words_list[pdyn_str->indexes[i]]);
        status++;
    }
    strcat(str, pdyn_str->strings[pdyn_str->count_strings - 1]);
    return status;
}

char *parse_splitted(char *str)
{
    static char *input;
    char *lead;
    bool in_block = false;
    uint8_t block_index;
    char *p_block_index;
    char *block_chars = "\"'";
    size_t string_len;

    if (str != NULL)
    {
        input = str;
        lead = input;
    }
    else
    {
        if (input[0] == '\0')
            return NULL;
        lead = input;
    }
    while (input[0] != '\0')
    {
        if (in_block)
        {
            // printf("input is [%s]\n", input);
            // printf("in_block block_index[%d]\n", block_index);
            p_block_index = strchr(input, block_chars[block_index]);
            // printf("p_block_index [%s]\n", p_block_index);
            in_block = false;
            if (p_block_index)
            {
                // printf("found end\n");
                // printf("p_block_index[block_index] is [%c]\n",
                // block_chars[block_index]); printf("p_block_index[%p]\n",
                // p_block_index);
                input = p_block_index + 1;
                // printf("input [%s]\n", input);
                input[-1] = '\0';
                // in_block = 0;
                break;
                // continue;
            }
            // else
            // {
            //     in_block = 0;
            // }
        }
        else if ((p_block_index = strchr(block_chars, input[0])) != NULL)
        {
            // printf("input[0] is [%c]\n", input[0]);
            // printf("p_block_index[%p]\n", p_block_index);
            block_index = p_block_index - block_chars;
            // printf("block_index[%d]\n", block_index);
            // printf("lead [%s]\n", lead);
            in_block = true;
            lead++;
            input++;
            continue;
        }

        /* Allow escape special symbols with \ */
        if (input[0] == '\\')
        {
            switch (input[1])
            {
            case '\\':
            case ' ':
            case '\'':
            case '"':
                string_len = strlen(input + 1);
                memmove(input, input + 1, string_len + 1);
                input++;
                continue;
                break;
            case 'n':
            case 't':
                fprintf(stderr, "Warning: \\n and \\t are not supported by"
                                " xob, don't use them (we may add them in the"
                                " future\n");
                input += 2;
                continue;
            case '\0':
                fprintf(stderr, "Warning: no specifier found after \\\n");
                input++;
                continue;
                break;
            default:
                fprintf(stderr,
                        "Warning: \\%c is not correct escape"
                        " sequece, don't use them\n",
                        input[1]);
                input += 2;
                continue;
                break;
            }
        }

        if (input[0] == ' ')
        {
            if (input - lead == 0)
            {
                lead++;
                input++;
                continue;
            }

            if (input[0] != '\0')
            {
                input[0] = '\0';
                input++;
            }
            break;
        }
        input++;
    }
    return lead;
}
