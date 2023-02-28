/*subfile:  utils.c  *********************************************************/

/*    utility functions   By George*/
#define _CRT_SECURE_NO_DEPRECATE 1
#include <time.h>   /* prototype: clock;  define CLOCKS_PER_SEC */
#include <stdio.h>
#include <string.h> /* prototypes: memset, ... */
#include <stdlib.h> /* prototype: ldiv;  define: ldiv_t */
#include <stdarg.h> /* variable argument list macro definitions */
//#include <limits.h> /* define: SHRT_MAX */
//#include <math.h>   /* prototypes: cos, sin sqrt */
//#include <float.h>  /* define: FLT_MAX */
//#include <ctype.h>  /* prototypes: isalpha, isdigit, toupper, tolower */
#include "Types.h"  /* define U1, I2, etc.  */
#include "Prtyp.h"  /* miscellaneous function prototypes */

#if( _MSC_VER )
# include <malloc.h>/* prototypes: coreleft, _heapchk;
                       define: HEAPEMPTY, etc. */
# include <direct.h>/* prototypes: _makepath, _splitpath */
#endif

#include "Sxtrn.h"

IX _emode=1;   /* error message mode: 0=logFile, 1=DOS, 2=Windows */

/***  errora.c  **************************************************************/

/*  Minimal error message - written to .LOG file.  */

void errora( const I1 *head, I1 *message, I1 *source )
  {
  if( _ulog )
    {
    fputs( head, _ulog );
    fputs( source, _ulog );
    fputs( message, _ulog );
    fflush( _ulog );
    }

  }  /*  end of errora  */

/***  errorb.c  **************************************************************/

/*  Standard error message for console version.  */

void errorb( const I1 *head, I1 *message, I1 *source )
  {
  fputs( head, stderr );
  fputs( message, stderr );
  errora( head, message, source );

  }  /*  end of errorb  */

#if( _MSC_VERXXX )
/***  errorw.c  **************************************************************/

/*  Error message routine for Windows; called from error().  */

void errorw( const I1 *head, I1 *message, I1 *source )
  {
  HWND hwnd=GetForegroundWindow();
  errora( head, message, source );
  strcat( message, source );
  if( head[0] == 'N' )        /* note */
    MessageBox( hwnd, message, head, MB_OK|MB_ICONINFORMATION );
  else if( head[0] == 'W' )   /* warning */
    MessageBox( hwnd, message, head, MB_OK|MB_ICONEXCLAMATION );
  else                        /* error; fatal */
    MessageBox( hwnd, message, head, MB_OK|MB_ICONERROR );

  } /*  end errorw  */
#endif

/***  error.c  ***************************************************************/

/*  Standard error message routine - select errora, errorb or errorw.
 *  _ulog MUST be defined.  */

