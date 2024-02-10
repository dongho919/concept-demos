/* A 3D array can be created with only three allocations.
 * For example, a 2x3x4 array, visualized below, can be created
 *
 *       +--+--+--+--+
 *      /  /  /  /  /|
 *     +--+--+--+--+ |
 *    /  /  /  /  /|/|
 *   +--+--+--+--+ | |
 *  /  /  /  /  /|/|/
 * +--+--+--+--+ | /
 * |  |  |  |  |/|/
 * +--+--+--+--+ /
 * |  |  |  |  |/
 * +--+--+--+--+
 * 
 * - A "slice" is a horizontal layer from the array; there are 2 slices
 * - A "row" is a row of data taken from a particular slice; there are 3 rows in
 *   each slice (2x3 = 6 rows total)
 * - A "data" is an integer taken from a particular row; there are 4 data in
 *   each row (2x3x4 = 24 data total)
 *
 * The rest is just about setting the pointers in the "slice" and "row" arrays
 * correctly.
 */

int *** allocate( int d1, int d2, int d3 ) 
{
    int i, j;
    int* data = malloc(d1 * d2 * d3 * sizeof(int));
    int** row = malloc(d1 * d2 * sizeof(int*));
    int*** slice = malloc(d1 * sizeof(int**));

    for (i = 0; i < d1; ++i)
    {
        slice[i] = row + i * d2;
        for (j = 0; j < d2; ++j)
        {
            slice[i][j] = data + i * d2 * d3 + j * d3;
        }
    }
    return slice;
}

void deallocate( int *** ppp ) 
{
    free(ppp[0][0]);
    free(ppp[0]);
    free(ppp);
}