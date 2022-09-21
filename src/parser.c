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

#include <ctype.h>
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

    return dyn_str;
}

void free_dyn_str(Dynamic_string *pdyn_str)
{
    int i;
    for (i = 0; i < pdyn_str->count_strings; i++)
        free(pdyn_str->strings[i]);
    return;
}
