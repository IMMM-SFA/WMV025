/*
 * SUMMARY:      Program used to include two more columns 
 *               (upstream runoff and reservoir/point runoff 
 *               available for irrigation, in mm averaged over the cell area) 
 *               to a cell's metdata file. 
 * USAGE:        a.out <soilfile> 
                       <reservoir extractwaterfile>
 *                     <irrfile> 
 *                     <original metdata directory> 
 *                     <new metdata directory>
 *                     <routing output directory (where to read routed runoff values> 
 *                     <routing output directory (where to read routed runoff values> 
 *                     <basin number>
 *                     <latres>
 *                     <reservoirs included (1) or not (0)>    
 *                     <upstream grid cells>
 *                     <irrincell>
 *                     <forceyear>
 *                     <startyearsim> 
 *                     <endyear> 
 * AUTHOR:       Ingjerd Haddeland
 * E-MAIL:       iha@nve.no
 * ORIG-DATE:    Feb 2003
 * LAST-MOD: Dec 2010, Ingjerd Haddeland 
 * DESCRIPTION:  
 * DESCRIP-END.
 * FUNCTIONS:    
 * COMMENTS:   Filename of routed runoff in the routing output directory 
 *              must correspond to cellnumber in soilfile.  
 *            Here: SoilCols = 56
 *            Here: OldParam (metfile) = 10

gcc -lm -Wall ../programs/C/metdata.modify.runoff.c


*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

#define EPS 1e-7		/* precision */
#define SOILCOLS 56
#define OLD_PARAM 4           /* Number of parameters in old metfile */ 
#define OLD_DECIMAL_PLACES 4   /* Number of decimal places used by 
				  old gridded met station files */
#define NEW_DECIMAL_PLACES 4   /* Number of decimal places used by 
				  new gridded met station files */
#define TRUE 1
#define FALSE 0
/******************************************************************************/
/*			TYPE DEFINITIONS, GLOBALS, ETC.                       */
/******************************************************************************/

typedef enum {double_f, int_f, float_f, long_f, short_f} FORMAT_SPECIFIER;

typedef struct _CELL_ *CELLPTR;
typedef struct _CELL_ {
  int row;
  int col;
  float lat;
  float lon;
  float frac;
} CELL;


const char *frac_path = "rout/input";
const double DEGTORAD = M_PI/180.;
const double EARTHRADIUS = 6378.;
const double CONVFACTOR = 86.4; /* Conversion factor from m3/s to mm*area(km2)/day */
/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
void CalcArea(float lat,float latres,float lonres,float *area);
void ReadFracFile(const char *frac_path,CELL **fraccells,int basin,float latres,
		 float lat,float lon,float *fracarea);
void ReadIrr(char irrfile[400],float IRR[20]);
void ReadMetdata(char metdir[400],float **MET,
		 float lat,float lon,int skip,int ndays);
void ReadStreamflow(char rundir[400],char upstreamfile[400],double **STREAMFLOW,int basin,int cell,
		    float lat,float lon,int ndays);
void ReadReservoirRouted(char rundir[400],double **ROUTED,int basin,int cell,
			 float latitude,float longitude,int ndays);
void ReadSoil(char soilfile[400],float *lat,float *lon,int *cell);
void FindPointsToExtractWaterFrom(char extractfile[400],float LL[25][5],
				  float lat,float lon,int *count);
void WriteData(char outfir[400],float **MET,
	       double **RUNOFF,double *AVAILWATER,
	       float IRR[20],float lat,float lon,float area,int ndays,int Nparam);
