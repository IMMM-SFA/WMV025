/*
 * SUMMARY:      Program used to calculate total fluxvalues in each cell   
                 and mean monthly values over the area. Reads binary files.
 *               Needs the lat and lon and fraction from a file. 
                 *cropping.month defines the area
                 Calculates area of each cell.  
 *               Output: File 1 gives cell location, fraction and total values.
                         File 2 gives mean monthly values over the area
                         (12*year values).
			 File 3 gives mean monthly values (12 values).
			 File 4 gives mean monthly values for each cell.
 * DESCRIPTION OF USE:  fluxdata.calculate.total.binary
 			   -c irr file
			   -k flag (1deg flux files: 1)
                           -l fluxdata path, current simulation
                           -m continent (or basin)
                           -n resolution
                           -o postfix
                           -p startyear
			   -q parameter list (file)
                           -r endyear
                           -s latlong file 
                           -t leapdays
			   -u timestep in hours
			   -v skip days
			   -z fluxdata path, non irrigated
 * AUTHOR:       Ingjerd Haddeland
 * ORG:          University of Oslo, Department of Geophysics
 * E-MAIL:       ingjerd@geo.uio.no
 * ORIG-DATE:    Feb 2001
 * LAST-MOD:     Oct 2010


gcc -lm -Wall  ../programs/C/fluxdata.binary.to.daily.ascii.c 

run from /home/iha/watch/run 

#############
# Forcingmip, using WFD:
./a.out -lbatch1/output/ -m0 -n5 -owfd -q../misc/fluxes.forcingmip -s../misc/LatLon.cru -u3 -p1981 -r1996 -t3 -c../../data/Justin/2010/soil1.txt

#################
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

#define DECIMAL_PLACES 4
#define EPS 1e-7            // precision 
#define FALSE 0
#define TRUE 1
#define ENOERROR 0		/* no error */

char *optstring = "c:k:l:m:n:o:p:q:r:s:t:u:v:w:z:";
const double DEGTORAD = M_PI/180.;
const double EARTHRADIUS = 6371.229; //Same as in routing program

/* Function prototypes **********************************************/
void ReadArgs(int,char **,char *,char *,char *,char *,int *,
	      int *, int *,int *,int *);
void Usage(char *);
int  CalcArea(float lat,float latres, float lonres, float *area);
int  DaysOfMonth(int,int);
int  IsLeapYear(int);
int  LineCount(char *);
void ReadFluxes(char *,float **,int **,float,float,
		int,int,int,int,int,int);
void ReadLatlong(char *,float **,int);
int  ReadList(char *,int **,int);
/*****************************************************************/

