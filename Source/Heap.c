/*subfile:  heap.c  **********************************************************/

/*   functions for heap (memory) processing  By George*/

/* MEMTEST: 0 = no tests; 1 = test guard bytes; 2 = and log actions */
#define _CRT_SECURE_NO_DEPRECATE 1
#ifdef _DEBUG
# define MEMTEST 1
#else
# define MEMTEST 0
#endif

#include <stdio.h>
#include <string.h> /* prototypes: memset, ... */
#include <stdlib.h> /* prototype: malloc, free */
#include "Types.h"  /* define U1, I2, etc.  */
#include "Prtyp.h"  /* miscellaneous function prototypes */
#include "Sxtrn.h"  /* external variables */

#ifdef __TURBOC__              /* requires TURBO C compiler */
# include <alloc.h> /* prototype: heapcheck, heapwalk */
#endif

#define MCHECK 0x5A5A5A5AL

I4 _bytesAllocated=0L;
I4 _bytesFreed=0L;

//#if( _MSC_VER )
/***  memrem.c  **************************************************************/

void memrem( I1 *msg )
  {
  fprintf( _ulog, "%s: %ld bytes allocated, %ld freed, %ld net\n",
    msg, _bytesAllocated, _bytesFreed, _bytesAllocated - _bytesFreed );
  }  /* end of memrem */
//#endif

/***  alc_e.c  ***************************************************************/

/*  Allocate memory for a single element, i.e. contiguous portion of
 *  the heap.  This may be a single structure or a vector.  */

void *alc_e( UX length, I1 *name )
/*  length; length of element (bytes).
 *  name;   name of variable being allocated.  */
  {
  U1 *p;     /* pointer to allocated memory */
#if( MEMTEST > 0 )
  U4 *pt;    /* pointer to heap guard bytes */
#endif

#ifdef __TURBOC__  //  (16-bit limit: 2^16-1 = 65536 = 8192*8)
  if( length > 65512L ) error( 3, __FILE__, __LINE__,
    name, " too large to allocate", "" );
#endif
  if( length <= 0L ) error( 3, __FILE__, __LINE__,
    name, " too small to allocate", "" );

#if( MEMTEST > 0 )
  p = (U1 *)malloc( length+8 );
  _bytesAllocated += length+8;
#else
  p = (U1 *)malloc( length );
  _bytesAllocated += length;
#endif

#if( MEMTEST > 1 )
  fprintf( _ulog, "alc_e %5u bytes at [%p] for: %s\n", length, p, name );
  fflush( _ulog );
#endif

  if( p == NULL )
    {
    memrem( "alc_e" );
    fprintf( _ulog, "Attempting to allocate %u bytes\n", length );
    error( 3, __FILE__, __LINE__, "Insufficient memory to allocate: ", name, "" );
    }

#if( MEMTEST > 0 )
  pt = (U4 *)p;
  *pt = MCHECK;      /* set guard bytes */
  p+= 4;
  pt = (U4 *)(p+length);
  *pt = MCHECK;
#endif

  memset( p, 0, length );    /* zero the vector */

  return (void *)p;

  }  /*  end of alc_e  */

/***  chk_e.c  ***************************************************************/

/*  Check pointer to memory allocated by ALC_E().
//  Return non-zero if heap is in error.  */