int  IsLeapYear(int);
/******************************************************************************/
int main(int argc, char *argv[])
{
  char metdir[400]; 
  char rundir[400];
  char routdir[400];
  char outdir[400];
  char soilfile[400];
  char extractfile[400];
  char irrfile[400];
  char upstreamfile[400];
  int cell,basin,count,i,j,reservoir,Nparam;
  int StartYear,EndYear,ForceYear,skip,ndays;
  float **MET;
  double **STREAMFLOW; //Local streamflow. 0: year, 1: month, 2: day, 3: streamflow
  double **ROUTED; //Routed runoff from reservoir. 0: year, 1: month, 2: day, 3: runoff
  double *AVAILWATER; //Total available water from reservoir(s)
  float IRR[20];
  float LL[25][5]; //0:lat, 1:lon, 2:capacity 3:mean_inflow 4:capacity/totcapacity 
  float lat,lon,latres;
  float area,irrarea,fracarea;
  CELL **fraccells = NULL;

  if(argc!= 16) {
    fprintf(stderr,"Usage: %s <soilfile> <extractfile> <irrfile> <old metdata dir> <new metdata dir>\n \
                              <runoff dir> <routing dir> <basin number> <latres> <reservoirs> <upstreamcells>\n \
                              <irrincell> <forceyear> <startyearsim> <endyear> \n",argv[0]);
    fprintf(stderr,"\t<soilfile> is the soilfile to extract lat and lon from\n");
    fprintf(stderr,"\t<extractfile> gives information on which dams water can be extracted from\n");
    fprintf(stderr,"\t<irrfile> is cropping calendar for cell in question\n");
    fprintf(stderr,"\t<old metdata directory> is the directory for the 'old' forcing data\n");
    fprintf(stderr,"\t<new metdata directory> is the directory where to put the 'new' forcing data\n");
    fprintf(stderr,"\t<runoff directory> is the directory where to find the local routed runoff values\n");
    fprintf(stderr,"\t<routing directory> is the directory where to find the distant routed runoff values\n");
    fprintf(stderr,"\t<basin> is basin number\n");
    fprintf(stderr,"\t<latres> is grid resolution (in degrees)\n");
    fprintf(stderr,"\t<reservoirs> included (1) or not (0) in simulation\n");
    fprintf(stderr,"\t<upstream cells (file with one line)");
    fprintf(stderr,"\t<irrincell> percent");
    fprintf(stderr,"\t<forceyear> startyear old forcingdata");
    fprintf(stderr,"\t<startyear> simulations and new forcingdata");
    fprintf(stderr,"\t<endyear> simulations and new forcingdata\n");
    exit(0);
  }

  strcpy(soilfile,argv[1]);
  strcpy(extractfile,argv[2]);
  strcpy(irrfile,argv[3]);
  strcpy(metdir,argv[4]); 
  strcpy(outdir,argv[5]);
  strcpy(rundir,argv[6]);
  strcpy(routdir,argv[7]);
  basin=atof(argv[8]);
  latres=atof(argv[9]);
  reservoir=atoi(argv[10]);
  strcpy(upstreamfile,argv[11]);
  irrarea=atof(argv[12]);
  ForceYear=atoi(argv[13]);
  StartYear=atoi(argv[14]);
  EndYear=atoi(argv[15]);
  Nparam=OLD_PARAM;

  /* Calculate skipdays */
  skip = 0;
  for(i=ForceYear;i<StartYear;i++) {
   if(IsLeapYear(i)) skip+=366; 
   else skip+=365;
  }
  ndays=0;
  for(i=StartYear;i<=EndYear;i++) {
   if(IsLeapYear(i)) ndays+=366; 
   else ndays+=365;
  }

  MET = (float**)calloc(ndays,sizeof(float*));
  for(i=0;i<ndays;i++) 
    MET[i] = (float*)calloc((Nparam+1),sizeof(float));
  STREAMFLOW = (double**)calloc(ndays,sizeof(double*));
  for(i=0;i<ndays;i++) 
    STREAMFLOW[i] = (double*)calloc(4,sizeof(double));
  ROUTED = (double**)calloc(ndays,sizeof(float*));
  for(i=0;i<ndays;i++) 
    ROUTED[i] = (double*)calloc(4,sizeof(double));
  AVAILWATER =  (double*)calloc(ndays,sizeof(double));

  for(i=0;i<ndays;i++) 
    for(j=0;j<4;j++)    
   STREAMFLOW[i][j]=0.;

  ReadSoil(soilfile,&lat,&lon,&cell);
  printf("ReadSoil finished %s %f %f %d \n",soilfile,lat,lon,cell);
  ReadMetdata(metdir,MET,lat,lon,skip,ndays); //Reads current cell's original metdata file
  printf("ReadMetdata finished\n");
  ReadFracFile(frac_path,fraccells,basin,latres,lat,lon,&fracarea);   
  printf("RedFracFile finished %f\n",fracarea);

  // Read file which tells you from where to extract local runoff
  ReadStreamflow(rundir,upstreamfile,STREAMFLOW,basin,cell,lat,lon,ndays); //Reads streamflow from upstream cell(s), i.e. numbers in m3/s. 
  printf("ReadStreamflow finished\n");
  ReadIrr(irrfile,IRR); //one-line file for current cell which includes monthly irrigated area values 
  printf("ReadIrr finished\n");
  CalcArea(lat,latres,latres,&area);
  printf("CalcArea finished %f %f\n",area,irrarea);

  if(reservoir==1) //if reservoir routing should be included in simulations
    FindPointsToExtractWaterFrom(extractfile,LL,lat,lon,&count); //i.e. reservoirs
  else count=0;
  printf("Points found, reservoirs/count=%d\n",count);

  for(i=0;i<ndays;i++) AVAILWATER[i]=0.; //water from reservoir(s)

  for(i=0;i<count;i++) {
    ReadReservoirRouted(routdir,ROUTED,basin,cell,LL[i][0],LL[i][1],ndays); //Reads routed runoff 
                                                             //(m3/s) from reservoir(s)
    printf("ReadRouted finished %f %f\n",LL[i][0],LL[i][1]);
    for(j=0;j<ndays;j++) {
      if(ROUTED[j][3]>0.5) AVAILWATER[j]+=ROUTED[j][3]-0.49; //keep at least 0.01 m3s-1 in river. original: multiplied by capacity/totcapacity here.
      //printf("availwater %d %f\n",j,AVAILWATER[j]);
    }
  }

  /* Write upstream routed runoff + more distant available water to file */
  WriteData(outdir,MET,STREAMFLOW,AVAILWATER,IRR,lat,lon,area,ndays,Nparam); //Must take irrigated area into account here
  printf("WriteData finished %f %f %f\n",fracarea,area,irrarea);

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
/******************************************************************************/
/*				 ReadFracFile                                 */
/******************************************************************************/
void ReadFracFile(const char *frac_path,CELL **incells,int basin,float latres,
		      float lat,float lon,float *fracarea)
{
  FILE *fracfile = NULL;
  char filename[BUFSIZ+1];
  char dummy[25];
  int i;
  int j;
  int rows;
  int cols;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open input fraction file */
  sprintf(filename,"%s/%d.frac",frac_path,basin);
  fracfile = fopen(filename, "r");
  if (fracfile == NULL) {
   printf("Cannot open file %s\n",filename);exit(0);} 
  //else printf("Fraction file opened: %s\n",filename);

  /* Read header */
  fscanf(fracfile,"%s %d ",dummy,&cols);
  fscanf(fracfile,"%s %d ",dummy,&rows);
  fscanf(fracfile,"%s %f ",dummy,&west);
  fscanf(fracfile,"%s %f ",dummy,&south);
  fscanf(fracfile,"%s %f ",dummy,&cellsize);
  fscanf(fracfile,"%s %f ",dummy,&nodata);

  incells = calloc(rows, sizeof(CELLPTR));
  for (i = 0; i < rows; i++) 
    incells[i] = calloc(cols, sizeof(CELL));
  
  /* initialize all the cells */
  for (i = 0; i < rows; i++) {
    for (j = 0; j < cols; j++) {
      incells[i][j].lat = (south + rows*latres) - (i+.5)*latres;
      incells[i][j].lon = west + (j+.5)*latres;
      incells[i][j].row = rows-i;
      incells[i][j].col = j;
      incells[i][j].frac = 0;
    }
  }
  
  for (i = 0; i < rows; i++) {
    for (j = 0; j < cols; j++) {
      fscanf(fracfile,"%f ",&incells[i][j].frac);
      if((fabs(lat-incells[i][j].lat)<EPS) && 
	 (fabs(lon-incells[i][j].lon)<EPS)) 
	(*fracarea)=incells[i][j].frac;	
    }
  }
  fclose(fracfile);
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
    printf("Cannot open file %s \n",file);exit(0);}
  //else printf("\tFile opened: %s \n",file);

  for(i=0;i<14;i++) fscanf(fp,"%f ",&IRR[i]);
  fclose(fp);
}
/*************************************************************************/
/*                 ReadMetdata                                           */
/*************************************************************************/
void ReadMetdata(char indir[400], 
		 float **MET, 
		 float lat, 
		 float lon,
		 int skip,
		 int ndays)
{
  FILE *fp;
  int i;
  char LATLON[50];
  char file[400];
  unsigned short int utmp;
  signed short int stmp;
  float tmp1,tmp2,tmp3,tmp4;

  /* Make filename */
  strcpy(file,indir);
  sprintf(LATLON,"forcings_%.4f_%.4f",lat,lon);
  strcat(file,LATLON);
  if((fp = fopen(file,"r"))==NULL){ 
    printf("Cannot open file %s\n",file);exit(0);} 
  else printf("\tMetdata file opened: %s\n",file);

  for(i=0;i<skip;i++) { // 24 hr timestep,rainf,snowf,tmin,tmax,wind,sw,lw,qair,press,tair
    fscanf(fp,"%f %f %f %f",&tmp1,&tmp2,&tmp3,&tmp4);
    /*    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&stmp,sizeof(signed short int),1,fp);
    fread(&stmp,sizeof(signed short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&utmp,sizeof(unsigned short int),1,fp);
    fread(&stmp,sizeof(signed short int),1,fp);*/
  }

  for(i=0;i<ndays;i++) { //24 hr timestep,rainf,snowf,tmin,tmax,wind,sw,lw,qair,press,tair
    fscanf(fp,"%f %f %f %f",&MET[i][0],&MET[i][1],&MET[i][2],&MET[i][3]);
    /*fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][0]=(float)utmp/100.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][1]=(float)utmp/100.;
    fread(&stmp,sizeof(signed short int),1,fp);
    MET[i][2]=(float)stmp/100.;
    fread(&stmp,sizeof(signed short int),1,fp);
    MET[i][3]=(float)stmp/100.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][4]=(float)utmp/100.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][5]=(float)utmp/10.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][6]=(float)utmp/10.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][7]=(float)utmp/100000.;
    fread(&utmp,sizeof(unsigned short int),1,fp);
    MET[i][8]=(float)utmp/100.;
    fread(&stmp,sizeof(signed short int),1,fp);
    MET[i][9]=(float)stmp/100.;*/
  }
  
  fclose(fp); 
}
/*************************************************************************/
/*                 ReadReservoirRouted                                   */
/*************************************************************************/
void ReadReservoirRouted(char routdir[400],
			 double **ROUTED,
			 int basin,
			 int cell,
			 float latitude,
			 float longitude,
			 int ndays)
{
  FILE *fp;
  int i;
  char file[400];
  char LATLONG[150];

  /* Make filename */
  strcpy(file,routdir);
  sprintf(LATLONG,"streamflow_%.4f_%.4f",latitude,longitude);
  strcat(file,LATLONG);

  if((fp = fopen(file,"r"))==NULL){ 
    for(i=0;i<ndays;i++) 
      ROUTED[i][0]=ROUTED[i][1]=ROUTED[i][2]=ROUTED[i][3]=0.;
    printf("\tReadRouted file not found, %s, setting numbers to 0\n",file);
  } 
  else {
    printf("\tRouted file (i.e. from reservoir) opened for reading: %s \n",file);
    for(i=0;i<ndays;i++) 
      fscanf(fp,"%lf %lf %lf %lf",
	     &ROUTED[i][0],&ROUTED[i][1],&ROUTED[i][2],&ROUTED[i][3]); //year,month,day,streamflow
  fclose(fp); 
  }
}
/*************************************************************************/
/*                 ReadStreamflow                                        */
/*************************************************************************/
void ReadStreamflow(char rundir[400],
		    char upstreamfile[400],
		    double **STREAMFLOW,
		    int basin,
		    int cell,
		    float lat,
		    float lon,
		    int ndays)
{
  FILE *fp;
  FILE *fup;
  float LL[9][2];
  double value;
  int i,j,ncells;
  char file[400];
  char CELLNR[50];


  /* Open and read streamflow of upstream cells */
  if((fup = fopen(upstreamfile,"r"))==NULL) { 
      printf("Cannot open file %s \n",upstreamfile);exit(0);
  } 
  else printf("File opened %s \n",upstreamfile);

  fscanf(fup,"%*d %*d %*f %*f %*d %d",&ncells);

  for(i=0;i<ncells;i++) {
      fscanf(fup,"%*d %*d %f %f",&LL[i][0],&LL[i][1]);

      /* Make filename */
      strcpy(file,rundir);
      sprintf(CELLNR,"streamflow_%.4f_%.4f",LL[i][0],LL[i][1]);
      strcat(file,CELLNR);
      if((fp = fopen(file,"r"))==NULL){ 
	  printf("Cannot open file %s \n",file);exit(0);} 
      else printf("File opened: %s \n",file);
      
      for(j=0;j<ndays;j++) {
	  fscanf(fp,"%lf %lf %lf %lf",
		 &STREAMFLOW[j][0],&STREAMFLOW[j][1],&STREAMFLOW[j][2],&value);
	  if(value>0.5) STREAMFLOW[j][3]+=value-0.49; //i.e. total upstream streamflow in case of more than one upstream cell. Keep at least 0.01 m3s-1 in river
      }

      fclose(fp); 
  }      
  fclose(fup); 
}

