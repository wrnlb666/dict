#include "src/dict.h"
#include <stdint.h>

#define dict_double( dict, key ) ( *(double*) dict_get( dict, key ) )

int main( void )
{
    dict_t* dict1 = dict_create( (dict_args_t) { .key = { .type = DICT_I32 }, .val = { .size = sizeof (double) } } );

    for ( int32_t i = 0; i < 30; i++ )
    {
        dict_double( dict1, i ) = (double)i;
    }

    FILE* fp1 = fopen( "test3.bin", "wb+" );
    if ( !dict_serialize( dict1, fp1 ) )
    {
        fprintf( stderr, "Fail to serialize.\n" );
        exit(1);
    }
    fclose( fp1 );
    dict_destroy( dict1 );

    FILE* fp2 = fopen( "test3.bin", "rb" );
    dict_t* dict2 = dict_deserialize( (dict_args_t) { .key = { .type = DICT_I32 }, .val = { .size = sizeof (double) } }, fp2 );
    if ( dict2 == NULL )
    {
        fprintf( stderr, "Fail to deserialize.\n" );
        exit(1);
    }
    fclose( fp2 );

    size_t len = dict_len( dict2 );
    for ( int32_t i = 0; i < (int32_t) len; i++ )
    {
        printf( "%5lf\n", dict_double( dict2, i ) );
    }
    dict_destroy( dict2 );

    return 0;
}
