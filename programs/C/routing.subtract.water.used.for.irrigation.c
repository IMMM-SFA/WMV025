/*
 * SUMMARY:      Program used to modify 
 *               routed runoff because of irrigation water use. Assumes daily VIC flux files.
 * USAGE:        a.out <soilfile> 

 *                     <basin number>
 *                     <freeirr (0/1)>

 * AUTHOR:       Ingjerd Haddeland
 * E-MAIL:       ingjerd.haddeland@geo.uio.no
 * ORIG-DATE:    Dec 2003
 * LAST-MOD: Dec 2010, Ingjerd Haddeland 
 * DESCRIPTION:  
 * DESCRIP-END.
 * FUNCTIONS:    
 * COMMENTS:    Filename of routed runoff in the routing output directory 
 *              must correspond to cellnumber in soilfile.  

gcc -lm -Wall -o ../bin/routing.subtract.water.used.for.irrigation.wur routing.subtract.water.used.for.irrigation.wur.c

a.out rout/input/28.reservoirs_extractwater irr.test frac.test  output/28.wb.irr_false_res/ rout/output/28.work.modified/ rout/output/28.wb.irr_false_res/ 28 5 1 1 0

run from /mn/hox/d1/ingjerd/global/run

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

#define EPS 1e-7		/* precision */
#define OLD_DECIMAL_PLACES 4   /* Number of decimal places used by 
				  old gridded met station files */
#define NEW_DECIMAL_PLACES 4   /* Number of decimal places used by 
				  new gridded met station files */
#define TRUE 1
#define FALSE 0
/******************************************************************************/
/*			TYPE DEFINITIONS, GLOBALS, ETC.                       */
/******************************************************************************/

const double DEGTORAD = 3.14159265358979323846/180.;
const double EARTHRADIUS = 6378.;
const double CONVFACTOR = 86.4; /* Conversion factor from m3/s to mm*area(km2)/day */
/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
void CalcArea(float lat,float latres,float lonres,float *area);
int  LineCount(char *);
void ReadFluxes(char *,float **,int **,float,float,
		int,int,int,int,int);
void ReadFraction(char fracfile[400],float lat,float lon,float *fraction);
void ReadIrr(char irrfile[400],float IRR[20]);
int ReadList(char *,int **,int);
int ReadUpstream(char irrfile[400],float UPSTREAM[10][5]);
void ReadExtractWaterFile(char extractfile[400],float lat,float lon,
			  float LL[25][5],int *upstream_dams);
void WriteData(char outdir[400], char tempdir[400], float **FLUX,
	       float UPSTREAM[10][5],float lat,float lon,float area,
	       float LL[25][5],int upstream_cells,int upstream_dams,int basin,int irrflag,
	       int freeirr,int reservoir,int ndays,int PrecCol,int PrecOrigCol,int ExtractWaterCol);