/*************************************************************************/
/*                 ReadSoil                                              */
/*************************************************************************/
void ReadSoil(char file[400],
	      float *lat,
	      float *lon,
	      int *cell)
{
  FILE *fp;
  float dummy;

  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open file %s \n",file);exit(0);}

  fscanf(fp,"%f %d %f %f",&dummy,&(*cell),&(*lat),&(*lon));
  fclose(fp);
}
/*************************************************************************/
/*                 FindPointsToExtractWaterFrom                          */
/*************************************************************************/
void FindPointsToExtractWaterFrom(char file[400],
				  float LL[25][5],
				  float lat,
				  float lon,
				  int *count)
{
  FILE *fp;
  char LINE[1000];
  int i,nr,damnr,truea;
  float latitude,longitude,damlat,damlon,mean_inflow;
  float capacity,totcapacity;

  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open file %s \n",file);exit(0);}
  else printf("\tExtractwaterfile opened: %s \n",file);
  
  truea=0;
  totcapacity=0.; 

  while(!truea) {
    fscanf(fp,"%*d %f %f %d",&latitude,&longitude,&nr);
    (*count)=nr;
    if(fabs(latitude-lat)<=EPS && fabs(longitude-lon)<=EPS) {
      totcapacity=0.; 
      printf("\t FindPoints: %f %f %f %f %d\t",latitude,longitude,lat,lon,nr);
      for(i=0;i<nr;i++) { 
	fscanf(fp,"%d %f %f %f %f",
	       &damnr,&damlat,&damlon,&capacity,&mean_inflow);
	printf(" %d %f %f\t",damnr,damlat,damlon);
	LL[i][0]=damlat;
	LL[i][1]=damlon;
	LL[i][2]=capacity;
	LL[i][3]=mean_inflow;
	totcapacity+=capacity;
      }
      printf("\n");
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
void WriteData(char outdir[400],
	       float **MET,
	       double **STREAMFLOW,
	       double *AVAILWATER, 
	       float IRR[20],
	       float lat, 
	       float lon,
	       float area,
	       int ndays,
	       int Nparam)
{
  FILE *fp;
  int i,month,param,nstep;
  char LATLON[50];
  char outfile[400];

  strcpy(outfile,outdir);
  sprintf(LATLON,"forcings_%.4f_%.4f",lat,lon);
  strcat(outfile,LATLON);
  if((fp = fopen(outfile,"w"))==NULL)   
    {printf("Cannot open file %s \n",outfile);exit(0);}   
  else printf("\tWrite file opened: %s\n",outfile);

  for(i=0;i<ndays;i++) {
    for(param=0;param<Nparam;param++) 
      fprintf(fp,"%.4f\t",MET[i][param]);
    fprintf(fp,"%10.5f %10.5f\n",STREAMFLOW[i][3]*CONVFACTOR/area,AVAILWATER[i]*CONVFACTOR/area); //i.e in mm for entire cell in question
  }
  fclose(fp); 
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
/****************************/
