/*
Copyright (c) 2015, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
Written by Mark C. Miller

LLNL-CODE-676051. All rights reserved.

This file is part of MACSio

Please also read the LICENSE file at the top of the source code directory or
folder hierarchy.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License (as published by the Free Software
Foundation) version 2, dated June 1991.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <macsio_data.h>

int main()
{
    struct timeval tv;
    int i;
    int id1 = MACSIO_DATA_CreatePRNG(0xDeadBeef);
    int id2;
    int id3 = MACSIO_DATA_CreatePRNG(0xBabeFace);
    int id5 = MACSIO_DATA_CreatePRNG(0xDeadBeef);
    long tseed;
    long series1[100], series2[100], series3[100], series4[100][5], series5[100][5];

    gettimeofday(&tv, 0);
    tseed = (long) (((unsigned int) (tv.tv_sec + tv.tv_usec)) & 0xFFFFFFFF);
    id2 = MACSIO_DATA_CreatePRNG(tseed); /* time varying output */

    for (i = 0; i < 100; i++)
    {
        int j, id4 = MACSIO_DATA_CreatePRNG(0x0BedFace);
        series1[i] = MACSIO_DATA_GetValPRNG(id1);
        series2[i] = i%3?0:MACSIO_DATA_GetValPRNG(id2);
        series3[i] = i%5?0:MACSIO_DATA_GetValPRNG(id3);
        for (j = 0; j < 5; j++)
            series4[i][j] = MACSIO_DATA_GetValPRNG(id4);
        MACSIO_DATA_DestroyPRNG(id4);
        for (j = 0; j < 5; j++)
            series5[i][j] = MACSIO_DATA_GetValPRNG(id5);
        MACSIO_DATA_ResetPRNG(id5);
        if (i && !(i%50))
            MACSIO_DATA_ResetPRNG(id2);
    }
    if (memcmp(&series2[0], &series2[51], 40*sizeof(long)))
        return 1;
    if (memcmp(&series4[0], &series4[1], 5*sizeof(long)))
        return 1;
    if (memcmp(&series4[1], &series4[23], 5*sizeof(long)))
        return 1;
    if (memcmp(&series5[0], &series5[1], 5*sizeof(long)))
        return 1;
    if (memcmp(&series5[1], &series5[23], 5*sizeof(long)))
        return 1;

    MACSIO_DATA_DestroyPRNG(id1);
    MACSIO_DATA_DestroyPRNG(id2);
    MACSIO_DATA_DestroyPRNG(id3);
    MACSIO_DATA_DestroyPRNG(id5);

    return 0;
}
