#include "src/dict.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"


#define dict_double( dict, key ) ( *(double*) dict_get( dict, key ) )

arena_t arena = { 0 };

void* arena_malloc( size_t size )
{
    return arena_alloc( &arena, size );
}

int main( void )
{
    dict_t* dict = dict_create( (dict_args_t) { .key = { .type = DICT_I32 }, .val = { .size = sizeof (double) }, .alloc = { .malloc = arena_malloc } } );


    for ( size_t i = 0; i < 30; i++ )
    {
        dict_double( dict, i ) = (double)i;
    }

    for ( size_t i = 0; i < 30; i++ )
    {
        printf( "%5f\n", dict_double( dict, i ) );
    }

    dict_destroy( dict );
    arena_free( &arena );

    return 0;
}