int main(int argc, char *argv[]) 
{
  FILE *fp;

  char *flatlong,*firr,*inpath,*outpath;
  char *firryear;
  char *ffluxdata;
  char *flist,*postfix;
  char *INLATLON,*flxstr;

  float **LLF;
  float **FLUXPARAM;
  float lat,lon;
  float trueres;

  int **LIST;
  int basin;
  int res;
  int i,j;
  int cells,Nparam;
  int skip;
  int nbytes;
  int days;
  int years;
  int StartSimYear;
  int StartYear;
  int EndYear;
  int QsbCol;
  int QsCol;

  /* Allocate memory for filenames */
  inpath = (char*)calloc(150,sizeof(char));
  outpath = (char*)calloc(150,sizeof(char));
  flatlong = (char*)calloc(100,sizeof(char));
  firr = (char*)calloc(100,sizeof(char));
  firryear = (char*)calloc(100,sizeof(char));
  flist = (char*)calloc(100,sizeof(char));
  ffluxdata = (char*)calloc(100,sizeof(char));
  postfix = (char*)calloc(100,sizeof(char));
  INLATLON = (char*)calloc(100,sizeof(char));
  flxstr = (char*)calloc(50,sizeof(char));
  

  /* Read in arguments (infile,outfiles etc) */
  ReadArgs(argc,argv,inpath,outpath,flist,flatlong,&StartSimYear,
	   &StartYear,&EndYear,&QsCol,&QsbCol);

  cells = LineCount(flatlong);
  Nparam = LineCount(flist);

  trueres=0.5;
  //printf("\nFile %s has %d lines\n",flatlong,cells);
  //printf("File %s has %d lines\n",flist,Nparam);
  //printf("Resolution: %f\n\n",trueres);

  years=EndYear-StartSimYear+1;
  skip=0;
  days=0;
  for(i=StartSimYear;i<=EndYear;i++) {
    if(IsLeapYear(i)) days+=366;
    else days+=365;
  }

  /* Allocate memory for FLUXDATA,LATLONG,LIST */
  FLUXPARAM = (float**)calloc(days,sizeof(float*));
  for(i=0;i<days;i++) 
    FLUXPARAM[i] = (float*)calloc((Nparam+1),sizeof(float));
  LLF = (float**)calloc(cells,sizeof(float*));
  for(i=0;i<cells;i++) 
    LLF[i] = (float*)calloc(4,sizeof(float));
  LIST = (int**)calloc(Nparam,sizeof(int*));
  for(i=0;i<Nparam;i++) 
    LIST[i] = (int*)calloc(4,sizeof(int));

  /* Read files */
  ReadLatlong(flatlong,LLF,cells);  
  nbytes=ReadList(flist,LIST,Nparam);
  //printf("years %d days %d skip %d nbytes %d\n\n",years,days,skip,nbytes);

  /* Go through cells */
  for(i=0;i<cells;i++) { 

    lat=LLF[i][0];
    lon=LLF[i][1];
 
   sprintf(flxstr,"fluxes_%%.%if_%%.%if",DECIMAL_PLACES,DECIMAL_PLACES); // add by Tian 2013
   sprintf(INLATLON,flxstr,lat,lon);
   strcpy(ffluxdata,outpath);
   strcat(ffluxdata,INLATLON);
  
    //sprintf(ffluxdata,"/net/power/raid/tizhou/Colorado_run/run/output/daily.ascii/fluxes_%.2f_%.2f",lat,lon);          //                HERE IS A HARD CODED PATH!!!!!!
    if((fp = fopen(ffluxdata,"w"))==NULL){
      printf("Cannot open file %s \n",ffluxdata);exit(0);} 
    //else printf("\nFile opened: %s \n",ffluxdata); 

    if(i%100==0) printf("Cell:%d lat=%f lon=%f inpath=%s\n",i,lat,lon,inpath);
    
    ReadFluxes(inpath,FLUXPARAM,LIST,lat,lon,Nparam,
	       skip,days,basin,res,nbytes);

    /* Write ascii file fluxes_* */
    fprintf(fp,"Year month Day Precip PrecipOrig Snowf Evap Runoff baseflow SWE ExWater SoilM PotLake Wind AirTemp\n");
    for(j=0;j<days;j++) 
      fprintf(fp,"%d %d %d %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n",(int)FLUXPARAM[j][0],(int)FLUXPARAM[j][1],(int)FLUXPARAM[j][2],FLUXPARAM[j][3],FLUXPARAM[j][4],FLUXPARAM[j][5],FLUXPARAM[j][6],FLUXPARAM[j][7],FLUXPARAM[j][8],FLUXPARAM[j][9],FLUXPARAM[j][10],FLUXPARAM[j][11],FLUXPARAM[j][12],FLUXPARAM[j][13],FLUXPARAM[j][14],FLUXPARAM[j][15]);
    fclose(fp);
      
  } //for i=cells
  
  return(0);
}
/* END MAIN ******************************************************************/

/************************************************************************************/
/* Read_Args:  This routine checks the command line for valid program options.  If
 * no options are found, or an invalid combination of them appear, the
 * routine calls usage() to print the model usage to the screen, before exiting. */ 
/************************************************************************************/
void ReadArgs(int argc,char *argv[],char *inpath,char *outpath,
	      char *flist,char *flatlong,
	      int *StartSimYear,int *StartYear,int *EndYear,
	      int *QsCol,int *QsbCol)
{
  extern int getopt();
  extern char *optarg;
  extern char *optstring;

  int optchar;

  if(argc==1) {
    Usage(argv[0]);
    exit(1);
  }

  while((optchar = getopt(argc, argv, optstring)) != EOF) {
    switch((char)optchar) {
    case 'l': //Input Path 
      strcpy(inpath, optarg);
      break;
    case 'o': //Output Path 
      strcpy(outpath, optarg);
      break;
    case 'm': //QsCol
      *QsCol = atoi(optarg);
      break;
    case 'n': //QsbCol
      *QsbCol = atoi(optarg);
      break;
    case 'q': //Parameter list
      strcpy(flist,optarg);
      break;
    case 's': //Latlong file
      strcpy(flatlong,optarg);
      break;
    case 'p': //StartSimYear
      *StartSimYear = atoi(optarg);
      break;
    case 'v': //startyear
      *StartYear = atoi(optarg);
      break;
    case 'r': //endyear
      *EndYear = atoi(optarg);
      break;
    default: //Print Usage if Invalid Command Line Arguments
      Usage(argv[0]);
      exit(1);
      break;
    }
  }  
} // end ReadArgs

