#define _CRT_SECURE_NO_DEPRECATE 1
#define _FILE_OFFSET_BITS 64 //for I/O large files > 2G
#if(  _MSC_VER )
	# include <direct.h>
#else
	#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "math.h"
/*  global variables  */
#include "Sglob.h"
#include "Sxtrn.h"
#include "Prtyp.h"

typedef struct fds_time
{
	struct fds_time *next;
	IX dbindex;
	R8 curtime;
} FDS_TIME;
typedef struct fds_path
{
	struct fds_path *next;
	R8 cp;
} FDS_PATH;
typedef union {
  char ch[4];
  int  i;
} UNION;

R8 xref,yref,zref,*xf,*yf,*zf,*x,*y,*z,***p,tlr,pamb,dens,*wp,ppp;
R8 xref1,yref1,zref1;
R8 xmax,ymax,zmax,xx,yy,zz,**cp,**windz,wallz,xmin,ymin,zmin,TLC;
IX CSTEADY; // CSTEADY = 0, time VS species; = 1, wind angle VS species; =2, EWC
IX NOC,DBINDEX;
R8 TIMESTEP,TIMEMAX,TIMEMIN,CFDTIME;
I1 dtemp = ' ', _dbtype = ' ';
I1 nxtw[1024];
FILE *pld,*ewc,*wpc,*lb2;
I4 np,nc, *lbl, **ibl;
R8 **xbl, *sbl;
I4 nmax[3],nbl, bloc[7];
I4 ***icell,*id, cell,stepa = 0;
R4 curta;
R8 curta1;
FDS_TIME  *_fdstime0;
UNION header;
FDS_PATH **_fdspath;

IX main(IX argc, I1 **argv)
{
  I4 i, pp;
  I1 dump[1024],temp=' ';
  I1 workDir[_MAX_PATH];    /* working directory - prj file */

  /* Get working and Set log file directories */
#if( _MSC_VER )
  _getcwd( workDir, LINELEN );
#elif( __GNUC__ )
  getcwd( workDir, LINELEN );
#else
  getcwd( workDir, LINELEN );
#endif

  /* Set directory separator character */
if( !setdirchr( workDir ) )
#if( __GNUC__ )
    setdirchr( "/" );
#else
    setdirchr( "\\" );
#endif

  //tolerance of distance
  TLC=0.01;
  CSTEADY=0;
  if( argc < 2 )
    {
	fprintf( stderr, "Enter PLD file name:  " );
	fgets( _string, LINELEN, stdin );
	if( _string[strlen(_string)-1] == '\n' )
	  _string[strlen(_string)-1] = '\0';
    }
  else
	strcpy( _string, argv[1] );

  pathsplit( _string, _plddrive, sizeof(_plddrive), _plddir, sizeof(_plddir),
	             _pld_name, sizeof(_pld_name), NULL, 0 );

  pathmerge(_pldPRJ, sizeof(_pldPRJ), _plddrive, _plddir, _pld_name, ".pld" );

  pld=fopen(_pldPRJ,"r");
 if(!pld)
  {
	printf("No PLD file has been found in the directory specified,exit\n");
	Pause();
    exit(1);
  }
  if( argc < 2 ) printf("\nReading PLD file: %s\n",_pldPRJ);

 //begin read .pld information
  i=0;
 while((temp=fgetc(pld))!='!')
  {
	  nxtw[i]=temp;
	  i++;
  }
  nxtw[i]=0;

	for(i=1;i<=4;i++) fgets(dump,sizeof(dump),pld);
	fscanf(pld,"%lf %lf %lf",&xref1, &yref1,&zref1);
    for(i=1;i<=6;i++) fgets(dump,sizeof(dump),pld);
	fscanf(pld,"%ld %ld",&pp,&nc);

	//is it wind pressure required?
	//if(pp==0)
	//{
	//  printf("To compute wind pressure flag is off,exit\n");
	//  Pause();
	//  exit(1);
	//}

	for(i=1;i<=nc+2;i++) fgets(dump,sizeof(dump),pld);
	fscanf(pld,"%ld %lf",&np,&tlr);
	TLC = tlr;

	x=alc_v(1,np,sizeof(R8),"x");
	y=alc_v(1,np,sizeof(R8),"y");
	z=alc_v(1,np,sizeof(R8),"z");
	id=alc_v(1,np,sizeof(I4),"id");
	wp=alc_v(1,np,sizeof(R8),"wp");

	for(i=1;i<=2;i++) fgets(dump,sizeof(dump),pld);
	for(i=1;i<=np;i++)
	{
		fscanf(pld,"%ld %lf %lf %lf\n",&id[i],&x[i],&y[i],&z[i]);
	}
//end reading pld file

	printf("\nCreate wpc link from CFD0/FDS or ewc link from CFD0? CFD0=1; EWC=2; FDS=3:");
	//while(_dbtype!='1'&&_dbtype!='2'&&_dbtype!='3') scanf("%c",&_dbtype);
	_dbtype = '3';

	printf("\nCreate wind pressure link file=1, or ambient contaminant link file=2:");
	//while(dtemp!='1'&&dtemp!='2') scanf("%c",&dtemp);
	dtemp = '2';

	//read ewc files
	switch(_dbtype)
	{ case '1': wpc_cfd0();print_cfd0();break;
	case '2': ewc_cfd0(); print_cfd0();break;
	case '3': wpc_fds();print_fds();break;
	}
	return 1;
}

