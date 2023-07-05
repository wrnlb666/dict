# dict
dict in pure C with runtime generic

## Example for quick start
```c
#include "src/dict.h"

#define dict_double( dict, key ) ( *(double*) dict_get( dict, key ) )

int main( void )
{
    dict_t* dict = dict_create( (dict_args_t) { .key = { .type = DICT_I32 }, .val = { .size = sizeof (double) } } );


    for ( size_t i = 0; i < 30; i++ )
    {
        dict_double( dict, i ) = (double)i;
    }

    for ( size_t i = 0; i < 30; i++ )
    {
        printf( "%5f\n", dict_double( dict, i ) );
    }

    dict_destroy( dict );

    return 0;
}
```
