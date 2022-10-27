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

#ifndef LOG_H
#include <stdio.h>

#ifdef DEBUG
#define print_loge(fmt, ...)                                                   \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, fmt, ##__VA_ARGS__);                                   \
    } while (0)

#else /* DEBUG */
#define print_loge(fmt, ...)

#endif /* DEBUG */
#endif /* LOG_H_ */