IX chk_e( void *pm, UX length, I1 *name )
/*  *pm;    pointer to allocated memory.
 *  length; length of element (bytes).
 *  name;   name of variable being checked.  */
  {
  IX status=0;
#if( MEMTEST > 0 )
  U1 *p;     /* pointer to allocated memory */
  U4 *pt;    /* pointer to guard bytes */

  p = (U1 *)pm + length;
  pt = (U4 *)p;
  if( *pt != MCHECK )
    {
    error( 2, __FILE__, __LINE__, "Overrun at end of: ", name, "" );
    status = 1;
    }
  p = (U1 *)pm - 4;
  pt = (U4 *)p;
  if( *pt != MCHECK )
    {
    error( 2, __FILE__, __LINE__, "Overrun at start of: ", name, "" );
    status = 1;
    }
  pm = (void *)p;
#endif

#if (__TURBOC__ >= 0x295)      /* requires Turbo C++ compiler */
  switch( heapchecknode( pm ) )
    {
    case _HEAPEMPTY:
      error( 2, __FILE__, __LINE__, "The heap is empty: ", name, "" );
      status = 1;
      break;
    case _HEAPCORRUPT:
      error( 2, __FILE__, __LINE__, "The heap is corrupted: ", name, "" );
      status = 1;
      break;
    case _BADNODE:
      error( 2, __FILE__, __LINE__, "Bad heap node entry: ", name, "" );
      status = 1;
      break;
    case _FREEENTRY:
      error( 2, __FILE__, __LINE__, "Free heap node entry: ", name, "" );
      status = 1;
      break;
    case _USEDENTRY:
      break;
    default:
      error( 2, __FILE__, __LINE__, "Unknown heap node status: ", name, "" );
      break;
    }  /* end switch */
#endif

  return status;

  }  /*  end of chk_e  */

/***  fre_e.c  ***************************************************************/

/*  Free pointer to previously memory allocated by ALC_E().
 *  Includes a memory check.  */

void *fre_e( void *pm, UX length, I1 *name )
/*  *pm;    pointer to allocated memory.
 *  length; length of element (bytes).
 *  name;   name of variable being freed.  */
  {
  IX test=0;
#if( MEMTEST > 0 )
  U4 *pt;          /* pointer to guard bytes */

  test = chk_e( (void *)pm, length, name );

  pt = (U4 *)pm;
  pt -= 1;
  pm = (void *)pt;
# if( MEMTEST > 1 )
  fprintf( _ulog, "fre_e %5u bytes at [%p] for: %s\n", length, pm, name );
  fflush( _ulog );
# endif
#endif

  if( !test )
    {
#if( MEMTEST > 0 )
  _bytesFreed += length+8;
#else
  _bytesFreed += length;
#endif
    free( pm );
    }

  return (NULL);

  }  /*  end of fre_e  */

/***  alc_v.c  ***************************************************************/

/*  Allocate pointer for a vector with optional debugging data.
 *  This vector is accessed and stored as:
 *     v[i] v[i+1] v[i+2] ... v[N-1] v[N]
 *  where i is the minimum vector index and N is the maximum index --
 *  i need not equal zero; i may be less than zero.
 */

void *alc_v( IX min_index, IX max_index, IX size, I1 *name )
/*  min_index;  minimum vector index:  vector[min_index] valid.
 *  max_index;  maximum vector index:  vector[max_index] valid.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
  {
  I1 *p;      /* pointer to the vector */
  UX length;  /* length of vector (bytes) */

  if( max_index < min_index )
    {
    sprintf( _string, "Max index(%d) < min index(%d): %s",
      max_index, min_index, name );
    error( 3, __FILE__, __LINE__, _string, "" );
    }

  length = (UX)(max_index - min_index + 1) * size;
  p = (I1 *)alc_e( length, name );
  p -= min_index * size;

  return ((void *)p);

  }  /*  end of alc_v  */

#if( MEMTEST > 0 )
/***  chk_v.c  ***************************************************************/

/*  Check a vector allocated by ALC_V().  */

void chk_v( void *v, IX min_index, IX max_index, IX size, I1 *name )
/*  v;      pointer to allocated vector.
 *  min_index;  minimum vector index.
 *  max_index;  maximum vector index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being checked.
 */
  {
  I1 *p;      /* pointer to the vector */
  UX length;  /* number of bytes in vector elements */

  p = (I1 *)v + min_index * size;
  length = (UX)(max_index - min_index + 1) * size;
  chk_e( (void *)p, length, name );

  }  /*  end of chk_v  */
#endif

/***  fre_v.c  ***************************************************************/

/*  Free pointer to a vector allocated by ALC_V().  */