void wpc_cfd0()
{
	switch(dtemp)
	{case '1':
		pathmerge(nxtw,sizeof(nxtw), _plddrive,_plddir, _pld_name,".wdb");
		pathmerge(_lb2PRJ,sizeof(_lb2PRJ),_plddrive,_plddir,_pld_name,".lb2");
		break;
	case '2':
		pathmerge(nxtw,sizeof(nxtw),_plddrive,_plddir,_pld_name,".cdb");
		pathmerge(_lb2PRJ,sizeof(_lb2PRJ),_plddrive,_plddir,_pld_name,".cfdlb2");
		break;
	}

	ewc=fopen( nxtw,"rb");
	lb2=fopen(_lb2PRJ,"w");

	if(!ewc)
	{
	  printf("Couldn't find the ewc file specified in pld file,exit\n");
	  Pause();
	  exit(1);
	}

	switch(dtemp)
	{case '1':
		cfd0_wdb();
		break;
	case '2':
		cfd0_cdb();
		break;
	}

	printf("\n");
	return;
}

void cfd0_wdb()
{
	I4 i1,i,j,k, a1;
	I1 cc;

    printf("Reading WDB file: %s\n",nxtw);
	//read wpp if any
		//if COUPLEWPP=1;//wpp
	if(COUPLEWPP)
	{
			fread(&xref,sizeof(xref),1,ewc);
			fread(&yref,sizeof(yref),1,ewc);
			fread(&zref,sizeof(zref),1,ewc);
			printf("The coordinates of the reference point: X=%lf; Y=%lf; Z=%lf\n", xref, yref, zref);
			Pause();
			for(i=1;i<=np;i++)
			{
				x[i]=x[i]+xref;
				y[i]=y[i]+yref;
				z[i]=z[i]+zref;
			}

			fread(&ABWIND.Hmet,sizeof(ABWIND.Hmet),1,ewc);
			fread(&ABWIND.Vmet,sizeof(ABWIND.Vmet),1,ewc);
			fread(&ABWIND.Dmet,sizeof(ABWIND.Dmet),1,ewc);
			fread(&ABWIND.amet,sizeof(ABWIND.amet),1,ewc);
			fread(&ABWIND.Dlocal,sizeof(ABWIND.Dlocal),1,ewc);
			fread(&ABWIND.alocal,sizeof(ABWIND.alocal),1,ewc);
			fread(&ABWIND.InitA,sizeof(ABWIND.InitA),1,ewc);
			fread(&ABWIND.LastA,sizeof(ABWIND.LastA),1,ewc);
			fread(&ABWIND.StepA,sizeof(ABWIND.StepA),1,ewc);
			fread(&cc,sizeof(cc),1,ewc);

			a1=0;
			for(ABWIND.CurtA=ABWIND.InitA;ABWIND.CurtA<=ABWIND.LastA;ABWIND.CurtA+=ABWIND.StepA) a1++;
			cp=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"cp");
			windz=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"windz");
			ABWIND.CurtA=ABWIND.InitA;
			//calculate wind velocity profile
			/*R8 Uref,Hlocal,Dlocal,alocal;
			Uref=ABWIND.Vmet*pow((ABWIND.Dmet/ABWIND.Hmet),ABWIND.amet);
			Dlocal=ABWIND.Dlocal;
			alocal=ABWIND.alocal;
			for(k=1;k<=NK;k++)
			{
				Hlocal=Z[k];
				WVPL[k]=Uref*pow((Hlocal/Dlocal),alocal);
				printf("%lf,%lf\n", Hlocal,WVPL[k]);
			}*/
		}

	if(COUPLEWPP)
	{

		do
		{
			if(!fread(&cc,sizeof(cc),1,ewc))
			{
				exit(1);
			}
		    fread(&curta,sizeof(curta),1,ewc);
			while(curta!=ABWIND.CurtA) ABWIND.CurtA+=ABWIND.StepA;
			stepa++;
		//begin reading ewc file
		fread(&pamb,sizeof(R8),1,ewc);
		fread(&dens,sizeof(R8),1,ewc);
		fread(&nmax,sizeof(I4),3,ewc);
		fread(&bloc,sizeof(I4),7,ewc);
		fread(&nbl,sizeof(I4),1,ewc);
		if(stepa==1) ibl=alc_mc(1,nbl,0,6,sizeof(I4),"ibl");
		for(i1=1;i1<=nbl;i1++)
		{
			lbl=ibl[i1];
			fread(lbl,sizeof(I4),7,ewc);
		}
		if(stepa==1)
		{
			xf=alc_v(bloc[1]-2,bloc[2]+1,sizeof(R8),"xf");
			yf=alc_v(bloc[3]-2,bloc[4]+1,sizeof(R8),"yf");
			zf=alc_v(1,bloc[6]+1,sizeof(R8),"zf");
		}
		for(i=(bloc[1]-2);i<=(bloc[2]+1);i++)
		{
			fread(&xx,sizeof(R8),1,ewc);
			xf[i]=xx;
		}
		for(j=(bloc[3]-2);j<=(bloc[4]+1);j++)
		{
			fread(&yy,sizeof(R8),1,ewc);
			yf[j]=yy;
		}
		for(k=1;k<=(bloc[6]+1);k++)
		{
			fread(&zz,sizeof(R8),1,ewc);
			zf[k]=zz;
		}
		if(stepa==1) p=alc_ac(bloc[1]-1,bloc[2]+1,bloc[3]-1,bloc[4]+1,2,bloc[6]+1,sizeof(R8),"p");
		if(stepa==1) icell=alc_ac(bloc[1]-1,bloc[2]+1,bloc[3]-1,bloc[4]+1,2,bloc[6]+1,sizeof(I4),"icell");
		for(i=(bloc[1]-1);i<=(bloc[2]+1);i++)
		{
			for(j=(bloc[3]-1);j<=(bloc[4]+1);j++)
			{
				for(k=2;k<=(bloc[6]+1);k++)
				{
					fread(&ppp,sizeof(R8),1,ewc);
					p[i][j][k]=ppp;
					fread(&cell,sizeof(I4),1,ewc);
					icell[i][j][k]=cell;
				}
			}
		}
		fread(&cc,sizeof(cc),1,ewc);
		//search
		if(stepa == 1) printf("\nSearching wind pressure/species level in EWC file\n");
		search(dtemp);

		ABWIND.CurtA+=ABWIND.StepA;
} while (ABWIND.CurtA<=ABWIND.LastA);
}
	//end reading ewc file
	return;
}


