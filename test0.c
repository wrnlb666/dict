#include "src/dict.h"
#include <stdint.h>

#define dict_double( dict, key ) ( *(double*) dict_get( dict, key ) )

int main( void )
{
    dict_t* dict = dict_new( DICT_I32, 0, sizeof (double) );

    for ( int32_t i = 0; i < 30; i++ )
    {
        dict_double( dict, i ) = (double)i;
    }

    for ( int32_t i = 0; i < 30; i++ )
    {
        printf( "%5lf\n", dict_double( dict, i ) );
    }

    dict_destroy( dict );

    return 0;
}