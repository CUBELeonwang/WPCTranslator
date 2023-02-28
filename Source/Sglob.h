/*subfile:  sglob.h  *********************************************************/

/*  SGLOB:  global variables associated with simulation functions  By George*/
#ifdef linux
#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#endif

I1 _string[LINELEN];  /* buffer for a character string */
I1 _stringx[LINELEN]; /* buffer for a help/error strings */
FILE *_ulog=NULL;     /* output file */
I1 _file_name[_MAX_PATH]; /* temporary file name */
I1 _cdrive[_MAX_DRIVE];   /* drive letter for program ContamX.EXE */
I1 _cdir[_MAX_DIR];       /* directory path for program ContamX.EXE */

I1 _wpcdrive[_MAX_DRIVE];   /* drive letter for file <project>.PRJ */
I1 _wpcdir[_MAX_DIR];       /* directory path for file <project>.PRJ */
I1 _wpcPRJ[_MAX_PATH];    /* full path for <project>.PRJ */
I1 _lb2PRJ[_MAX_PATH];
I1 _wpc_name[_MAX_PATH]; /* temporary file name */
I1 _wpcpdrive[_MAX_DRIVE];   /* drive letter for file <project>.PRJ */
I1 _wpcpdir[_MAX_DIR];       /* directory path for file <project>.PRJ */

I1 _plddrive[_MAX_DRIVE];   /* drive letter for file <project>.PRJ */
I1 _plddir[_MAX_DIR];       /* directory path for file <project>.PRJ */
I1 _pldPRJ[_MAX_PATH];    /* full path for <project>.PRJ */
I1 _pld_name[_MAX_PATH]; /* temporary file name */
IX _doDialog=0;
IX _doDOSwdw=0;
//global variables for WPP calculations
typedef struct cpl_wpp      /* structure for WPP calculation */
{
	R4 Vmet; /*reference local wind velocity at Hmet */
	R4 Hmet;
	R4 Dmet;
	R4 amet;
	R4 Dlocal;
	R4 alocal;
	R4 InitA; /*initial calculation wind angle: 0-360, <LastA*/
	R4 LastA; /*last calculation wind angle: 0-360*/
	R4 StepA; /*calculaiton step of wind angle*/
	R4 CurtA; /*current angle*/
} CPL_WPP;

CPL_WPP ABWIND; //AmBient wind

IX COUPLEWPP=1;