void cfd0_cdb()
{
	I4 i1,i,j,k, a1;
	I1 cc;
	IX ddtemp=0;
	//read cdb file:

       printf("\nReading CDB file: %s\n",nxtw);

		fread(&xref,sizeof(xref),1,ewc);
		fread(&yref,sizeof(yref),1,ewc);
		fread(&zref,sizeof(zref),1,ewc);

		printf("\nThe coordinates of the reference point: X=%lf; Y=%lf; Z=%lf\n", xref, yref, zref);
		Pause();

		for(i=1;i<=np;i++)
		{
			x[i]=x[i]+xref;
			y[i]=y[i]+yref;
			z[i]=z[i]+zref;
		}

		fread(&CSTEADY,sizeof(CSTEADY),1,ewc);

	if(CSTEADY==1) //wind angle VS contaminant distributions
	{
		if(COUPLEWPP)
		{
			fread(&ABWIND.Hmet,sizeof(ABWIND.Hmet),1,ewc);
			fread(&ABWIND.Vmet,sizeof(ABWIND.Vmet),1,ewc);
			fread(&ABWIND.Dmet,sizeof(ABWIND.Dmet),1,ewc);
			fread(&ABWIND.amet,sizeof(ABWIND.amet),1,ewc);
			fread(&ABWIND.Dlocal,sizeof(ABWIND.Dlocal),1,ewc);
			fread(&ABWIND.alocal,sizeof(ABWIND.alocal),1,ewc);
			fread(&ABWIND.InitA,sizeof(ABWIND.InitA),1,ewc);
			fread(&ABWIND.LastA,sizeof(ABWIND.LastA),1,ewc);
			fread(&ABWIND.StepA,sizeof(ABWIND.StepA),1,ewc);
			fread(&NOC,sizeof(NOC),1,ewc);
			fread(&cc,sizeof(cc),1,ewc);

			a1=0;
			for(ABWIND.CurtA=ABWIND.InitA;ABWIND.CurtA<=ABWIND.LastA;ABWIND.CurtA+=ABWIND.StepA) a1++;
			cp=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"cp");
			windz=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"windz");
			ABWIND.CurtA=ABWIND.InitA;
		}
	}
	else
	{
		if(COUPLEWPP)
		{
			fread(&ABWIND.Hmet,sizeof(ABWIND.Hmet),1,ewc);
			fread(&ABWIND.Vmet,sizeof(ABWIND.Vmet),1,ewc);
			fread(&ABWIND.Dmet,sizeof(ABWIND.Dmet),1,ewc);
			fread(&ABWIND.amet,sizeof(ABWIND.amet),1,ewc);
			fread(&ABWIND.Dlocal,sizeof(ABWIND.Dlocal),1,ewc);
			fread(&ABWIND.alocal,sizeof(ABWIND.alocal),1,ewc);
			fread(&ABWIND.InitA,sizeof(ABWIND.InitA),1,ewc);
			fread(&ABWIND.LastA,sizeof(ABWIND.LastA),1,ewc);
			fread(&ABWIND.StepA,sizeof(ABWIND.StepA),1,ewc);

			fread(&NOC,sizeof(NOC),1,ewc);
			fread(&CFDTIME,sizeof(CFDTIME),1,ewc);
			fread(&TIMESTEP,sizeof(TIMESTEP),1,ewc);
			fread(&TIMEMAX,sizeof(TIMEMAX),1,ewc);

			fread(&cc,sizeof(cc),1,ewc);
			a1=0;
			for(CFDTIME=TIMESTEP;CFDTIME<=TIMEMAX;CFDTIME+=TIMESTEP) a1++;

			cp=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"cp");
			windz=(R8 **)alc_mc(1,a1,1,np,sizeof(R8),"windz");
			CFDTIME=TIMESTEP;
			ABWIND.InitA=(R4)CFDTIME;ABWIND.LastA=(R4)TIMEMAX;ABWIND.StepA=(R4)TIMESTEP;
		}
	}

	printf("\nThe number of species locations is: %d\n",NOC);
	printf("\nChoose index of species to generate WPPC file (1~%d):",NOC);
	while((ddtemp!=1&&ddtemp!=2&&ddtemp!=3&&ddtemp!=4&&ddtemp!=5)||ddtemp>NOC) scanf("%d",&ddtemp);


	if(COUPLEWPP)
	{
		//R8 temp1;
		do
		{
			if(!fread(&cc,sizeof(cc),1,ewc))
			{
				exit(1);
			}

		if(CSTEADY==1) fread(&curta,sizeof(R4),1,ewc);
		else if(CSTEADY==0)
		{fread(&curta1,sizeof(R8),1,ewc);
		curta=(R4)curta1;}
		while(curta!=ABWIND.CurtA) ABWIND.CurtA+=ABWIND.StepA;

		stepa++;

		//begin reading ewc file
		fread(&pamb,sizeof(R8),1,ewc);
		fread(&dens,sizeof(R8),1,ewc);
		fread(&nmax,sizeof(I4),3,ewc);
		fread(&bloc,sizeof(I4),7,ewc);
		fread(&nbl,sizeof(I4),1,ewc);

		if(stepa==1) ibl=alc_mc(1,nbl,0,6,sizeof(I4),"ibl");
		for(i1=1;i1<=nbl;i1++)
		{
			lbl=ibl[i1];
			fread(lbl,sizeof(I4),7,ewc);
		}

		if(stepa==1)
		{
			xf=alc_v(bloc[1]-2,bloc[2]+1,sizeof(R8),"xf");
			yf=alc_v(bloc[3]-2,bloc[4]+1,sizeof(R8),"yf");
			zf=alc_v(1,bloc[6]+1,sizeof(R8),"zf");
		}

		for(i=(bloc[1]-2);i<=(bloc[2]+1);i++)
		{
			fread(&xx,sizeof(R8),1,ewc);
			xf[i]=xx;
		}
		for(j=(bloc[3]-2);j<=(bloc[4]+1);j++)
		{
			fread(&yy,sizeof(R8),1,ewc);
			yf[j]=yy;

		}
		for(k=1;k<=(bloc[6]+1);k++)
		{
			fread(&zz,sizeof(R8),1,ewc);
			zf[k]=zz;
		}


		if(stepa==1) p=alc_ac(bloc[1]-1,bloc[2]+1,bloc[3]-1,bloc[4]+1,2,bloc[6]+1,sizeof(R8),"p");
		if(stepa==1) icell=alc_ac(bloc[1]-1,bloc[2]+1,bloc[3]-1,bloc[4]+1,2,bloc[6]+1,sizeof(I4),"icell");

		for(i=(bloc[1]-1);i<=(bloc[2]+1);i++)
		{
			for(j=(bloc[3]-1);j<=(bloc[4]+1);j++)
			{
				for(k=2;k<=(bloc[6]+1);k++)
				{
					for(i1=1;i1<=ddtemp;i1++) fread(&ppp,sizeof(R8),1,ewc);
					p[i][j][k]=ppp;
					for(i1=1;i1<=NOC-ddtemp;i1++) fread(&ppp,sizeof(R8),1,ewc);
					fread(&cell,sizeof(I4),1,ewc);
					icell[i][j][k]=cell;
				}
			}
		}

		fread(&cc,sizeof(cc),1,ewc);

		//search
		if(stepa == 1) printf("\nSearching wind pressure/species level in EWC file\n");
		search(dtemp);

	ABWIND.CurtA+=ABWIND.StepA;

} while (ABWIND.CurtA<=ABWIND.LastA);

}
	//end reading ewc file
