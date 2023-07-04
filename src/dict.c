#include "dict.h"

#define DEFAULT_MOD     16
#define DEFAULT_STEP    2
#define HASH_BASE       256LLU
#define HASH_MOD        1000000007LLU

typedef struct dict_elem dict_elem_t;
struct dict_elem
{
    void*           key;
    void*           val;
    uint64_t        code;
    dict_elem_t*    prev;
    dict_elem_t*    next;
};

typedef struct dict_list
{
    size_t          size;
    dict_elem_t*    head;
    dict_elem_t*    tail;
} dict_list_t;

struct dict
{
    dict_key_attr_t     key;
    dict_val_attr_t     val;
    dict_alloc_t        alloc;
    size_t              mod;
    dict_list_t*        list;
};


static inline bool dict_reshape( dict_t* dict )
{
    size_t old_size = dict->mod;
    size_t new_size = old_size * DEFAULT_STEP;

    dict_list_t* old_list = dict->list;
    dict_list_t* new_list = dict->alloc.malloc( sizeof (dict_list_t) * new_size );

    if ( new_list == NULL ) return false;

    dict->mod   = new_size;
    dict->list  = new_list;

    for ( size_t i = 0; i < new_size; i++ )
    {
        memset( &( dict->list[i]), 0, sizeof (dict_list_t) );
    }

    for ( size_t i = 0; i < old_size; i++ )
    {
        dict_elem_t* curr = old_list[i].head;
        dict_elem_t* next;
        while ( curr != NULL )
        {
            next            = curr->next;
            uint64_t index  = curr->code % new_size;
            
            if ( new_list[ index ].head == NULL )
            {
                new_list[ index ].head = new_list[ index ].tail = curr;
                curr->prev = curr->next = NULL;
            }
            else
            {
                new_list[ index ].tail->next = curr;
                curr->prev = new_list[ index ].tail;
                curr->next = NULL;
            }
            new_list[ index ].size++;
            curr = next;
        }
    }

    if ( dict->alloc.free != NULL )
    {
        dict->alloc.free( old_list );
    }

    return true;
}