IX error( IX severity, I1 *file, IX line, ... )
/* severity;  values 0 - 3 defined below
 * file;      file name: __FILE__
 * line;      line number: __LINE__
 * ...;       string variables (up to 80 char total) */
  {
  I1 message[256];
  I1 source[64];
  va_list argp;        /* variable argument list */
  I1 start[]=" ";      /* leading blank in message */
  I1 *msg, *s;
  static IX count=0;   /* count of severe errors */
  static const I1 *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };
  I1 name[32];
  IX n=1;

  if( severity >= 0 )
    {
    if( severity>3 ) severity = 3;
    if( severity==2 && (_doDialog || _doDOSwdw) ) count += 1;
    msg = start;   /* merge message strings */
    s = message;
    va_start( argp, line );
    while( *msg && n<LINELEN )
      {
      while( *msg && n++<LINELEN )
        *s++ = *msg++;
      msg = (I1 *)va_arg( argp, I1 * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    pathsplit( file, NULL, 0, NULL, 0, name, 32, NULL, 0 );
    sprintf( source, "  (file %s, line %d)\n", name, line );

#if( _MSC_VER )
    if( _emode == 2 )
      exit(1);
    else
#endif
    if( _emode == 1 )
      errorb( head[severity], message, source );
    else
      errora( head[severity], message, source );
    if( severity>2 )
      exit(1);
    }
  else if( severity < -1 )   /* clear error count */
    {
    count = 0;
    }

#ifdef _DEBUG
  if( count > 9 )  // 2005/11/10  force stop
      exit(1);
#endif
  if( _emode == 2 && count > 9 )  /* limit interactive errors */
      {
      count = 0;
      }

  return( count );

  }  /*  end error  */


/***  intstr.c  **************************************************************/

/*  Convert an integer to a string of characters.
 *  Can handle short or long integers by conversion to long.
 *  Static variables required to retain results in calling function.
 *  NMAX allows up to NMAX calls to intstr() in one statement.
 *  Replaces nonstandard ITOA & LTOA functions for radix 10.  */

I1 *intstr( I4 i )
  {
#define NMAX 2
  static I1 string[NMAX][12];  /* string long enough for longest integer */
  static IX index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( string[index], "%ld", i );

  return string[index];
#undef NMAX

  }  /* end of intstr */

I1 _dirchr='\0'; // directory charactor - reset in PathSplit
                 // which is used early to create program.log

/*  Set directory separator character based on path. */
/*  MUST BE CALLED prior to pathmerge().             */

I1 *setdirchr( I1 *fullpath )
  {
  I1 *pchar = strpbrk( fullpath, "\\/" );
  _dirchr = *pchar;

  return pchar;
  }

/***  pathmerge.c  ***********************************************************/

/*  Merge full path from its component parts.  */

void pathmerge( I1 *fullpath, IX szfp, I1 *drv, I1 *path, I1 *name, I1 *ext )
  {
  IX n=0; // index to fullpath string
  I1 *c;  // pointer to source string

# ifdef _DEBUG
  if( !fullpath ) {printf("PathMerge: NULL fullpath" );exit(1);}
  if( !_dirchr ) {printf("PathMerge: NULL _dirchr" );exit(1);}
# endif

  if( !fullpath ) return;

  if( drv && *drv )
    for( c=drv; *c; c++ )
      fullpath[n++] = *c;

  if( path && *path )
    {
    for( c=path; *c && n<szfp; c++ )
      fullpath[n++] = *c;
    if( fullpath[n-1] != _dirchr && n<szfp )
      fullpath[n++] = _dirchr;
    }

  if( name && *name )
    for( c=name; *c && n<szfp; c++ )
      fullpath[n++] = *c;

  if( ext && *ext )
    {
    if( ext[0] != '.' && n<szfp )
      fullpath[n++] = '.';
    for( c=ext; *c && n<szfp; c++ )
      fullpath[n++] = *c;
    }

  if( n < szfp )
    fullpath[n] = '\0';
  else
    {
    fullpath[szfp-1] = '\0';
    }

  }  /* end pathmerge */

/***  pathsplit.c  ***********************************************************/

/*  Split full path into its component parts.
 *  In the call pass each part string and sizeof(string).
 *  Use NULL pointers for any parts not wanted.  */

void pathsplit( I1 *fullpath, I1 *drv, IX szd, I1 *path, IX szp,
                I1 *name, IX szn, I1 *ext, IX sze )
  {
  I1 *c, // source string pointer
     *m; // mark position in fullpath
  IX n;  // character count

# ifdef _DEBUG
  if( !fullpath ) {printf("PathSplit: NULL fullpath" ); exit(1);}
# endif

  m = fullpath;
  n = 0;
  if( fullpath[1] == ':' )
    {
    if( drv )
      {
      if( szd < 3 )
        printf("PathSplit: file drive overrun" );
      else
        {
        drv[n++] = fullpath[0];  // copy drive character
        drv[n++] = ':';
       }
      }
    m += 2;  // m = start of path in fullpath
    }
  if( drv )
    drv[n] =  '\0';

  n = 0;
  c = strpbrk( m, "\\/" );  // find directory character
  if( c )
    {
    I1 *pd;   // position of last directory charactor
    _dirchr = *c;  // directory charactor: \ or /
    pd = strrchr( m, _dirchr );  // find last _dirchr
    if( path )
      {
      for( c=m; c<=pd && n<szp; c++ )
        path[n++] = *c;
      if( n == szp )
        { printf("PathSplit: file path overrun" ); n--; }
      }
    m = pd + 1;  // m = start of name in fullpath
    }
  if( path )
    path[n] = '\0';  // terminate path

  n = 0;
  c = m;  // save start of name in fullpath
  m = strrchr( m, '.' );  // find last period in name
  if( name )
    {
    if( m )  // m = last period in fullpath
      while( c<m && n<szn )
        name[n++] = *c++;
    else     // no period in/after name
      while( *c && n<szn )
        name[n++] = *c++;
    if( n == szn )
      { printf("PathSplit: file name overrun"); n--; }
    name[n] = '\0';  // terminate name
    }

  if( ext )
    {
    n = 0;
    if( m )  // m = last period in fullpath
      {
      for( n=0,c=m; *c && n<sze; c++ )
        ext[n++] = *c;
      if( n == sze )
        { printf("PathSplit: file extension overrun" ); n--; }
      }
    ext[n] = '\0';  // terminate extension
    }

  }  /* end pathsplit */

/***  ivdump.c  **************************************************************/

/*  Dump IX (integer) vector to _ulog.  */

void ivdump( IX *iv, IX nn, I1 *title )
  {
  IX j;

  fprintf( _ulog, "%s (%d):\n", title, nn );
  for( j=1; j<=nn; j++,iv++ )
    {
    fprintf( _ulog, " %6d", *iv );
    if( j%10 == 0 )
      fprintf( _ulog, "\n" );
    }
  if( j%10 != 1 )
    fprintf( _ulog, "\n" );

  }  /* end ivdump */

/***  rvdump.c  **************************************************************/

/*  Dump R4 (float) vector to _ulog.  */

void rvdump( R4 *rv, IX nn, I1 *title )
  {
  IX j;

  fprintf( _ulog, "%s (%d):\n", title, nn );
  for( j=1; j<=nn; j++,rv++ )
    {
    fprintf( _ulog, " %6g", *rv );
    if( j%10 == 0 )
      fprintf( _ulog, "\n" );
    }
  if( j%10 != 1 )
    fprintf( _ulog, "\n" );

  }  /* end rvdump */

/***  dvdump.c  **************************************************************/

/*  Dump R8 (double) vector to _ulog.  */

void dvdump( R8 *dv, IX nn, I1 *title )
  {
  IX j;

  fprintf( _ulog, "%s (%d):\n", title, nn );
  for( j=1; j<=nn; j++,dv++ )
    {
    fprintf( _ulog, " %6g", *dv );
    if( j%10 == 0 )
      fprintf( _ulog, "\n" );
    }
  if( j%10 != 1 )
    fprintf( _ulog, "\n" );

  }  /* end dvdump */

/***  cputime.c  *************************************************************/

/*  Determine elapsed time.  Call once to determine t1;
    call later to get elapsed time. */

R4 cputime( R4 t1 )
  {
  R4 t2;

  t2 = (R4)clock() / (R4)CLOCKS_PER_SEC;
  return (t2-t1);

  }  /* end cputime */