return;
}

void ewc_cfd0()
{
	I4 i;
	I1 temp;

	if(!COUPLEWPP)
	{
			//create wpc file
			//wpc file
			pathsplit(nxtw,_wpcpdrive,sizeof(_wpcpdrive),_wpcpdir,sizeof(_wpcpdir),_wpc_name,sizeof(_wpc_name),NULL, 0);
			pathmerge(_wpcPRJ,sizeof(_wpcPRJ),_wpcpdrive,_wpcpdir,_wpc_name,".WPC");
			wpc=fopen(_wpcPRJ,"w");

            printf("Creating WPC file: %s\n",_wpcPRJ);

			for(i=1;i<=np;i++)
			{
				x[i]=x[i]-xref;
				y[i]=y[i]-yref;
				z[i]=z[i]-zref;
			}

			fprintf(wpc,"WPCfile ContamW 2.1\nWPC description\n");
			fprintf(wpc,"%ld !Flowpaths\n",np);
			fprintf(wpc,"%d !Contaminants\n",0);
			fprintf(wpc,"%d !Use pressures flag\n",1);
			fprintf(wpc,"%d !Time step\n",0);
			fprintf(wpc,"01/01 !Start date\n");
			fprintf(wpc,"01/01 !End date\n");
			for(i=1;i<=np;i++)
			{
				fprintf(wpc,"%ld  %f %f %f %d\n",i,x[i],y[i],z[i],0);
			}
			fprintf(wpc,"01/01 00:00:00 %f %lf\n",pamb,dens);
			for(i=1;i<=np;i++) fprintf(wpc,"%f ",pamb+wp[i]);
			fprintf(wpc,"\n01/01 24:00:00 %f %lf\n",pamb,dens);
			for(i=1;i<=np;i++) fprintf(wpc,"%f ",pamb+wp[i]);

			temp=' ';

			fclose(wpc);
			fclose(pld);
			fclose(ewc);
			//if( argc < 2 )
			{
				printf("WPC file has been created\n\nPress enter to exit\n");
				Pause();
			}
		}
return;
}