void *fre_v( void *v, IX min_index, IX max_index, IX size, I1 *name )
/*  v;      pointer to allocated vector.
 *  min_index;  minimum vector index.
 *  max_index;  maximum vector index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being freed.
 */
  {
  I1 *p;      /* pointer to the vector */
  UX length;  /* number of bytes in vector elements */

  p = (I1 *)v + min_index * size;
  length = (UX)(max_index - min_index + 1) * size;
  fre_e( (void *)p, length, name );

  return (NULL);

  }  /*  end of fre_v  */

/***  alc_mc.c  **************************************************************/

/*  Allocate (contiguously) a matrix, M[i][j].
 *  This matrix is accessed (and stored) in a rectangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][j  ] | [i  ][j+1] | [i  ][j+2] | [i  ][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+1][j  ] | [i+1][j+1] | [i+1][j+2] | [i+1][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+2][j  ] | [i+2][j+1] | [i+2][j+2] | [i+2][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+3][j  ] | [i+3][j+1] | [i+3][j+2] | [i+3][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *  where i is the minimum row index and
 *  j is the minimum column index (usually 0 or 1).
 */

void *alc_mc( IX min_row_index, IX max_row_index, IX min_col_index,
              IX max_col_index, IX size, I1 *name )
/*  min_row_index;  minimum row index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
  {  // prow - vector of pointers to rows of A
  IX nrow = max_row_index - min_row_index + 1;       // number of rows [i]
  IX tot_row_index = min_row_index + nrow - 1;       // max prow index
  I1 **prow = (I1 **)alc_v( min_row_index, tot_row_index, sizeof(I1 *), name );

     // pdata - vector of contiguous A[i][j] data values
  IX ncol = max_col_index - min_col_index + 1;       // number of columns [j]
  IX tot_col_index = min_col_index + nrow*ncol - 1;  // max pdata index
  I1 *pdata = (I1 *)alc_v( min_col_index, tot_col_index, size, name );
     // Note: must have nrow >= 1 and ncol >= 1.
     // If nrow < 1, allocation of prow will fail with fatal error message.
     // If ncol < 1, allocation of pdata will fail since nrow > 0.

  IX n, m;
  for( m=0,n=min_row_index; n<=tot_row_index; n++ )
    prow[n] = pdata + ncol*size*m++;  // set row pointers

  return ((void *)prow);

  }  /*  end of alc_mc  */

#if( MEMTEST > 0 )
/***  chk_mc.c  **************************************************************/

/*  Check a matrix allocated by ALC_MC().  */

void chk_mc( void *m, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name )
  {
  IX nrow = max_row_index - min_row_index + 1;
  IX tot_row_index = min_row_index + nrow - 1;
  I1 **prow = (I1 **)m;
  IX ncol = max_col_index - min_col_index + 1;
  IX tot_col_index = min_col_index + nrow*ncol - 1;
  I1 *pdata = prow[min_row_index];

  chk_v( pdata, min_col_index, tot_col_index, size, name );
  chk_v( prow, min_row_index, tot_row_index, sizeof(I1 *), name );

  }  /*  end of chk_mc  */
#endif

/***  fre_mc.c  **************************************************************/

/*  Free pointer to a matrix allocated by ALC_MC().  */

void *fre_mc( void *m, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name )
  {
  IX nrow = max_row_index - min_row_index + 1;
  IX tot_row_index = min_row_index + nrow - 1;
  I1 **prow = (I1 **)m;
  IX ncol = max_col_index - min_col_index + 1;
  IX tot_col_index = min_col_index + nrow*ncol - 1;
  I1 *pdata = prow[min_row_index];

  fre_v( pdata, min_col_index, tot_col_index, size, name );
  fre_v( prow, min_row_index, tot_row_index, sizeof(I1 *), name );

  return (NULL);

  }  /*  end of fre_mc  */

/***  alc_ac.c  **************************************************************/

