/*subfile:  prtyp.h  *********************************************************/

/*  CONTAMX function prototypes.  By George*/

     /* functions in utils.c */
I1 *intstr( I4 i );
IX error( IX severity, I1 *file, IX line, ... );
void pathmerge( I1 *fullpath, IX szfp, I1 *drv, I1 *path, I1 *name, I1 *ext );
void pathsplit( I1 *fullpath, I1 *drv, IX szd, I1 *path, IX szp,
                I1 *name, IX szn, I1 *ext, IX sze );
void ivdump( IX *iv, IX nn, I1 *title );
void rvdump( R4 *rv, IX nn, I1 *title );
void dvdump( R8 *rv, IX nn, I1 *title );

     /* functions in heap.c */
void memrem( I1 *msg );
void *alc_e( UX length, I1 *name );
IX chk_e( void *pm, UX length, I1 *name );
void *fre_e( void *pm, UX length, I1 *name );
void *alc_v( IX, IX, IX, I1 * );
void chk_v( void *, IX, IX, IX, I1 * );
void *fre_v( void *, IX, IX, IX, I1 * );
void *alc_mc( IX, IX, IX, IX, IX, I1 * );
void chk_mc( void *, IX, IX, IX, IX, IX, I1 * );
void *fre_mc( void *, IX, IX, IX, IX, IX, I1 * );
void *alc_ac( IX, IX, IX, IX, IX, IX, IX, I1 * );
void chk_ac( void *, IX, IX, IX, IX, IX, IX, IX, I1 * );
void *fre_ac( void *, IX, IX, IX, IX, IX, IX, IX, I1 * );
void *alc_ec( I1 **block, UX size, I1 *name );
void *fre_ec( I1 *block, I1 *name );
void *alc_eci( UX size, I1 *name );
void Pause();
R4 cputime( R4 t1 );
void ewc_cfd0();
void cfd0_wdb();
void cfd0_cdb();
void search(I1 dbtemp);
void wpc_cfd0();
void wpc_fds();
void fds_cdb();
void fds_wdb();
void checkbt(IX bt1, IX bt2);
IX checkeof();
void print_cfd0();
void print_fds();
I1 *setdirchr( I1 *fullpath );