void search(I1 dbtemp)
{
    I4 i, i1, plane=0, kk, ii, jj, imin,jmin,kmin;
   R8 bxmin, bxmax, bymin, bymax, bzmin, bzmax;

	//begin search for wind pressure
		for(i=1;i<=np;i++)
		{
			plane = 0;
			for(i1=1;i1<=nbl;i1++)
			{
				plane=0;
				bxmin = xbl[i1][1];
				bxmax = xbl[i1][2];
				bymin = xbl[i1][3];
				bymax = xbl[i1][4];
				bzmin = xbl[i1][5];
				bzmax = xbl[i1][6];

				//if fds
				imin = ibl[i1][1];
				jmin = ibl[i1][3];
				kmin = ibl[i1][5];

				//if not fds
				if(_dbtype != '3')
				{
					imin--;
					jmin--;
					kmin--;
				}

				xmin=xf[imin];
				ymin=yf[jmin];
				zmin=zf[kmin];

				xmax=xf[ibl[i1][2]];
				ymax=yf[ibl[i1][4]];
				zmax=zf[ibl[i1][6]];

				//fprintf(lb2,"%d %lf %lf %lf %lf %lf %lf\n",i1,xmin, xmax, ymin, ymax, zmin, zmax);

				 //find the positions of the openings
				//find which plane the opening in: plane: N-1;S-2;W-3;E-4;T-5
				 if(fabs(y[i]-bymax)<=TLC) plane=1;
				 else if(fabs(y[i]-bymin)<=TLC) plane=2;
				 else if(fabs(x[i]-bxmin)<=TLC) plane=3;
				 else if(fabs(x[i]-bxmax)<=TLC) plane=4;
				 else if(fabs(z[i]-bzmax)<=TLC) plane=5;
				 else if(fabs(z[i]-bzmin)<=TLC) plane=6;
				/* if(fabs(y[i]-ymax)<=TLC) plane=1;
				 else if(fabs(y[i]-ymin)<=TLC) plane=2;
				 else if(fabs(x[i]-xmin)<=TLC) plane=3;
				 else if(fabs(x[i]-xmax)<=TLC) plane=4;
				 else if(fabs(z[i]-zmax)<=TLC) plane=5;
				 else if(fabs(z[i]-zmin)<=TLC) plane=6;*/

				 if(plane!=0)
				 {
					 switch(plane)
					 {
					 case 1: if(x[i]<xmin||x[i]>xmax||z[i]>zmax||z[i]<zmin) {plane=0;continue;} else break;
					 case 2: if(x[i]<xmin||x[i]>xmax||z[i]>zmax||z[i]<zmin) {plane=0;continue;} else break;
					 case 3: if(y[i]<ymin||y[i]>ymax||z[i]>zmax||z[i]<zmin) {plane=0;continue;} else break;
					 case 4: if(y[i]<ymin||y[i]>ymax||z[i]>zmax||z[i]<zmin) {plane=0;continue;} else break;
					 case 5: if(x[i]<xmin||x[i]>xmax||y[i]<ymin||y[i]>ymax) {plane=0;continue;} else break;
					 case 6: if(x[i]<xmin||x[i]>xmax||y[i]<ymin||y[i]>ymax) {plane=0;continue;} else break;
					 case 7: break;
					 }
				 }
				 else continue;


				 if(plane!=0)
				 {
					 //find the pressures
					 ii=ibl[i1][1];
					 jj=ibl[i1][3];
					 kk=ibl[i1][5];
					 switch(plane)
					 {
					 case 1:
						 {
							 while(xf[ii]<x[i]) ii++;
							 while(zf[kk]<z[i]) kk++;
							 jj=ibl[i1][4]+1;
							 wallz=0.0;
							 break;
						 }
					 case 2:
						 {
							 while(xf[ii]<x[i]) ii++;
							 while(zf[kk]<z[i]) kk++;
							 jj=jmin;
							 wallz=180.0;
							 break;
						 }
					 case 3:
						 {
							 while(yf[jj]<y[i]) jj++;
							 while(zf[kk]<z[i]) kk++;
							 ii=imin;
							 wallz=270.0;
							 break;
						 }
					 case 4:
						 {
							 while(yf[jj]<y[i]) jj++;
							 while(zf[kk]<z[i]) kk++;
							 ii=ibl[i1][2]+1;
							 wallz=90.0;
							 break;
						 }
					 case 5:
						 {
							 while(yf[jj]<y[i]) jj++;
							 while(xf[ii]<x[i]) ii++;
							 kk=ibl[i1][6]+1;
							 wallz=0.0;
							 break;
						 }
					case 6:
						 {
							 while(yf[jj]<y[i]) jj++;
							 while(xf[ii]<x[i]) ii++;
							 kk=kmin;
							 wallz=0.0;
							 break;
						 }
					 }
					 wp[i]=p[ii][jj][kk];
					 cp[stepa][i]=wp[i];

					 if(CSTEADY!=0||dbtemp == '1')
					 {
					  windz[stepa][i]=curta-wallz;
					 if(windz[stepa][i]<0.0) windz[stepa][i]+=360.0;

					 //temp1=0.0;
					 //if(curta==0.0)	temp1=cp[stepa][i];
					 //if(windz[stepa][i]==0.0||windz[stepa][i]==360.0)	temp1=cp[stepa][i];

					 if(curta==360.0) windz[stepa][i]=360.0;
					 }
					 else windz[stepa][i]=curta;

					//	 cp[stepa][i]=temp1;
					 //}

				 }

				 break;
			 }
			 //after going through all blocks plane=0 still set wallz=0 and get windz
			 if(plane==0&&i1==nbl+1)
			 {
	 			printf("\nPath %ld is not on a building surface, exit\n",i);
				//Pause();

				if(CSTEADY!=0)
				{
					windz[stepa][i]=curta;
					if(windz[stepa][i]<0.0) windz[stepa][i]+=360.0;

					if(curta==360.0) windz[stepa][i]=360.0;
				}
				else windz[stepa][i]=curta;
			}
		}
	return;
}

void Pause()
{
    I1 temp=' ';
  fprintf( stderr, "\a" );  /* sound */
  fprintf( stderr, "\nPress ENTER to continue..." );
    //clear key
    temp = getchar();
  do	  temp = getchar();
  while(temp!='\n');
  return;
}