/*  Allocate (contiguously) a 3D array, A[i][j][k], - original by Leon Wang  */

void *alc_ac( IX min_plane_index, IX max_plane_index, IX min_row_index,
  IX max_row_index, IX min_col_index, IX max_col_index, IX size, I1 *name )
/*  min_plane_index; minimum plane [i] index.
 *  max_palne_index; maximum plane index.
 *  min_row_index;  minimum row [j] index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column [k] index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
  {  // ppln - vector of pointers to planes
  IX npln = max_plane_index - min_plane_index + 1;   // number of planes [i]
  IX tot_pln_index = min_plane_index + npln - 1;     // ppln max index
  I1 ***ppln = (I1 ***)alc_v( min_plane_index, tot_pln_index, sizeof(I1 **), name );

     // prow - vector of pointers to rows for all planes
  IX nrow = max_row_index - min_row_index + 1;       // number of rows [j]
  IX tot_row_index = min_row_index + npln*nrow - 1;  // prow max index
  I1 **prow = (I1 **)alc_v( min_row_index, tot_row_index, sizeof(I1 *), name );

     // pdata - vector of contiguous A[i][j][k] data values
  IX ncol = max_col_index - min_col_index + 1;       // number of columns [k]
  IX tot_col_index = min_col_index + npln*nrow*ncol - 1; // pdata max index
  I1 *pdata = (I1 *)alc_v( min_col_index, tot_col_index, size, name );
     // Note: must have npln >= 1, nrow >= 1 and ncol >= 1.
     // If npln < 1, allocation of ppln will fail with fatal error message.
     // If nrow < 1, allocation of prow will fail since npln > 0.
     // If ncol < 1, allocation of pdata will fail since npln*nrow > 0.

  IX n, m;
  for( m=0,n=min_row_index; n<=tot_row_index; n++ )
    prow[n] = pdata + ncol*size*m++;  // set row pointers

  for(m=0,n=min_plane_index; n<=tot_pln_index; n++ )
    ppln[n] = prow + nrow*m++;  // set plane pointers

  return ((void *)ppln);

  }  /* end alc_ac  */

#if( MEMTEST > 0 )
/***  chk_ac.c  **************************************************************/

/*  Check a 3D array allocated by ALC_AC.  */

void chk_ac( void *a, IX min_plane_index, IX max_plane_index, IX min_row_index,
  IX max_row_index, IX min_col_index, IX max_col_index, IX size, I1 *name )
  {
  IX npln = max_plane_index - min_plane_index + 1;
  IX tot_pln_index = min_plane_index + npln - 1;
  I1 ***ppln = (I1 ***)a;
  IX nrow = max_row_index - min_row_index + 1;
  IX tot_row_index = min_row_index + npln*nrow - 1;
  I1 **prow = ppln[min_plane_index];
  IX ncol = max_col_index - min_col_index + 1;
  IX tot_col_index = min_col_index + npln*nrow*ncol - 1;
  I1 *pdata = prow[min_row_index];

  chk_v( pdata, min_col_index, tot_col_index, size, name );
  chk_v( prow, min_row_index, tot_row_index, sizeof(I1 *), name );
  chk_v( ppln, min_plane_index, tot_pln_index, sizeof(I1 **), name );

  }  /* end chk_ac  */
#endif

/***  fre_ac.c  **************************************************************/

/*  Free pointer to a 3D array allocated by ALC_AC().  */

void *fre_ac( void *a, IX min_plane_index, IX max_plane_index, IX min_row_index,
  IX max_row_index, IX min_col_index, IX max_col_index, IX size, I1 *name )
  {
  IX npln = max_plane_index - min_plane_index + 1;
  IX tot_pln_index = min_plane_index + npln - 1;
  I1 ***ppln = (I1 ***)a;
  IX nrow = max_row_index - min_row_index + 1;
  IX tot_row_index = min_row_index + npln*nrow - 1;
  I1 **prow = ppln[min_plane_index];
  IX ncol = max_col_index - min_col_index + 1;
  IX tot_col_index = min_col_index + npln*nrow*ncol - 1;
  I1 *pdata = prow[min_row_index];

    // free vectors in opposite order from allocation
  fre_v( pdata, min_col_index, tot_col_index, size, name );
  fre_v( prow, min_row_index, tot_row_index, sizeof(I1 *), name );
  fre_v( ppln, min_plane_index, tot_pln_index, sizeof(I1 **), name );

  return (NULL);

  }  /* end fre_ac  */