dict_t* dict_create( dict_args_t args )
{
    size_t key_size;
    switch ( args.key.type )
    {
        case DICT_CHAR:    key_size = sizeof ( char );                      break;
        case DICT_WCHAR:   key_size = sizeof ( wchar_t );                   break;
        case DICT_I32:     key_size = sizeof ( int32_t );                   break;
        case DICT_U32:     key_size = sizeof ( uint32_t );                  break;
        case DICT_F32:     key_size = sizeof ( float );                     break;
        case DICT_I64:     key_size = sizeof ( int64_t );                   break;
        case DICT_U64:     key_size = sizeof ( uint64_t );                  break;
        case DICT_F64:     key_size = sizeof ( double );                    break;
        case DICT_PTR:     key_size = sizeof ( void* );                     break;
        case DICT_STR:     key_size = sizeof ( char* );                     break;
        case DICT_STRUCT:  key_size = args.key.size;                        break;
        default:           fprintf( stderr, "[ERRO]: illegal type.\n" );    exit(1);
    }

    size_t val_size = args.val.size;

    dict_t* dict = NULL;
    if ( args.alloc.malloc != NULL )
    {
        dict = args.alloc.malloc( sizeof (dict_t) );
        if ( dict == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        dict->alloc = args.alloc;
    }
    else
    {
        dict = malloc( sizeof (dict_t) );
        if ( dict == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        dict->alloc = (dict_alloc_t)
        {
            .malloc = malloc,
            .free   = free,
        };
    }

    dict->key = args.key;
    dict->key.size = key_size;

    dict->val = args.val;
    dict->val.size = val_size;

    dict->mod   = DEFAULT_MOD;
    dict->list  = dict->alloc.malloc( sizeof (dict_list_t) * dict->mod );
    for ( size_t i = 0; i < dict->mod; i++ )
    {
        memset( &( dict->list[i]), 0, sizeof (dict_list_t) );
    }

    return dict;
}


void dict_destroy( dict_t* dict )
{
    for ( size_t i = 0; i < dict->mod; i++ )
    {
        dict_elem_t* curr = dict->list[i].head;
        dict_elem_t* next;
        while ( curr != NULL )
        {
            next = curr->next;

            if ( dict->key.copy != NULL && dict->key.free != NULL )
            {
                dict->key.free( curr->key );
            }
            else if ( dict->key.type == DICT_STR )
            {
                dict->alloc.free( *(char**) curr->key );
            }
            if ( dict->alloc.free != NULL )
            {
                dict->alloc.free( curr->key );
            }

            if ( dict->val.free != NULL )
            {
                dict->val.free( curr->val );
            }
            if ( dict->alloc.free != NULL )
            {
                dict->alloc.free( curr->val );
            }

            if ( dict->alloc.free != NULL )
            {
                dict->alloc.free( curr );
            }

            curr = next;
        }
    }

    if ( dict->alloc.free != NULL )
    {
        dict->alloc.free( dict->list );
        dict->alloc.free( dict );
        dict = NULL;
    }
}


void* dict_get( dict_t* dict, ... )
{
    va_list ap;
    va_start( ap, dict );

    // get the key
    void* key;
    if ( dict->key.copy != NULL )
    {
        void* data = va_arg( ap, void* );
        key = dict->alloc.malloc( dict->key.size );
        if ( key == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        dict->key.copy( key, data );
    }
    else
    {
        key = dict->alloc.malloc( dict->key.size );
        if ( key == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        switch ( dict->key.type )
        {
            case DICT_CHAR:         *(char*)        key = va_arg( ap, int );          break;
            case DICT_WCHAR:        *(wchar_t*)     key = va_arg( ap, int );          break;
            case DICT_I32:          *(int32_t*)     key = va_arg( ap, int32_t );      break;
            case DICT_U32:          *(uint32_t*)    key = va_arg( ap, uint32_t );     break;
            case DICT_F32:          *(float*)       key = va_arg( ap, double );       break;
            case DICT_I64:          *(int64_t*)     key = va_arg( ap, int64_t );      break;
            case DICT_U64:          *(uint64_t*)    key = va_arg( ap, uint64_t );     break;
            case DICT_F64:          *(double*)      key = va_arg( ap, double );       break;
            case DICT_PTR:          *(void**)       key = va_arg( ap, void* );        break;
            case DICT_STR:
            {
                char* str = va_arg( ap, char* );
                *(char**) key = dict->alloc.malloc( strlen(str) + 1 );
                if ( *(char**) key == NULL )
                {
                    fprintf( stderr, "[ERRO]: out of memory.\n" );
                    exit(1);
                }
                strcpy( *(char**) key, str );
                break;
            }
            case DICT_STRUCT:
            {
                void* data = va_arg( ap, void* );
                memcpy( key, data, dict->key.size );
                break;
            }
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
            }
        }
    }

    va_end(ap);

    // get hash code
    uint64_t code = 0;
    if ( dict->key.hash != NULL )
    {
        code = dict->key.hash( key );
    }
    else
    {
        size_t length;
        switch ( dict->key.type )
        {
            case DICT_CHAR:         code = *(char*)     key;    break;
            case DICT_WCHAR:        code = *(wchar_t*)  key;    break;
            case DICT_I32:          code = *(int32_t*)  key;    break;
            case DICT_U32:          code = *(uint32_t*) key;    break;
            case DICT_F32:          code = *(float*)    key;    break;
            case DICT_I64:          code = *(int64_t*)  key;    break;
            case DICT_U64:          code = *(uint64_t*) key;    break;
            case DICT_F64:          code = *(double*)   key;    break;
            case DICT_PTR:
            {
                code = *(uintptr_t*) key;
                break;
            }
            case DICT_STR:          
                length = strlen( *(char**) key );
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( *(char**) key )[i] ) % HASH_MOD;
                }
                break;
            case DICT_STRUCT:
                length = dict->key.size;
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( (char*) key )[i] ) % HASH_MOD;
                }
                break;
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
            }
        }
    }

    // get into the linked list
    size_t index = code % dict->mod;
    for ( dict_elem_t* curr = dict->list[ index ].head; curr != NULL; curr = curr->next )
    {
        if ( curr->code != code ) continue;
        if ( dict->key.cmpr != NULL )
        {
            if ( dict->key.cmpr( curr->key, key ) == 0 )
            {
                if ( dict->key.copy != NULL && dict->key.free != NULL )
                {
                    dict->key.free( key );
                }
                else if ( dict->key.type == DICT_STR )
                {
                    dict->alloc.free( *(char**) key );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( key );
                }
                return curr->val;
            }
        }
        else
        {
            switch ( dict->key.type )
            {
                case DICT_CHAR:
                case DICT_WCHAR:
                case DICT_I32:
                case DICT_U32:
                case DICT_F32:
                case DICT_I64:
                case DICT_U64:
                case DICT_F64:
                case DICT_PTR:
                case DICT_STRUCT:
                {
                    if ( memcmp( curr->key, key, dict->key.size ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        return curr->val;
                    }
                    break;
                }
                case DICT_STR:
                {
                    if ( strcmp( *(char**) curr->key, *(char**) key ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        else
                        {
                            dict->alloc.free( *(char**) key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        return curr->val;
                    }
                    break;
                }
                default:
                {
                    return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
                }
            }
        }
    }

    // doesn't already appear in the list
    dict_elem_t* elem = dict->alloc.malloc( sizeof (dict_elem_t) );
    if ( elem == NULL )
    {
        fprintf( stderr, "[ERRO]: out of memory.\n" );
        exit(1);
    }
    *elem = (dict_elem_t)
    {
        .code   = code,
        .key    = key,
        .prev   = dict->list[ index ].tail,
        .val    = dict->alloc.malloc( dict->val.size ),
    };
    if ( elem->val == NULL )
    {
        fprintf( stderr, "[ERRO]: out of memory.\n" );
        exit(1);
    }
    if ( dict->list[ index ].size == 0 )
    {
        dict->list[ index ].head = dict->list[ index ].tail = elem;
    }
    else
    {
        dict->list[ index ].tail->next = elem;
        dict->list[ index ].tail = elem;
    }
    if ( dict->list[ index ].size++ > dict->mod )
    {
        if ( dict_reshape(dict) == false )
        {
            return NULL;
        }
    }
    

    return elem->val;
}


bool dict_remove( dict_t* dict, ... )
{
    va_list ap;
    va_start( ap, dict );

    // get the key
    void* key;
    if ( dict->key.copy != NULL )
    {
        void* data = va_arg( ap, void* );
        key = dict->alloc.malloc( dict->key.size );
        if ( key == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        dict->key.copy( key, data );
    }
    else
    {
        key = dict->alloc.malloc( dict->key.size );
        if ( key == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        switch ( dict->key.type )
        {
            case DICT_CHAR:         *(char*)        key = va_arg( ap, int );          break;
            case DICT_WCHAR:        *(wchar_t*)     key = va_arg( ap, int );          break;
            case DICT_I32:          *(int32_t*)     key = va_arg( ap, int32_t );      break;
            case DICT_U32:          *(uint32_t*)    key = va_arg( ap, uint32_t );     break;
            case DICT_F32:          *(float*)       key = va_arg( ap, double );       break;
            case DICT_I64:          *(int64_t*)     key = va_arg( ap, int64_t );      break;
            case DICT_U64:          *(uint64_t*)    key = va_arg( ap, uint64_t );     break;
            case DICT_F64:          *(double*)      key = va_arg( ap, double );       break;
            case DICT_PTR:          *(void**)       key = va_arg( ap, void* );        break;
            case DICT_STR:
            {
                char* str = va_arg( ap, char* );
                *(char**) key = dict->alloc.malloc( strlen(str) + 1 );
                if ( *(char**) key == NULL )
                {
                    fprintf( stderr, "[ERRO]: out of memory.\n" );
                    exit(1);
                }
                strcpy( *(char**) key, str );
                break;
            }
            case DICT_STRUCT:
            {
                void* data = va_arg( ap, void* );
                memcpy( key, data, dict->key.size );
                break;
            }
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
            }
        }
    }

    va_end(ap);

    // get hash code
    uint64_t code = 0;
    if ( dict->key.hash != NULL )
    {
        code = dict->key.hash( key );
    }
    else
    {
        size_t length;
        switch ( dict->key.type )
        {
            case DICT_CHAR:         code = *(char*)     key;    break;
            case DICT_WCHAR:        code = *(wchar_t*)  key;    break;
            case DICT_I32:          code = *(int32_t*)  key;    break;
            case DICT_U32:          code = *(uint32_t*) key;    break;
            case DICT_F32:          code = *(float*)    key;    break;
            case DICT_I64:          code = *(int64_t*)  key;    break;
            case DICT_U64:          code = *(uint64_t*) key;    break;
            case DICT_F64:          code = *(double*)   key;    break;
            case DICT_PTR:
            {
                code = *(uintptr_t*) key;
                break;
            }
            case DICT_STR:          
                length = strlen( *(char**) key );
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( *(char**) key )[i] ) % HASH_MOD;
                }
                break;
            case DICT_STRUCT:
                length = dict->key.size;
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( (char*) key )[i] ) % HASH_MOD;
                }
                break;
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
            }
        }
    }

    // get into the linked list
    size_t index = code % dict->mod;
    for ( dict_elem_t* curr = dict->list[ index ].head; curr != NULL; curr = curr->next )
    {
        if ( curr->code != code ) continue;
        if ( dict->key.cmpr != NULL )
        {
            if ( dict->key.cmpr( curr->key, key ) == 0 )
            {
                if ( dict->key.copy != NULL && dict->key.free != NULL )
                {
                    dict->key.free( key );
                }
                else if ( dict->key.type == DICT_STR )
                {
                    dict->alloc.free( *(char**) key );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( key );
                }
                // redirect node
                if ( curr == dict->list[ index ].head )
                {
                    dict->list[ index ].head = curr->next;
                }
                if ( curr == dict->list[ index ].tail )
                {
                    dict->list[ index ].tail = curr->prev;
                }
                if ( curr->prev != NULL )
                {
                    curr->prev->next = curr->next;
                }
                if ( curr->next != NULL )
                {
                    curr->next->prev = curr->prev;
                }
                dict->list[ index ].size--;
                // delete key and val and node
                if ( dict->key.copy != NULL && dict->key.free != NULL )
                {
                    dict->key.free( curr->key );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( curr->key );
                }
                if ( dict->val.free != NULL )
                {
                    dict->val.free( curr->val );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( curr->val );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( curr );
                }
                return true;
            }
        }
        else
        {
            switch ( dict->key.type )
            {
                case DICT_CHAR:
                case DICT_WCHAR:
                case DICT_I32:
                case DICT_U32:
                case DICT_F32:
                case DICT_I64:
                case DICT_U64:
                case DICT_F64:
                case DICT_PTR:
                case DICT_STRUCT:
                {
                    if ( memcmp( curr->key, key, dict->key.size ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        // redirect node
                        if ( curr == dict->list[ index ].head )
                        {
                            dict->list[ index ].head = curr->next;
                        }
                        if ( curr == dict->list[ index ].tail )
                        {
                            dict->list[ index ].tail = curr->prev;
                        }
                        if ( curr->prev != NULL )
                        {
                            curr->prev->next = curr->next;
                        }
                        if ( curr->next != NULL )
                        {
                            curr->next->prev = curr->prev;
                        }
                        dict->list[ index ].size--;
                        // delete key and val and node
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( curr->key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr->key );
                        }
                        if ( dict->val.free != NULL )
                        {
                            dict->val.free( curr->val );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr->val );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr );
                        }
                        return true;
                    }
                    break;
                }
                case DICT_STR:
                {
                    if ( strcmp( curr->key, key ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        else
                        {
                            dict->alloc.free( *(char**) key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        // redirect node
                        if ( curr == dict->list[ index ].head )
                        {
                            dict->list[ index ].head = curr->next;
                        }
                        if ( curr == dict->list[ index ].tail )
                        {
                            dict->list[ index ].tail = curr->prev;
                        }
                        if ( curr->prev != NULL )
                        {
                            curr->prev->next = curr->next;
                        }
                        if ( curr->next != NULL )
                        {
                            curr->next->prev = curr->prev;
                        }
                        dict->list[ index ].size--;
                        // delete key and val and node
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( curr->key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr->key );
                        }
                        if ( dict->val.free != NULL )
                        {
                            dict->val.free( curr->val );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr->val );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( curr );
                        }
                        return true;
                    }
                    break;
                }
                default:
                {
                    return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
                }
            }
        }
    }

    // doesn't already appear in the list
    if ( dict->key.copy != NULL && dict->key.free != NULL )
    {
        dict->key.free( key );
    }
    else if ( dict->alloc.free != NULL )
    {
        dict->alloc.free( key );
    }
    return false;
}