/****************************************************/
/* Usage: Function to print out usage details.      */  
/*****************************************************/
void Usage(char *temp)
{
    fprintf(stderr,"Usage: %s -l<original fluxdata dir> \
                    -m<basin number> \
                    -n<resolution> \
                    -o<postfix> \
                    -q<parameter list file> \
                    -r<path to irrigated cells> \
                    -s<latlong file>  \
                    -u<timestep> \
                    -v<skip> \n",temp);
    fprintf(stderr,"\t<flux-data dir> is the directory for the resulting flux-files \n");
    fprintf(stderr,"\t<metdata-dir> is the directory for the DAILY met-files w/4col.\n");
    fprintf(stderr,"\t<latlongfraction-file> is the file with lat and long and fraction\n");
    fprintf(stderr,"\t<soil-file> is the soilfile \n");
    fprintf(stderr,"\t<output fluxes xyz> is the x column total flux file to be written.\n");
    fprintf(stderr,"\t<monthfile> is the x column monthly total flux file to be written.\n");
    fprintf(stderr,"\t<number of parameters> is the nr of param \
                                 (data types/columns) in the flux file.\n");
    fprintf(stderr,"\t<skip days> is the # days to skip in input flux file.\n");
    exit(0);
}
/**********************************************************************/
/*			   CalcArea                                   */
/**********************************************************************/
int CalcArea(float lat,		/* latitude of gridcell center in degrees */
	     float latres,	/* zonal resolution in degrees */
	     float lonres,	/* meridional resolution in degrees */
	     float *area)	
{
  double testarea;
  double PI,radius,size;

  lat *= DEGTORAD;
  latres *= DEGTORAD/2.;
  lonres *= DEGTORAD;
  *area = lonres * EARTHRADIUS * EARTHRADIUS * 
    fabs(sin(lat+latres)-sin(lat-latres));

  PI=4.0*atan(1.0);
  radius=(double)EARTHRADIUS;
  size=0.5;
  testarea = radius*radius*fabs(size)*PI/180*            
      fabs(sin((lat-size/2.0)*PI/180)-     
	   sin((lat+size/2.0)*PI/180));

  return ENOERROR;
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
/* ReadLatLong                             */
/********************************************/
void ReadLatlong(char *llf,float **LLF,int cells)
{
  FILE *fp;
  int i;
  if((fp = fopen(llf,"r"))==NULL){
    printf("Cannot open latlong file %s, exiting \n",llf);exit(0);}
  //  else printf("ReadLatLong file opened %s\n",llf);
  for(i=0;i<cells;i++) fscanf(fp,"%f %f ",&LLF[i][0],&LLF[i][1]);
  fclose(fp);
}/* END function */

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
		int days,
		int basin,
		int res,
		int nbytes)
{
  FILE *fp;
  int i,j,k;
  char LATLON[60];
  char file[400];
  char fmtstr[17];
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
  sprintf(fmtstr,"fluxes_%%.%if_%%.%if",DECIMAL_PLACES,DECIMAL_PLACES);
  sprintf(LATLON,fmtstr,lat,lon);
  strcpy(file,indir);
  strcat(file,LATLON);
  if((fp = fopen(file,"rb"))==NULL) { 
    printf("Cannot open input file %s %s\n",file,indir);
    exit(0); 
  }
  //else printf("File opened: %s\n",file);

   /* Initialize data */
  for(i=0;i<days;i++) 
    for(j=0;j<Nparam;j++)
      FLUX[i][j]=0.;

 /* Skip data */
  for(i=0;i<skip;i++) 
      fread(tmpdata,nbytes,sizeof(char),fp);
  //printf("skip:%d days and %d bytes\n",skip,nbytes);

  /* Read data */
  for(i=0;i<days;i++) {
     for(k=0;k<Nparam;k++) {
      if(LIST[k][3]==1) { 
	fread(cptr,1,sizeof(char),fp);
	FLUX[i][k] = (int)cptr[0];
      }
      if(LIST[k][3]==2) { 
	fread(usiptr,1,sizeof(unsigned short int),fp);
	if(LIST[k][2]==1) //calculate total 
	  FLUX[i][k] += (float)((int)usiptr[0]/(float)LIST[k][4]);
	if(LIST[k][2]==2) //calculate mean 
	  FLUX[i][k] += (float)((int)usiptr[0]/((float)LIST[k][4]));
      }
      if(LIST[k][3]==3) {
	fread(siptr,1,sizeof(short int),fp);
	if(LIST[k][2]==1) //calculate total 
	  FLUX[i][k] += (float)((int)siptr[0]/(float)LIST[k][4]);
	if(LIST[k][2]==2) //calculate mean 
	  FLUX[i][k] += (float)((int)siptr[0]/((float)LIST[k][4]));
      }
      if(LIST[k][3]==4) {
	fread(fptr,1,sizeof(float),fp);
	if(LIST[k][2]==1) //calculate total 
	  FLUX[i][k] += fptr[0];
	if(LIST[k][2]==2) //calculate mean 
	  FLUX[i][k] += fptr[0];
      }
      if(LIST[k][3]==5) { 
	fread(iptr,1,sizeof(int),fp);
	FLUX[i][k] = (int)iptr[0];
      }
    }
  } 

  fclose(fp);
}

/*****************************/
/*DaysOfMonth                */
/*****************************/
int DaysOfMonth(int month,int year)
{
  int DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int i;

  if(IsLeapYear(year)) DaysInMonth[1]=29;

  i=DaysInMonth[month-1];

  return i;
}
/***************************/
/*IsLeapYear               */
/***************************/

int IsLeapYear(int year) 
{
  if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) 
    return TRUE;
  return FALSE;
}