int IsLeapYear(int);
/******************************************************************************/
int main(int argc, char *argv[])
{
  char fluxdir[400]; 
  char irrfile[400];
  char fracfile[400];
  char outdir[400];
  char tempdir[400];
  char flist[400];
  char upstreamfile[400];
  char extractfile[400];
  int **LIST;
  int i,nbytes,skip;
  int basin;
  int freeirr,irrflag;
  int upstream_dams;
  int reservoir;
  int upstream_cells;
  int ndays;
  int Nparam;
  int ForceYear,StartYear,EndYear;
  int PrecCol,PrecOrigCol,ExtractWaterCol;
  float **FLUX; 
  float LL[25][5];
  float IRR[20];
  float UPSTREAM[10][5];
  float lat,lon,latres;
  float area;
  float fraction;
  float irrigationincell;

  if(argc!= 21) {
    fprintf(stderr,"Usage: %s <extractfile> <irr file> <fluxdata dir> <outdir> <basin number> <latres>\n",argv[0]);
    fprintf(stderr,"\t<extractfile> is the file telling which reservoirs to extract water from\n");
    fprintf(stderr,"\t<irr file> is the file containing percent irrigated area pr month in cell\n");
    fprintf(stderr,"\t<upstream cells file> is the file containing information on upstream cells\n");
    fprintf(stderr,"\t<fracfile> is the file containing cell lon, lat, and fraction\n");
    fprintf(stderr,"\t<listfile> is the file containing list of VIC output fluxes\n");
    fprintf(stderr,"\t<fluxdata dir> is the directory with current VIC output (fluxes)\n");
    fprintf(stderr,"\t<outdir> is the directory where to write modified local runoff values\n");
    fprintf(stderr,"\t<basin number> is basin number\n");
    fprintf(stderr,"\t<latres> is grid resolution (in degrees)\n");
    fprintf(stderr,"\t<reservoir 0/1> 0: no reservoirs, 1: reservoirs included\n");
    fprintf(stderr,"\t<irrflag 0/1> 0: no irrigation, 1: irrigation\n");
    fprintf(stderr,"\t<freeirr 0/1> 0: irrigation restricted, 1: free irrigation\n");
    fprintf(stderr,"\t<irrigation in cell>\n");
    exit(0);
  }

  strcpy(extractfile,argv[1]);
  strcpy(irrfile,argv[2]);
  strcpy(upstreamfile,argv[3]); 
  strcpy(flist,argv[4]); 
  strcpy(fracfile,argv[5]); 
  strcpy(fluxdir,argv[6]);
  strcpy(outdir,argv[7]);
  basin=atof(argv[8]);
  latres=atof(argv[9]);
  reservoir=atoi(argv[10]);
  irrflag=atoi(argv[11]);
  freeirr=atoi(argv[12]);
  irrigationincell=atof(argv[13]);
  ForceYear=atoi(argv[14]);
  StartYear=atoi(argv[15]);
  EndYear=atoi(argv[16]);
  PrecCol=atoi(argv[17]);
  PrecOrigCol=atoi(argv[18]);
  ExtractWaterCol=atoi(argv[19]);
  strcpy(tempdir,argv[20]);


  /* Calculate skipdays and ndays */
  skip = 0;
  /*for(i=ForceYear;i<StartYear;i++) {
   if(IsLeapYear(i)) skip+=366; 
   else skip+=365;
  }*/

  ndays=0;
  for(i=StartYear;i<=EndYear;i++) {
   if(IsLeapYear(i)) ndays+=366; 
   else ndays+=365;
  }

  ReadIrr(irrfile,IRR); //this is a one-line file (irr.month) having lat and lon for current cell, + monthly irr fractions. 
    printf("\tReadIrr finished for lat %.4f %.4f irrflag %d irrfree %d file %s\n",
       IRR[0],IRR[1],irrflag,freeirr,irrfile);
    lat=IRR[0];
    lon=IRR[1];

  upstream_cells=ReadUpstream(upstreamfile,UPSTREAM); 
  printf("\tReadUpstream finished for lat %.4f %.4f Number of upstream cells: %d\n",lat,lon,upstream_cells);

  if(reservoir==1) 
    ReadExtractWaterFile(extractfile,lat,lon,LL,&upstream_dams);
  else upstream_dams=0;
  printf("\tReadExtractWater finished for lat %.4f %.4f Number of upstream dams: %d\n",
	     lat,lon,upstream_dams);

  /* Read fluxdata for current cell */
  Nparam = LineCount(flist);
  LIST = (int**)calloc(Nparam,sizeof(int*));
  for(i=0;i<Nparam;i++) 
    LIST[i] = (int*)calloc(4,sizeof(int));
  FLUX = (float**)calloc(ndays,sizeof(float*));
  for(i=0;i<ndays;i++) 
    FLUX[i] = (float*)calloc((Nparam+1),sizeof(float));
  nbytes = ReadList(flist,LIST,Nparam);
  ReadFluxes(fluxdir,FLUX,LIST,lat,lon,Nparam,skip,ndays,basin,nbytes);
  printf("\tReadFluxes finished\n");

  CalcArea(lat,latres,latres,&area);
      printf("\tCalcArea finished area=%f irrpercent=%f\n",area,irrigationincell);

  ReadFraction(fracfile,lat,lon,&fraction);    
  printf("\tReadFraction finished fraction=%f\n",fraction);

  printf("\tStart to write data in the output file\n");
  WriteData(outdir,tempdir, FLUX,UPSTREAM,lat,lon,area,LL,upstream_cells,
	    upstream_dams,basin,irrflag,freeirr,reservoir,ndays,
	    PrecCol,PrecOrigCol,ExtractWaterCol); //includes water extractions from routed runoff and dams
  printf("\tWriteData finished frac%f irrpercent%f irrarea:%f\n",
	 fraction,irrigationincell,area);

return(EXIT_SUCCESS);
}
/******************************************************************************/
/*				   CalcArea                                   */
/******************************************************************************/
void CalcArea(float lat,		/* latitude of gridcell center in degrees */
	      float latres,	/* zonal resolution in degrees */
	      float lonres,      /* meridional resolution in degrees */
	      float *area)	
{
  lat *= DEGTORAD;
  latres *= DEGTORAD/2.;
  lonres *= DEGTORAD;
  *area = lonres * EARTHRADIUS * EARTHRADIUS * 
    fabs(sin(lat+latres)-sin(lat-latres));
}
/***************************************************************/
/*                 ReadFraction                                */
/***************************************************************/
void ReadFraction(char file[400],
		  float lat,
		  float lon,
		  float *fraction)
{
  FILE *fp;
  float fvalue;
  float templat,templon;

  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open fraction file %s \n",file);exit(0);}
  //else printf("\tFile opened: %s \n",file);

  /* initialize */
  (*fraction)=0;

  while(!feof(fp)) { 
    fscanf(fp,"%f %f %f",&templon,&templat,&fvalue);
    if(templon==lon && templat==lat) { 
      (*fraction)=fvalue;
      break;
    }
  }

  fclose(fp);
}
/*************************************************************************/
/*                 ReadIrr                                               */
/*************************************************************************/
void ReadIrr(char file[400],
	     float IRR[20])
{
  FILE *fp;
  int i;

  if((fp = fopen(file,"r"))==NULL){
    printf("\tCannot open irrfile %s \n",file);exit(0);}
  //else printf("\tFile opened: %s \n",file);

  for(i=0;i<14;i++) fscanf(fp,"%f ",&IRR[i]);
  fclose(fp);
}
/*************************************************************************/
/*                 ReadUpstream                                          */
/*************************************************************************/
int ReadUpstream(char file[400],
		  float UPSTREAM[10][5])
{
  FILE *fp;
  int i;
  int nr;

  if((fp = fopen(file,"r"))==NULL){
    printf("\tCannot open upstream file %s \n",file);exit(0);}
  //else printf("\tFile opened: %s \n",file);

  fscanf(fp,"%*d %*d %*f %*f %*d %d",&nr);
  for(i=0;i<nr;i++) { 
	fscanf(fp,"%f %f %f %f",&UPSTREAM[i][0],&UPSTREAM[i][1],&UPSTREAM[i][2],&UPSTREAM[i][3]);
	printf("\tReadUpstream %d %.4f %.4f\n",nr,UPSTREAM[i][2],UPSTREAM[i][3]);
  }
  fclose(fp);

  return nr;
}
/*************************************************************************/
/*                 ReadExtractWaterFile                                  */
/*************************************************************************/
void ReadExtractWaterFile(char file[400],
                          float lat,
			  float lon,
			  float LL[25][5],
			  int *upstream_dams)
{
  FILE *fp;
  char LINE[1000];
  int i,nr,damnr,truea;
  float latitude,longitude,damlat,damlon,mean_inflow;
  float capacity,totcapacity;

  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open extractwaterfile %s \n",file);exit(0);}
  //else printf("\tFile opened: %s \n",file);

  truea=0;
  (*upstream_dams)=0;
  totcapacity=0.;

  while(!truea) {
    fscanf(fp,"%*d %f %f %d",&latitude,&longitude,&nr);
    if(fabs(latitude-lat)<=EPS && fabs(longitude-lon)<=EPS) {
      for(i=0;i<nr;i++) { 
	printf("\tReadExtractWater %f %f %f %f %d\n",latitude,longitude,lat,lon,nr);
	fscanf(fp,"%d %f %f %f %f",
	       &damnr,&damlat,&damlon,&capacity,&mean_inflow);
	LL[i][0]=damlat;
	LL[i][1]=damlon;
	LL[i][2]=capacity;
	LL[i][3]=mean_inflow;
	(*upstream_dams)+=1;
        totcapacity+=capacity;
      }
      for(i=0;i<nr;i++) 
	LL[i][4]=LL[i][2]/totcapacity;
      truea=1;
    }
    else
      fgets(LINE,1000,fp);

  }
  fclose(fp);
}
/*************************************************************************/
/*                 WriteData                                             */
/*************************************************************************/
void WriteData(char indir[400],
              char tempdir[400],
	       float **FLUX,
               float UPSTREAM[10][5],
	       float lat,
	       float lon,
	       float area,
	       float LL[25][5],
	       int upstream_cells,
	       int upstream_dams,
	       int basin,
	       int irrflag,
	       int freeirr,
	       int reservoir,
	       int ndays,
	       int PrecCol,
	       int PrecOrigCol,
	       int ExtractWaterCol)
{
  FILE *fp,*fin;
  float discharge[10][10000]; //routed discharge at upstream locations
  float origdischarge[10][10000];
  float totalavailablewater;
  float diff;
  float deficit;
  float precip_added[10000]; //numbers in m3/s!!!
  float orig_precip_added[10000]; 
  float reservoirdischarge[25][10000];
  int i,j,dam,flag;
  char LATLON[50];
  char fmtstr[30];
  char outfile2[400];
  char outfile3[400];
  char infile[400];

 // char *tempdir = "/net/power/raid/tizhou/Colorado_run_resevp/run/rout/temp/";  /*changed by Tian, hard coded dir for temp file !!!*/
 

  /* Go through upstream cells, modify streamflow */
  for(j=0;j<ndays;j++) { 
    precip_added[j] = (FLUX[j][PrecCol]-FLUX[j][PrecOrigCol]-FLUX[j][ExtractWaterCol])*area/CONVFACTOR; 
                               /* precip-orig_precip-extract_water, from mm to m3/s!!!!
				  extract_water is taken from local cell, and is already subracted from
				  runoff/baseflow in output from VIC. The number is, however, 
				  included in 'precip'. Hence, precip_added here is what is taken from 
                                  upstream routed runoff, or from reservoirs  */
    orig_precip_added[j] = precip_added[j];
  }
  for(i=0;i<upstream_cells;i++) {
    for(j=0;j<ndays;j++) { 
      discharge[i][j]=0.;
    }
  }

  for(i=0;i<upstream_cells;i++) {
    strcpy(infile,indir);
    sprintf(LATLON,"streamflow_%.4f_%.4f",UPSTREAM[i][2],UPSTREAM[i][3]);
    strcat(infile,LATLON);
    if((fin = fopen(infile,"r"))==NULL)   
      {
		  printf("\tCannot open local streamflow file %s \n",infile);
		  exit(0);}   
    else printf("\tStreamflow file opened for reading: %s\n",infile);
    
    for(j=0;j<ndays;j++) {
      fscanf(fin,"%*d %*d %*d %f\n",&discharge[i][j]);
      origdischarge[i][j]=discharge[i][j];
      if(precip_added[j]>discharge[i][j]) {
	if(freeirr==1) {
	  discharge[i][j]-=precip_added[j];
	  precip_added[j]=0;
	}
	else {
	  precip_added[j]-=discharge[i][j];
	  discharge[i][j]=0.;
	}
      }
      else { // precip_added < upstream local discharge 
	discharge[i][j]-=precip_added[j];
	precip_added[j]=0;
      }
    }
    fclose(fin); 
  }
 
  for(i=0;i<ndays;i++) {
    deficit+= precip_added[i]; // m3/s
  }
  
  /* Modify reservoir streamflow file(s) if necessary */
  if(irrflag==1 && reservoir==1 && deficit>0) {
    /* First read distant original routed runoff at all dam locations of interest */
    /* units m3/s */
    for(dam=0;dam<upstream_dams;dam++) {
      flag=0;
      strcpy(infile,indir);
      sprintf(LATLON,"streamflow_%.4f_%.4f",LL[dam][0],LL[dam][1]);
      strcat(infile,LATLON);
      if((fin = fopen(infile,"r"))==NULL)  
	{printf("Cannot open distant file %s \n",infile);
	  flag=1;}   
      else printf("\tDistant reservoir release file opened for reading: %s\n",infile);
      
      for(i=0;i<ndays;i++) {
	if(flag==0) {
	  fscanf(fin,"%*d %*d %*d %f\n",&reservoirdischarge[dam][i]);
	}
	else reservoirdischarge[dam][i]=0;
      }
      if(flag==0) fclose(fin);
    } // streamflow from all upstream dam locations read

    /* Then subtract water from these locations if they exist */
    if(upstream_dams>0) {
      for(i=0;i<ndays;i++) {
	totalavailablewater=0.;
	for(dam=0;dam<upstream_dams;dam++) 
	  totalavailablewater+=reservoirdischarge[dam][i]; //still in m3/s
	for(dam=0;dam<upstream_dams;dam++) {
	  if(precip_added[i]>0.0) {
	    diff = precip_added[i];
	    reservoirdischarge[dam][i]-=precip_added[i]*reservoirdischarge[dam][i]/totalavailablewater;
            precip_added[i]-=diff; 
	  } 
	}
      }
      /* and write back to file. units: m3/s */
      for(dam=0;dam<upstream_dams;dam++) {
	strcpy(outfile2,tempdir);
	sprintf(LATLON,"streamflow_%.4f_%.4f",LL[dam][0],LL[dam][1]);
	strcat(outfile2,LATLON);
	printf("\n-----------------============--------------\n");
	printf("the streamflow output file of dams is %s\n", outfile2);
	printf("\n---------------------------------------------\n");
	if((fp = fopen(outfile2,"w"))==NULL)   
	  {printf("Cannot open file to write to: %s \n",outfile2);exit(0);}   
	else printf("\tFile opened for writing (water taken from other sources, e.g. reservoir): %s\n",outfile2);
	
	for(i=0;i<ndays;i++) {
	  if(reservoirdischarge[dam][i]>0) 
	    fprintf(fp,"%.0f %.0f %.0f %.5f\n",
		    FLUX[i][0],FLUX[i][1],FLUX[i][2],reservoirdischarge[dam][i]);
	  else   fprintf(fp,"%.0f %.0f %.0f %.5f\n",
			 FLUX[i][0],FLUX[i][1],FLUX[i][2],0.);
	}
	fclose(fp); 
      }
    }
  }

  /* Write modified discharge to file(s) */
  for(i=0;i<upstream_cells;i++) {
    strcpy(outfile3,tempdir);

sprintf(LATLON,"streamflow_%.4f_%.4f", UPSTREAM[i][2], UPSTREAM[i][3]); //changed by Tian

  //sprintf(fmtstr,"streamflow_%%.%if_%%.%if",OLD_DECIMAL_PLACES,OLD_DECIMAL_PLACES);
  //sprintf(LATLON,fmtstr,UPSTREAM[i][2], UPSTREAM[i][3]);

    strcat(outfile3,LATLON);
	printf("\n----------------------------------");
	printf("\n----------------------------------\n");
	printf("the streamflow output file of upstream cells is %s\n", outfile3);
	printf("\n----------------------------------\n");
    if((fp = fopen(outfile3,"w"))==NULL)   
      {printf("Cannot open testfile %s \n",outfile3);exit(0);}   
    else printf("\tFile opened for writing (testfile): %s\n",outfile3);

    for(j=0;j<ndays;j++) {
      fprintf(fp,"%.0f %.0f %.0f %f\n",
	      FLUX[j][0],FLUX[j][1],FLUX[j][2],discharge[i][j]);
	 /* printf("%.0f %.0f %.0f %f\n",
	      FLUX[j][0],FLUX[j][1],FLUX[j][2],discharge[i][j]);*/
    }
    fclose(fp); 
  }

}
/********************************************/
/* Function returns number of lines in file */
/********************************************/
int LineCount(char *file)
{
  FILE *fp;
  int c, lines;
  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open linecount file %s, exiting \n",file);exit(0);}
  lines = 0;
  while((c = fgetc(fp)) !=EOF)  if (c=='\n') lines++;
  fclose(fp);
  return lines;
}