bool dict_has( const dict_t* dict, ... )
{
    va_list ap;
    va_start( ap, dict );

    // get the key
    void* key;
    if ( dict->key.copy != NULL )
    {
        void* data = va_arg( ap, void* );
        key = dict->alloc.malloc( dict->key.size );
        if ( key == NULL )
        {
            fprintf( stderr, "[ERRO]: out of memory.\n" );
            exit(1);
        }
        dict->key.copy( key, data );
    }
    else
    {
        if ( dict->key.type != DICT_STR )
        {
            key = dict->alloc.malloc( dict->key.size );
            if ( key == NULL )
            {
                fprintf( stderr, "[ERRO]: out of memory.\n" );
                exit(1);
            }
        }
        switch ( dict->key.type )
        {
            case DICT_CHAR:         *(char*)        key = va_arg( ap, int );          break;
            case DICT_WCHAR:        *(wchar_t*)     key = va_arg( ap, int );          break;
            case DICT_I32:          *(int32_t*)     key = va_arg( ap, int32_t );      break;
            case DICT_U32:          *(uint32_t*)    key = va_arg( ap, uint32_t );     break;
            case DICT_F32:          *(float*)       key = va_arg( ap, double );       break;
            case DICT_I64:          *(int64_t*)     key = va_arg( ap, int64_t );      break;
            case DICT_U64:          *(uint64_t*)    key = va_arg( ap, uint64_t );     break;
            case DICT_F64:          *(double*)      key = va_arg( ap, double );       break;
            case DICT_PTR:          *(void**)       key = va_arg( ap, void* );        break;
            case DICT_STR:
            {
                char* str = va_arg( ap, char* );
                key = dict->alloc.malloc( strlen(str) + 1 );
                if ( key == NULL )
                {
                    fprintf( stderr, "[ERRO]: out of memory.\n" );
                    exit(1);
                }
                strcpy( key, str );
                break;
            }
            case DICT_STRUCT:
            {
                void* data = va_arg( ap, void* );
                memcpy( key, data, dict->key.size );
                break;
            }
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), false );
            }
        }
    }

    va_end(ap);

    // get hash code
    uint64_t code = 0;
    if ( dict->key.hash != NULL )
    {
        code = dict->key.hash( key );
    }
    else
    {
        size_t length;
        switch ( dict->key.type )
        {
            case DICT_CHAR:         code = *(char*)     key;    break;
            case DICT_WCHAR:        code = *(wchar_t*)  key;    break;
            case DICT_I32:          code = *(int32_t*)  key;    break;
            case DICT_U32:          code = *(uint32_t*) key;    break;
            case DICT_F32:          code = *(float*)    key;    break;
            case DICT_I64:          code = *(int64_t*)  key;    break;
            case DICT_U64:          code = *(uint64_t*) key;    break;
            case DICT_F64:          code = *(double*)   key;    break;
            case DICT_PTR:
            {
                code = *(uintptr_t*) key;
                break;
            }
            case DICT_STR:          
                length = strlen( *(char**) key );
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( *(char**) key )[i] ) % HASH_MOD;
                }
                break;
            case DICT_STRUCT:
                length = dict->key.size;
                for ( size_t i = 0; i < length; i++ )
                {
                    code = ( code * HASH_BASE + ( (char*) key )[i] ) % HASH_MOD;
                }
                break;
            default:
            {
                return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
            }
        }
    }

    // get into the linked list
    size_t index = code % dict->mod;
    for ( dict_elem_t* curr = dict->list[ index ].head; curr != NULL; curr = curr->next )
    {
        if ( curr->code != code ) continue;
        if ( dict->key.cmpr != NULL )
        {
            if ( dict->key.cmpr( curr->key, key ) == 0 )
            {
                if ( dict->key.copy != NULL && dict->key.free != NULL )
                {
                    dict->key.free( key );
                }
                else if ( dict->key.type == DICT_STR )
                {
                    dict->alloc.free( *(char**) key );
                }
                if ( dict->alloc.free != NULL )
                {
                    dict->alloc.free( key );
                }
                return true;
            }
        }
        else
        {
            switch ( dict->key.type )
            {
                case DICT_CHAR:
                case DICT_WCHAR:
                case DICT_I32:
                case DICT_U32:
                case DICT_F32:
                case DICT_I64:
                case DICT_U64:
                case DICT_F64:
                case DICT_PTR:
                case DICT_STRUCT:
                {
                    if ( memcmp( curr->key, key, dict->key.size ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        return true;
                    }
                    break;
                }
                case DICT_STR:
                {
                    if ( strcmp( curr->key, key ) == 0 )
                    {
                        if ( dict->key.copy != NULL && dict->key.free != NULL )
                        {
                            dict->key.free( key );
                        }
                        else
                        {
                            dict->alloc.free( *(char**) key );
                        }
                        if ( dict->alloc.free != NULL )
                        {
                            dict->alloc.free( key );
                        }
                        return true;
                    }
                    break;
                }
                default:
                {
                    return ( fprintf( stderr, "[ERRO]: illegal type.\n" ), exit(1), NULL );
                }
            }
        }
    }

    // doesn't already appear in the list
    if ( dict->key.copy != NULL && dict->key.free != NULL )
    {
        dict->key.free( key );
    }
    else if ( dict->alloc.free != NULL )
    {
        dict->alloc.free( key );
    }

    return false;
}


const void* dict_key( const dict_t* dict, size_t* size )
{
    *size = 0;
    for ( size_t i = 0; i < dict->mod; i++ )
    {
        *size += dict->list[i].size;
    }
    char* arr = dict->alloc.malloc( (*size) * dict->key.size );

    size_t index = 0;
    for ( size_t i = 0; i < dict->mod; i++ )
    {
        for ( dict_elem_t* curr = dict->list[i].head; curr != NULL; curr = curr->next )
        {
            memcpy( arr + ( dict->key.size * index ), curr->key, dict->key.size );
            if ( index++ == *size ) break;
        }
    }

    return arr;
}