void wpc_fds()
{
	switch(dtemp)
	{case '1':
		pathmerge(nxtw,sizeof(nxtw), _plddrive,_plddir, _pld_name,".wdb");
		pathmerge(_lb2PRJ,sizeof(_lb2PRJ),_plddrive,_plddir,_pld_name,".lb2");
		break;
	case '2':
		pathmerge(nxtw,sizeof(nxtw),_plddrive,_plddir,_pld_name,".cdb");
		pathmerge(_lb2PRJ,sizeof(_lb2PRJ),_plddrive,_plddir,_pld_name,".cfdlb2");
		break;
	}

	ewc=fopen( nxtw,"rb");
	lb2=fopen(_lb2PRJ,"w");

	if(!ewc)
	{
	  printf("Can not create wdb/cdb file from wpc_fds(),exit\n");
	  Pause();
	  exit(1);
	}
	//allocate memory for fds paths
	_fdspath=alc_v(1,np,sizeof(FDS_PATH),"fds_path");

	switch(dtemp)
	{case '1':
		fds_wdb();
		break;
	case '2':
		fds_cdb();
		break;
	}
	printf("\n");
	return;
}

void fds_cdb()
{
	IX tempint = 0, tempd = 0;
	I4 i1,i2,j1,j2,k1,k2,ii,i,j,k, bytes = 0;
	I1 *_block, *_block1;
	IX ddtemp=0;
	FDS_TIME *temptime, *temptime0;
	FDS_PATH *temppath, **temppath1;

	temppath1=alc_v(1,np,sizeof(FDS_PATH),"temppath1");
	//read cdb file:
       printf("\nReading CDB file created by FDS: %s\n",nxtw);

       //read header
		fread(&tempint, sizeof(int), 1, ewc);
		fread(&xref,sizeof(xref),1,ewc);
		fread(&yref,sizeof(yref),1,ewc);
		fread(&zref,sizeof(zref),1,ewc);

		bytes += 3*sizeof(xref);
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		printf("\nThe coordinates of the reference point: X=%lf; Y=%lf; Z=%lf\n", xref1, yref1, zref1);
		//Pause();

		for(i=1;i<=np;i++)
		{
			x[i]=x[i]+xref1;
			y[i]=y[i]+yref1;
			z[i]=z[i]+zref1;
		}

		CSTEADY = 0;

		fread(&tempint, sizeof(int), 1, ewc);
		bytes = 0;
		bytes += (tempd = sizeof(NOC));
		fread(&NOC,tempd,1,ewc);
		bytes += (tempd = sizeof(TIMESTEP));
		fread(&TIMESTEP,tempd,1,ewc);
		bytes += (tempd = sizeof(TIMEMIN));
		fread(&TIMEMIN,tempd,1,ewc);
		bytes += (tempd = sizeof(TIMEMAX));
		fread(&TIMEMAX,tempd,1,ewc);
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		printf("\nThe number of species locations is: %d\n",NOC);
		printf("\nChoose index of species to generate WPPC file (1~%d):",NOC);
		//while((ddtemp!=1&&ddtemp!=2&&ddtemp!=3&&ddtemp!=4&&ddtemp!=5)||ddtemp>NOC) scanf("%d",&ddtemp);
		ddtemp = 1;

		bytes = 0;
	        tempd = sizeof(I4);
	    fread(&tempint, sizeof(int), 1, ewc);
		fread(&bloc[1],tempd,6,ewc);
		bytes += 6*tempd;
		bytes += (tempd = sizeof(I4));
		fread(&nbl,tempd,1,ewc);
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		xbl=alc_mc(1,nbl,0,6,sizeof(R8),"xbl");
		ibl=alc_mc(1,nbl,0,6,sizeof(I4),"ibl");
		cp=(R8 **)alc_mc(1,1,1,np,sizeof(R8),"cp");
		windz=(R8 **)alc_mc(1,1,1,np,sizeof(R8),"windz");

		for(ii=1;ii<=nbl;ii++)
		{
			tempd = sizeof(R8);
			bytes = 0;
			fread(&tempint, sizeof(int), 1, ewc);
			sbl=xbl[ii];
			fread(&sbl[1],tempd,6,ewc);
			bytes += 6*tempd;
			checkbt(tempint, bytes);
			fread(&tempint, sizeof(int), 1, ewc);

			tempd = sizeof(I4);
			bytes = 0;
			fread(&tempint, sizeof(int), 1, ewc);
			lbl=ibl[ii];
			fread(&lbl[1],tempd,6,ewc);
			bytes += 6*tempd;
			checkbt(tempint, bytes);
			fread(&tempint, sizeof(int), 1, ewc);
		}

		if(bloc[1] == 0) i1 =1;
		else i1 = bloc[1];
		if(bloc[3] == 0) j1 =1;
		else j1 =bloc[3];
		if(bloc[5]==0) k1 =1;
		else k1 = bloc[5];
		i2 = bloc[2];
		j2 = bloc[4];
		k2 = bloc[6];

		p=alc_ac(i1,i2+1,j1,j2+1,k1,k2+1,sizeof(R8),"p");
		xf=alc_v(i1-1,i2+1,sizeof(R8),"xf");
		yf=alc_v(j1-1,j2+1,sizeof(R8),"yf");
		zf=alc_v(k1-1,k2+1,sizeof(R8),"zf");

		tempd = sizeof(R8);
		bytes = 0;
		fread(&tempint, sizeof(int), 1, ewc);
		for(i=i1-1;i<=i2+1;i++)
		{
			fread(&xx,tempd,1,ewc);
			xf[i]=xx;
			bytes += tempd;
		}
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		bytes = 0;
		fread(&tempint, sizeof(int), 1, ewc);
		for(j=j1-1;j<=j2+1;j++)
		{
			fread(&yy,tempd,1,ewc);
			yf[j]=yy;
			bytes += tempd;
		}
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		bytes = 0;
		fread(&tempint, sizeof(int), 1, ewc);
		for(k=k1-1;k<=k2+1;k++)
		{
			fread(&zz,tempd,1,ewc);
			zf[k]=zz;
			bytes += tempd;
		}
		checkbt(tempint, bytes);
		fread(&tempint, sizeof(int), 1, ewc);

		_block=(I1*)alc_eci(20000, "block-size");
		temptime0 = NULL;
		printf("\nSearching wind pressure/species level in EWC file\n");
		for(;;)
		{
			bytes = 0;
			bytes += (tempd = sizeof(R8));
			tempint = checkeof();
		    if(!tempint) break;

			temptime=(FDS_TIME *)alc_ec(&_block,sizeof(FDS_TIME),"fds-time");

			if(temptime0)
				temptime0=temptime0->next=temptime;
			else
				temptime0=_fdstime0=temptime;

			fread(&curta1,tempd,1,ewc);
			checkbt(tempint, bytes);
			fread(&tempint, sizeof(int), 1, ewc);
			bytes = 0;
			tempd = sizeof(R8);
			fread(&tempint, sizeof(int), 1, ewc);
			for(k=k1;k<=k2+1;k++)
			{
				for(j=j1;j<=j2+1;j++)
				{
					for(i=i1;i<=i2+1;i++)
					{
						for(ii=1;ii<=ddtemp;ii++)
							{fread(&ppp,tempd,1,ewc);bytes += tempd;}
						p[i][j][k]=ppp;
						for(ii=1;ii<=NOC-ddtemp;ii++)
							{fread(&ppp,tempd,1,ewc);bytes += tempd;}
					}
				}
			}
			checkbt(tempint, bytes);
			fread(&tempint, sizeof(int), 1, ewc);

			bytes = 0;
			bytes += (tempd = sizeof(I4));
			fread(&tempint, sizeof(int), 1, ewc);
			fread(&DBINDEX,tempd,1,ewc);
			checkbt(tempint, bytes);
		    fread(&tempint, sizeof(int), 1, ewc);
			//count the time start to read:
			temptime->curtime=curta1;
			temptime->dbindex=DBINDEX;

			//search
			stepa = 1;
			curta = (R4) curta1;

			search(dtemp);
			_block1=(I1*)alc_eci(2000, "block-size");
			for(i=1;i<=np;i++)
			{
				if(DBINDEX==1) temppath1[i] = NULL;

				temppath=(FDS_PATH *)alc_ec(&_block1,sizeof(FDS_PATH),"fds-path");
				if(temppath1[i])
					temppath1[i]=temppath1[i]->next=temppath;
				else
					temppath1[i]=_fdspath[i]=temppath;
				temppath1[i]->cp = cp[stepa][i];
			}
		    }
	return;
}