/*******************************************/
/*Read parameter list                      */
/********************************************/
int ReadList(char *flist,int **LIST,int Nparam)
{
  FILE *fp;
  int i;
  int nbytes;

  if((fp = fopen(flist,"r"))==NULL){
    printf("Cannot open file %s, exiting \n",flist);
    exit(0);
  }

  nbytes=0;

  for(i=0;i<Nparam;i++) {
      fscanf(fp,"%*s %d %d %d %d",&LIST[i][1],&LIST[i][2],&LIST[i][3],&LIST[i][4]);
      if(LIST[i][3]!=3 && LIST[i][3]!=5) nbytes+=LIST[i][3];
      if(LIST[i][3]==3) nbytes+=2;
      if(LIST[i][3]==5) nbytes+=4;     
  }
  fclose(fp);

  return nbytes;
}/* END function */
/****************************************/
/* ReadFluxes:                         */
/*****************************************/
void ReadFluxes(char *indir,
		float **FLUX, 
		int **LIST,
		float lat, 
		float lon, 
		int Nparam,
		int skip,
		int ndays,
		int basin,
		int nbytes)
{
  FILE *fp;
  int i,j,k;
  char LATLON[60];
  char file[400];
  char fmtstr[30];
  char tmpdata[100];
  char *cptr;
  short int *siptr;
  unsigned short int *usiptr;
  int   *iptr;
  float *fptr;

  /* Allocate */
  cptr = (char *)malloc(1*sizeof(char));
  usiptr = (unsigned short int *)malloc(1*sizeof(unsigned short int));
  siptr = (short int *)malloc(1*sizeof(short int));
  iptr = (int *)malloc(1*sizeof(int));
  fptr = (float *)malloc(1*sizeof(float));

  /* Make filename */
  sprintf(fmtstr,"fluxes_%%.%if_%%.%if",OLD_DECIMAL_PLACES,OLD_DECIMAL_PLACES);
  sprintf(LATLON,fmtstr,lat,lon);
  strcpy(file,indir);
  strcat(file,LATLON);
  if((fp = fopen(file,"rb"))==NULL) { 
    printf("Cannot open file %s, exiting\n",file);exit(0); 
  }
  //else printf("%s\n",file);

   /* Initialize data */
  for(i=0;i<ndays;i++) 
    for(j=0;j<Nparam;j++)
      FLUX[i][j]=0.;

  /* Skip data */
  printf("\n-----------------------------------\n");
  printf("\nthe skipped lines are %d\n", skip);
  for(i=0;i<skip;i++) 
      fread(tmpdata,nbytes,sizeof(char),fp);

  /* Read data */
  for(i=0;i<ndays;i++) {
      for(k=0;k<Nparam;k++) {
	if(LIST[k][3]==1) { 
	  fread(cptr,1,sizeof(char),fp);
	  FLUX[i][k] = (int)cptr[0];
	}
	if(LIST[k][3]==2) { 
	  fread(usiptr,1,sizeof(unsigned short int),fp);
	  FLUX[i][k] = (float)((int)usiptr[0]/(float)LIST[k][4]);
	}
	if(LIST[k][3]==3) {
	  fread(siptr,1,sizeof(short int),fp);
	  FLUX[i][k] = (float)((int)siptr[0]/(float)LIST[k][4]);
	}
	if(LIST[k][3]==4) {
	  fread(fptr,1,sizeof(float),fp);
	  FLUX[i][k] = fptr[0];
	}
	if(LIST[k][3]==5) { 
	  fread(iptr,1,sizeof(int),fp);
	  FLUX[i][k] = (int)iptr[0];
	}
      } 
  }
  fclose(fp);
}
/****************************************************************************/

/***************************/
/*IsLeapYear               */
/***************************/

int IsLeapYear(int year) 
{
  if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) 
    return TRUE;
  return FALSE;
}
/****************************/