/***  alc_ec.c  **************************************************************/

/*  Allocate small structures within a larger allocated block.
 *  Can delete entire block but not individual structures.
 *  This saves quite a bit of memory allocation overhead.
 *  Also quite a bit faster than calling alloc() for each small structure.
 *  Based on idea and code by Steve Weller, "The C Users Journal",
 *  April 1990, pp 103 - 107.
 *  Must begin with ALC_ECI for initialization; free with FRE_EC.
 */

typedef struct memblock   /* block of memory for small structure allocation */
  {
  UX bsize;   /* number of bytes in block */
  UX offset;  /* offset to free space */
  struct memblock *prior;  /* pointer to previous block */
  struct memblock *next;   /* pointer to next block */
  }  MEMBLOCK;

void *alc_ec( I1 **block, UX size, I1 *name )
/*  block;  pointer to current memory block.
 *  size;   size (bytes) of structure being allocated.
 *  name;   name of structure being allocated.
 */
  {
  I1 *p;  /* pointer to the structure */
  MEMBLOCK *mb = (void *)*block;

#if( MEMTEST > 0 )
  if( !mb ) error( 3, __FILE__, __LINE__, "block undefined, ", name, "" );
  if( size <= 0 ) error( 3, __FILE__, __LINE__,
    name, " too small to allocate", "" );
#endif

  size = (size+7) & 0xFFF8;   /* multiple of 8 */
  if( mb->offset + size > mb->bsize )
    {
    MEMBLOCK *nb = (MEMBLOCK *)alc_e( mb->bsize, name );
    if( size > mb->bsize - sizeof(MEMBLOCK) ) error( 3, __FILE__,
      __LINE__, "Requested size larger than block for ", name, "" );
    nb->prior = mb;           /* back linked list */
    mb->next = nb;            /* forward linked list */
    nb->next = NULL;
    nb->bsize = mb->bsize;
    nb->offset = sizeof(MEMBLOCK);
    mb = nb;
    *block = (I1 *)(void *)mb;
    }
  p = *block + mb->offset;
  mb->offset += size;

  return ((void *)p);

  }  /*  end of alc_ec  */

/*  Free blocks allocated by ALC_EC.  */

void *fre_ec( I1 *block, I1 *name )
/*  block;  pointer to current memory block.
 *  name;   name of structure being freed.  */
  {
  MEMBLOCK *mb, *nb;

  mb = (MEMBLOCK *)(void *)block;
  if( mb )
    while( mb->next )  /* guarantee mb at end of list */
      mb = mb->next;
  else
    error( 3, __FILE__, __LINE__, "null block pointer, call George", "" );

  while( mb )
    {
    nb = mb->prior;
    fre_e( mb, mb->bsize, "Fre_EC" );
    mb = nb;
    }
  return (NULL);

  }  /*  end of fre_ec  */
/***  alc_eci.c  *************************************************************/

/*  Block initialization for ALC_EC.  */

void *alc_eci( UX size, I1 *name )
/*  size;   size (bytes) of block being allocated.
 *  name;   name of block being allocated.
 */
  {
  I1 *p;  /* pointer to the block */
  MEMBLOCK *mb;

  p = (I1 *)alc_e( size, name );
  mb = (MEMBLOCK *)(void *)p;
  mb->prior = NULL;
  mb->next = NULL;
  mb->bsize = size;
  mb->offset = sizeof(MEMBLOCK);

  return ((void *)p);

  }  /*  end of alc_eci  */