void fds_wdb()
{
	return;
}

void checkbt(IX bt1, IX bt2)
{
	if(bt1 != bt2)
	{
		printf("Error reading CDB file created by FDS: bytes not matched, exit\n");
		Pause();
		exit(1);
	}
	return;
}

IX checkeof()
{
	IX i;
	for (i=0;i<4;i++)
	{
	  if ((header.ch[i] = fgetc(ewc)) == EOF) return 0;
	}
	return header.i;
}

void print_cfd0()
{
	I4 i,a1;
	I4 fit;
	R8 temp1;

	//to printf cp
	for(i=1;i<=np;i++)
	{
		R8 smalla,temp;
		I4 index,pointer;

		//printf("\n\nPath\\Wind/Time\tCp\n");

		//sort cp from windz of 0-360
		index=1;
		do
		{
			smalla=windz[index][i];
			pointer=index;

			for(a1=index+1;a1<=stepa;a1++)
			{
				if(windz[a1][i]<smalla)
				{
					pointer=a1;
					smalla=windz[a1][i];
				}
			}

			temp=windz[index][i];
			windz[index][i]=windz[pointer][i];
			windz[pointer][i]=temp;

			temp=cp[index][i];
			cp[index][i]=cp[pointer][i];
			cp[pointer][i]=temp;

			index++;
		}
		while(index<stepa);

	    temp1=0.0;

		for(a1=1;a1<=stepa;a1++)
		{
			 if(windz[a1][i]==0.0)	temp1=cp[a1][i];

			 if(windz[a1][i]==360.0)	cp[a1][i]=temp1;
			 //printf("Path_%ld\t%f\t%le\n",id[i],windz[a1][i],cp[a1][i]);
		}
	}

	//create lb2 file //spline fit=2
	fit=2;
	fprintf(lb2,"Wind profiles created from WPPLINK\n");
	fprintf(lb2,"wind profiles:\n");
	fprintf(lb2,"%ld\n",np);

	for(i=1;i<=np;i++)
	{
		fprintf(lb2,"%ld %ld %ld Path_%ld\n",i,stepa,fit,id[i]);//need check!!!!!!!
		fprintf(lb2,"%f %f %f\n",x[i]-xref,y[i]-yref,z[i]-zref);
		if(CSTEADY!=0) fprintf(lb2,"Wind angle %f-%f\n",ABWIND.InitA,ABWIND.LastA);
		else fprintf(lb2,"Time(s) %f-%f\n",ABWIND.InitA,ABWIND.LastA);

		/*//sort cp from windz of 0-360
		index=1;
		do
		{
			smalla=windz[index][i];
			pointer=index;

			for(a1=index+1;a1<=stepa;a1++)
			{
				if(windz[a1][i]<smalla)
				{
					pointer=a1;
					smalla=windz[a1][i];
				}
			}

			temp=windz[index][i];
			windz[index][i]=windz[pointer][i];
			windz[pointer][i]=temp;

			temp=cp[index][i];
			cp[index][i]=cp[pointer][i];
			cp[pointer][i]=temp;

			index++;
		}
		while(index<stepa);*/

		//a1=0;
		for(a1=1;a1<=stepa;a1++)
		{
			//a1++;
			//fprintf(lb2," %d %5.3lf\n",(I2)ABWIND.CurtA,cp[a1][i]);
			if(CSTEADY==2) fprintf(lb2," %d %5.3lf\n",(I2)windz[a1][i],cp[a1][i]);
			else if(CSTEADY==1) fprintf(lb2," %d %le\n",(I2)windz[a1][i],cp[a1][i]);
			else if(CSTEADY==0) fprintf(lb2," %f %le\n",windz[a1][i],cp[a1][i]);
		}
	}
	fprintf(lb2,"-999 end library");
	printf("Create wppc link from CFD0 successfull\n");
	Pause();
	return;
}

void print_fds()
{
	I4 i, max_id, min_id;
	IX fit,ii;
	FDS_PATH *timepath;
	FDS_TIME *time;
	R8 *timearray,initial, last, time0=-1.0, cstzero=0.0;
	IX _npoint = 100; //last number of points for stat report
	R8 time1=-1.0, avec_all=0.0, minc_all=1e30, maxc_all=-1.0;
	I1 _statPRJ[_MAX_PATH];
	FILE *stat;

	if(CSTEADY == 0)
	{
		pathmerge(_statPRJ,sizeof(_lb2PRJ),_plddrive,_plddir,_pld_name,".sta");
		stat = fopen(_statPRJ, "w");
		fprintf(stat, "Path Min Max Ave\n");
	}
		
	//create series of time
	timearray = alc_v(1, DBINDEX, sizeof(R8), "time_array");
	time = _fdstime0;
	for(i=1;i<=DBINDEX;i++)
	{
		timearray[i] = time->curtime;
		if(i==DBINDEX-_npoint) time1 = time->curtime;
		time = time->next;		
	}

	//get intitial and last time:
	initial = timearray[1];
	last = timearray[DBINDEX];
	
	if(CSTEADY==0)
	{
		printf("\nSet transient concentration to zero before the time (%lf ~ %lf):", TIMEMIN, TIMEMAX);
		time0 = 0.0;
		//while(time0<TIMEMIN||time0>TIMEMAX) scanf("%lf",&time0);
	}
	//to printf cp
	//for(i=1;i<=np;i++)
	//{
	//	ii = 0;
	//	printf("\n\nPath\tTime\tC\n");
	//	for(timepath=_fdspath[i];timepath;timepath=timepath->next)
	//	{
	//		ii++;
	//		//screen dump
	//		printf("Path_%ld\t%lf\t%le\n",id[i],timearray[ii],timepath->cp);
	//	}
	//}

	//create lb2 file
	//spline fit=2
	fit=2;
	fprintf(lb2,"Wind and Species profiles created from FDS WPPLINK\n");
	fprintf(lb2,"wind and species profiles:\n");
	fprintf(lb2,"%ld\n",np);

	for(i=1;i<=np;i++)
	{
		R8 maxc=-1.0, minc=1e30, avec=0.0;

		fprintf(lb2,"%ld %d %d Path_%ld\n",i,DBINDEX,fit,id[i]);//need check!!!!!!!
		fprintf(lb2,"%f %f %f\n",x[i]-xref1,y[i]-yref1,z[i]-zref1);
		if(CSTEADY!=0) fprintf(lb2,"Wind angle %f-%f\n",initial,last);
		else fprintf(lb2,"Time(s) %f-%f\n",initial,last);

		ii = 0;
		for(timepath=_fdspath[i];timepath;timepath=timepath->next)
		{
			ii++;
			if(CSTEADY==2) fprintf(lb2," %d %5.3lf\n",(I2)timearray[ii],timepath->cp);
			else if(CSTEADY==1) fprintf(lb2," %d %le\n",(I2)timearray[ii],timepath->cp);
			else if(CSTEADY==0&&timepath->next!= NULL) 
			{
				if(timepath->cp < 0.0) timepath->cp = 0.0;
				if(timearray[ii]>=time0) fprintf(lb2," %lf %le\n",timearray[ii],timepath->cp);
				else fprintf(lb2," %lf %le\n",timearray[ii], cstzero);

				//statistics time ~ C
				if(timearray[ii] > time1)
				{
					if(timepath->cp > maxc) maxc = timepath->cp;
					if(timepath->cp < minc) minc = timepath->cp;
					avec += timepath->cp;
				}
			}
		}

		if(CSTEADY==0) 
		{
			avec /= (R8) _npoint;
			fprintf(lb2," %lf %le\n", last, avec);
			avec_all +=avec;
			fprintf(stat, "%ld %le %le %le\n", id[i], minc, maxc, avec);
			if(avec < minc_all) {minc_all = avec; min_id = id[i];}
			if(avec > maxc_all) {maxc_all = avec; max_id = id[i];}
		}
	}
	
	if(CSTEADY==0) 
	{
		if(np > 0) avec_all /= np;
		fprintf(stat, "min_id min_all\n");
		fprintf(stat, "%ld %le\n", min_id, minc_all);
		fprintf(stat, "max_id max_all\n");
		fprintf(stat, "%ld %le\n", max_id, maxc_all);
		fprintf(stat, "averag_all\n");
		fprintf(stat, "%le\n", avec_all);
		fflush(stat);
	}

	fprintf(lb2,"-999 end library");
	printf("\nCreate wppc link from FDS successfull\n");
	//Pause();
	return;
}

