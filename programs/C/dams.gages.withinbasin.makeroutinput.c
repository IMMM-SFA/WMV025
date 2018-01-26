/*
 * Purpose: Find dams located within a basin, i.e. combine information in a file describing dams (here: global coverage), 
            and a file describing the basin. The basin is defined by it's direction file.
 * Usage  : dams.gages.withinbasin.makeroutinput <basin number>  <year dams built in or before> <input directionfile> <output reservoirinfofile> <output pointsfile used for sorting order of simulations (includes dam locations, gage locations and outlet location, upstream to downstream>  <output .sta file for VIC routing>
	    Dams are listed in file "*dam_filename"
              If several dams within a cell:  These will be combined into one dam, using some suspect rule.
              dam_file must have the following information in this order:
	      DamId DamName Year Height(m) Capacity(m3) ReservoirArea(m2) Purpose(s) InstCap(MW) MeanAnnEnergy(GWh) Longitude Latitude
              If several dams within a cell:  These will be combined into one dam, using some suspect rule.
	    row 1 is lowest row in direction file (arcinfo .asc type)
            col 1 is left column
            Gages (for WaterMIP) are listed in file  "gage_filename"
              gage_file: List including basin number, basin name, gage name, lat and lon for all "WaterMIP gage locations"
* Notes  : If in doubt, read the disclaimer.
* Disclaimer: Feel free to use or adapt any part of this program for your own
             convenience.  However, this only applies with the understanding
             that YOU ARE RESPONSIBLE TO ENSURE THAT THE PROGRAM DOES WHAT YOU
             THINK IT SHOULD DO.  The author of this program does not in any
             way, shape or form accept responsibility for problems caused by
             the use of this code.  At any time, please feel free to discard
             this code and WRITE YOUR OWN, it's what I would do.
 * Author : Ingjerd Haddeland
 * E-mail : iha@nve.no
 * Created: October 2004, revised 2009, 2010
 *
 * gcc -lm -Wall -o ../bin/dams.gages.withinbasin.makeroutinput dams.gages.withinbasin.makeroutinput.c
 
 gcc -lm -Wall /hdata/fou/iha/watch/programs/C/dams.gages.withinbasin.makeroutinput.c
 

$BinPath/dams.gages.withinbasin.makeroutinput $Basin $YearForDams $RoutingNetworkPath/dir30min_overlap/$Basin.dir  $RoutingPath/input/$Basin.reservoirs.firstline  $RoutingPath/input/$Basin.points $RoutingPath/input/$Basin.sta $DamFile $GageListFile


 */

/******************************************************************************/
/*			    PREPROCESSOR DIRECTIVES                           */
/******************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EPS 0.125		/* precision */
#define EMPTY_STR ""		/* empty string */
#define MISSING -999		/* missing value indicator */
#define ENOERROR 0		/* no error */
#define EUSAGE 1001		/* usage error */
#define ENODOUBLE 1002		/* not a double */
#define ENOFLOAT 1003		/* not a float */
#define ENOINT 1004		/* not an integer */
#define ENOLONG 1005		/* not a long */
#define ENOSHORT 1006		/* not a short */
#define ENOCASE 1007		/* not a valid case in a switch statement */
#define EBASIN 1008	        /* invalid basin */
#define ESCAN 1009		/* error in *scanf */
#define ELAT 1010		/* wrong latitude */
#define ELON 1011		/* wrong longitude */

/******************************************************************************/
/*			TYPE DEFINITIONS, GLOBALS, ETC.                       */
/******************************************************************************/

typedef enum {double_f, int_f, float_f, long_f, short_f} FORMAT_SPECIFIER;

typedef struct {
  int dam;
  int gage;
  int direction;
  int row;
  int col;
  float lat;
  float lon;
  int torow;
  int tocol;
  int localdir;
  int flag;
  int outskirt;
  int rank;
  int newrank;
  int point;
  int inside;
  int withdrawal;
  char damname[30];
} CELL;

typedef struct {
  int id;
  float lat;
  float lon;
  int year;
  float capacity;
  float area;
  char purpose[10];
  float catcharea;
  int instcap;
  int annenergy;
  int irrareas;
  int height;
  float height_fall;
  char damname[30];
  int order;
  int row;
  int col;
  int exist;
} LIST;

typedef struct {
  int id;
  float lat;
  float lon;
  char name[30];
  int row;
  int col;
  int exist;
} LIST2;

const float RES = 0.25;
const char *usage = 
"\nUsage:\n%s\t"
"\t[basin number]\t"
"\t[input soilfile]\t"
"\t[input (and output) direction file]\n\n";
int status = ENOERROR;
char message[BUFSIZ+1] = "";

/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
int GetNumber(char *str, int format, int start, int end, void *value);
int ProcessCommandLine(int argc,char **argv, int *basin,
		       int *year_analyze,char *dirfilename,char *origstafilename,
                       char *outfilename,char *pointsfilename,
		       char *stafilename,
		       char *dam_filename,char *gage_filename);
int ProcessError(void);
int FindRowsCols(char *dirfilename,int *rows,int *cols,
		 float *south,float *west,int *nodata);
int ReadDirFile(char *dirfilename,CELL **BASIN,
		const float RES);
int ReadDams(const char *dam_filename,int ndams,
		 LIST *DAM,int *reduction,int year_analyze);
int ReadGages(const char *gage_filename,int ngages,LIST2 *GAGE);
void ReadStation(int,char *,int *,int *);
int FindDams(int ndams,int ngages,LIST *DAM,LIST2 *GAGE,CELL **BASIN,const float RES,
	     int rows,int cols,char *outfilename,char *pointsfilename,
	     char *stafilename,int col,int row,
	     float outlet_lat,float outlet_lon,int nodata);
int LineCount(const char *);
int SetToMissing(int format, void *value);
/******************************************************************************/
/******************************************************************************/
/*				      MAIN                                    */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
  char dirfilename[BUFSIZ+1];
  char outfilename[BUFSIZ+1];
  char pointsfilename[BUFSIZ+1];
  char origstafilename[BUFSIZ+1];
  char stafilename[BUFSIZ+1];
  char dam_filename[BUFSIZ+1];
  char gage_filename[BUFSIZ+1];
  int rows,cols,i,j,ndams,ngages,col,row,basin,reduction,year_analyze,nodata;
  float south,west,outlet_lat,outlet_lon;
  CELL **BASIN = NULL;
  LIST *DAM = NULL;
  LIST2 *GAGE = NULL;

  status = ProcessCommandLine(argc,argv,&basin,&year_analyze,dirfilename,origstafilename,outfilename,
			      pointsfilename,stafilename,dam_filename,gage_filename);
  if (status != ENOERROR)
    goto error;

  status = FindRowsCols(dirfilename,&rows,&cols,&south,&west,&nodata);
  if (status != ENOERROR)
    goto error;
  //printf("RowsCols found, rows=%d cols=%d\n",rows,cols);
 
  ReadStation(basin,origstafilename,&col,&row); //outlet station file
  outlet_lat=south+row*RES-RES/2;
  outlet_lon=west+col*RES-RES/2;

  /* Initialize BASIN */
  BASIN = calloc((rows+2), sizeof(CELL));
  if (BASIN == NULL) {
    status = errno;
    sprintf(message, "%s: %d", __FILE__, __LINE__);
    goto error;
  }
  for (i = 0; i <= rows+1; i++) {
    BASIN[i] = calloc((cols+2), sizeof(CELL));
    if (BASIN[i] == NULL) {
      status = errno;
      sprintf(message, "%s: %d", __FILE__, __LINE__);
      goto error;
    }
  }
  
  for (i = 0; i <= rows+1; i++) {
    for (j = 0; j <= cols+1; j++) {
      BASIN[i][j].lat = south + (i-.5)*RES;
      BASIN[i][j].lon = west + (j-.5)*RES;
      BASIN[i][j].row = i;
      BASIN[i][j].col = j;
      BASIN[i][j].direction = 0;
      BASIN[i][j].gage = MISSING;     
      BASIN[i][j].dam = MISSING;     
      BASIN[i][j].rank = MISSING;
      BASIN[i][j].point = MISSING;
      BASIN[i][j].torow = MISSING;    
      BASIN[i][j].tocol = MISSING;    
      BASIN[i][j].inside = MISSING;    
      BASIN[i][j].withdrawal = MISSING;  
    }
  }

  /* Read basin's direction file */
  status = ReadDirFile(dirfilename,BASIN,RES);
  if (status != ENOERROR)
    goto error;
  //printf("Dirfile read\n");

  /* Initialize and read dam info file (UNH) */
  ndams = LineCount(dam_filename);
  ngages =  LineCount(gage_filename);
  printf("Lines in damfile: %d and gageinfofile: %d\n",ndams,ngages);

  DAM=calloc(ndams+1,sizeof(LIST));
  GAGE=calloc(ngages+1,sizeof(LIST2));

  status = ReadDams(dam_filename,ndams,DAM,&reduction,year_analyze);
  if (status != ENOERROR)
    goto error;
  //printf("Dams read\n");

  status = ReadGages(gage_filename,ngages,GAGE);
  if (status != ENOERROR)
    goto error;
  /* Find cells with dam and/or gage within basin, their 'order within the basin', 
     and print to outfiles */
  status = FindDams(ndams,ngages,DAM,GAGE,BASIN,RES,rows,cols,
		    outfilename,pointsfilename,
		    stafilename,col,row,
		    outlet_lat,outlet_lon,nodata);
  if (status != ENOERROR)
    goto error;
  //printf("Dam and/or gage cells found\n");

  return EXIT_SUCCESS;

 error:
  ProcessError();
  exit(EXIT_FAILURE);
} 

/******************************************************************************/
/*				   GetNumber                                  */
/******************************************************************************/
int GetNumber(char *str, int format, int start, int end, void *value)
{
  char valstr[BUFSIZ+1];
  int i;
  int nchar;
  char *endptr = NULL;
  
  nchar = end-start+1;
  strncpy(valstr, &(str[start]), nchar);
  valstr[nchar] = '\0';
  
  for (i = 0; i < nchar; i++) {
    if (!isspace(valstr[i]))
      break;
  }
  if (i == nchar) {
    status = SetToMissing(format, value);
    if (status != ENOERROR)
      goto error;
    return ENOERROR;
  }
  
  switch (format) {
  case double_f:
    *((double *)value) = strtod(valstr, &endptr);
    break;
  case float_f:
    *((float *)value) = (float) strtod(valstr, &endptr);
    break;
  case int_f:
    *((int *)value) = (int) strtol(valstr, &endptr, 0);
    break;
  case long_f:
    *((long *)value) = (int) strtol(valstr, &endptr, 0);
    break;
  case short_f:
    *((short *)value) = (short) strtol(valstr, &endptr, 0);
    break;
  default:
    strcpy(message, "Unknown number format");
    status = ENOCASE;
    goto error;
    break;
  }
  
  if (!(isspace(*endptr) || *endptr == '\0' || endptr == NULL) ) {
    /* check if the string consists entirely of whitespace.  If so, this is a
       missing value */
    strcpy(message, valstr);
    switch (format) {
    case double_f:
      status = ENODOUBLE;
      break;
    case float_f:
      status = ENOFLOAT;
      break;
    case int_f:
      status = ENOINT;
      break;
    case long_f:
      status = ENOLONG;
      break;
    case short_f:
      status = ENOSHORT;
      break;
    default:
      strcpy(message, "Unknown number format");
      status = ENOCASE;
      break;
    }
    goto error;
  }
  
  return ENOERROR;

 error:
  return status;
}

/******************************************************************************/
/*			       ProcessCommandLine                             */
/******************************************************************************/
int ProcessCommandLine(int argc,char **argv,int *basin,int *year_analyze,
		       char *dirfilename,char *origstafilename,
                       char *outfilename,char *pointsfilename,
		       char *stafilename,
		       char *dam_filename,char *gage_filename)
{
  if (argc != 10) {
    status = EUSAGE;
    strcpy(message, argv[0]);
    goto error;
  }
  
  (*basin)=atoi(argv[1]);
  (*year_analyze)=atoi(argv[2]);
  strcpy(dirfilename,argv[3]);
  strcpy(origstafilename,argv[4]);
  strcpy(outfilename,argv[5]);
  strcpy(pointsfilename,argv[6]);
  strcpy(stafilename,argv[7]);
  strcpy(dam_filename,argv[8]);
  strcpy(gage_filename,argv[9]);
  return EXIT_SUCCESS;
  
 error:
    return status;
}

/******************************************************************************/
/*				  Processerror                                */
/******************************************************************************/
int ProcessError(void)
{
  if (errno) 
    perror(message);
  else {
    switch (status) {
    case EUSAGE:
      fprintf(stderr, usage, message);
      break;
    case ENODOUBLE:
      fprintf(stderr, "\nNot a valid double: %s\n\n", message);
      break;
    case ENOFLOAT:
      fprintf(stderr, "\nNot a valid float:\n%s\n\n", message);
      break;      
    case ENOINT:
      fprintf(stderr, "\nNot a valid integer:\n%s\n\n", message);
      break;      
    case ENOLONG:
      fprintf(stderr, "\nNot a valid long: %s\n\n", message);
      break;
    case ENOSHORT:
      fprintf(stderr, "\nNot a valid short: %s\n\n", message);
      break;
    case EBASIN:
      fprintf(stderr, "\nBasin does not exist: %s\n\n",message);
      break;      
    case ESCAN:
      fprintf(stderr, "\nError scanning file: %s\n\n", message);
      break;
    default:
      fprintf(stderr, "\nError: %s\n\n", message);
      break;
    }
  }
  status = ENOERROR;
  return status;
}
/******************************************************************************/
/*				 FindRowsCols                                 */
/******************************************************************************/
int FindRowsCols(char *dirfilename,int *rows,int *cols,
		 float *south,float *west,int *nodata)
{
  FILE *dirfile = NULL;
  char filename[BUFSIZ+1];
  char dummy[25];
  float cellsize;

  /* Open input direction file */
  sprintf(filename,"%s",dirfilename);
  dirfile = fopen(filename, "r");
  if (dirfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  else printf("\nDirection file opened (dams.withinbasin): %s\n",filename);

  /* Read header */
  fscanf(dirfile,"%s %d ",dummy,&(*cols));
  fscanf(dirfile,"%s %d ",dummy,&(*rows));
  fscanf(dirfile,"%s %f ",dummy,&(*west));
  fscanf(dirfile,"%s %f ",dummy,&(*south));
  fscanf(dirfile,"%s %f ",dummy,&cellsize);
  fscanf(dirfile,"%s %d ",dummy,&(*nodata));

  fclose(dirfile);

  return ENOERROR;
 error:
  if (dirfile != NULL)
    fclose(dirfile);
  return status;
}
/******************************************************************************/
/*				 ReadDirFile                                  */
/******************************************************************************/
int ReadDirFile(char *dirfilename,CELL **BASIN,
		const float RES)
{
  FILE *dirfile = NULL;
  char filename[BUFSIZ+1];
  char dummy[25];
  int i;
  int j;
  int rows,cols;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open input direction file */
  sprintf(filename,"%s",dirfilename);
  dirfile = fopen(filename, "r");
  if (dirfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  //  else printf("Direction file opened: %s\n",filename);

  /* Read header */
  fscanf(dirfile,"%s %d ",dummy,&cols);
  fscanf(dirfile,"%s %d ",dummy,&rows);
  fscanf(dirfile,"%s %f ",dummy,&west);
  fscanf(dirfile,"%s %f ",dummy,&south);
  fscanf(dirfile,"%s %f ",dummy,&cellsize);
  fscanf(dirfile,"%s %f ",dummy,&nodata);

  for (i = rows; i >= 1; i--) {
    for (j = 1; j <= cols; j++) {
       fscanf(dirfile,"%d ",&BASIN[i][j].direction);
       if(BASIN[i][j].direction<=nodata) BASIN[i][j].flag=MISSING;
       else BASIN[i][j].flag=0;      
    }
  }
  fclose(dirfile);

  for (i = 1; i <= rows; i++) {
    for (j = 1; j <= cols; j++) {
      if(BASIN[i][j].direction==0 || BASIN[i][j].direction==nodata) {
	BASIN[i][j].tocol=0;
	BASIN[i][j].torow=0;
      } 
      else if(BASIN[i][j].direction==1) {
         BASIN[i][j].tocol=j;
         BASIN[i][j].torow=i+1;
      } 
      else if(BASIN[i][j].direction==2) {
         BASIN[i][j].tocol=j+1;
         BASIN[i][j].torow=i+1;
      } 
      else if(BASIN[i][j].direction==3) {
         BASIN[i][j].tocol=j+1;
         BASIN[i][j].torow=i;
      } 
      else if(BASIN[i][j].direction==4) {
         BASIN[i][j].tocol=j+1;
         BASIN[i][j].torow=i-1;
      } 
      else if(BASIN[i][j].direction==5) {
         BASIN[i][j].tocol=j;
         BASIN[i][j].torow=i-1;
      } 
      else if(BASIN[i][j].direction==6) {
         BASIN[i][j].tocol=j-1;
         BASIN[i][j].torow=i-1;
      } 
      else if(BASIN[i][j].direction==7) {
         BASIN[i][j].tocol=j-1;
         BASIN[i][j].torow=i;
      } 
      else if(BASIN[i][j].direction==8) {
         BASIN[i][j].tocol=j-1;
         BASIN[i][j].torow=i+1;
      } 
    }
  }

  return ENOERROR;
 error:
  if (dirfile != NULL)
    fclose(dirfile);
  return status;
}
/******************************************************************************/
/*				 ReadDams                                     */
/******************************************************************************/
int ReadDams(const char *dam_file,int ndams,
	     LIST *DAM,int *reduction, int year_analyze)
{
  FILE *damfile = NULL;
  char filename[BUFSIZ+1];
  int i,j;

  /* Open dam information file */
  sprintf(filename,"%s",dam_file);
  damfile = fopen(filename, "r");
  if (damfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  else printf("damfile opened: %s\n",dam_file);

  for(i=0;i<ndams;i++) {
	DAM[i].id=0;
	DAM[i].lat=0;
	DAM[i].lon=0;
	DAM[i].year=0;
	DAM[i].capacity=0;
	DAM[i].area=0;
	DAM[i].catcharea=0;
	DAM[i].instcap=0;
	DAM[i].annenergy=0;
	DAM[i].irrareas=0;
	DAM[i].height=0;
	DAM[i].order=0;	
	DAM[i].col=0;	
	DAM[i].row=0;
	DAM[i].exist=1;
  }

  /* Read damfile. Exclude those built after year_analyze */
  //fgets(dummy,100,damfile); //header
  //printf("dummy %s\n",dummy);
  j=0;
  for (i = 0; i < ndams; i++) {
    fscanf(damfile,"%d %s %d %d %f %f %s %d %d %f %f",
	   &DAM[j].id,DAM[j].damname,&DAM[j].year,&DAM[j].height,&DAM[j].capacity,&DAM[j].area,DAM[j].purpose,&DAM[j].instcap,&DAM[j].annenergy,&DAM[j].lon,&DAM[j].lat);
    DAM[j].row=(int)((DAM[j].lat-(-90))/RES);
    DAM[j].col=(int)((DAM[j].lon-(-180))/RES);
    if(DAM[j].year<=year_analyze) j+=1; /* i.e. if year=0: include */
  }
  fclose(damfile);

  printf("readdams: dams built before or in year of analysis: %d\n",j);

  /* Go through file and figure out which dams will be located
     in same row/col in basin file at current resolution. Merge
  these dams into one dam.*/
  for (i = 0; i < ndams; i++) {
    for (j = i+1; j < ndams; j++) {
       if(DAM[i].row==DAM[j].row && DAM[i].col==DAM[j].col) {

	DAM[i].capacity+=DAM[j].capacity;
	if(DAM[j].height>DAM[i].height) 
	  DAM[i].height=DAM[j].height;
	DAM[j].exist=0;
       }
    }
  }


  j=0;
  for (i = 0; i < ndams; i++) {
    if(DAM[i].exist==1) j+=1;
  }

  printf("readdams: exclusive dams: %d\n",j);
  (*reduction)=ndams-j;

  return ENOERROR;
 error:
  if (damfile != NULL)
    fclose(damfile);
  return status;
}
/*********************************/
/* Reads the station file         */
/*********************************/
void ReadStation(int basin, 
		 char *origstafilename,
		 int *col,
		 int *row)
{
  FILE *fp;
  char filename[100];

  sprintf(filename,"%s",origstafilename);
  if((fp = fopen(filename, "r")) == NULL) {
    printf("Cannot open %s\n",filename);
    exit(1);
  }
  else printf("File opened: %s\n",filename); 

  fscanf(fp,"%*d %*d %*s %d %d",&(*col),&(*row));

  fclose(fp);

}
/******************************************************************************/
/*				 ReadGages                                     */
/******************************************************************************/
int ReadGages(const char *gage_file,int ngages,LIST2 *GAGE)
{
  FILE *fp = NULL;
  char filename[BUFSIZ+1];
  int i;

  /* Open dam information file */
  sprintf(filename,"%s",gage_file);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  else printf("gagefile opened: %s\n",gage_file);

  for(i=0;i<ngages;i++) {
	GAGE[i].id=0;
	GAGE[i].lat=0;
	GAGE[i].lon=0;
	GAGE[i].row=0;
	GAGE[i].col=0;
	GAGE[i].exist=1;
  }

  /* Read gagefile. */
  for (i = 0; i < ngages; i++) {
    fscanf(fp,"%d %*s %s %f %f",&GAGE[i].id,GAGE[i].name,&GAGE[i].lat,&GAGE[i].lon);
    GAGE[i].row=(int)((GAGE[i].lat-(-90))/RES);
    GAGE[i].col=(int)((GAGE[i].lon-(-180))/RES);
  }
  fclose(fp);

  return ENOERROR;
 error:
  if (fp != NULL)
    fclose(fp);
  return status;
}
/******************************************************************************/
/*				 FindDams                                     */
/******************************************************************************/
int FindDams(int ndams,int ngages,LIST *DAM,LIST2 *GAGE,CELL **BASIN,const float RES,
	     int rows,int cols,char *outfilename,char *pointsfilename,
	     char *stafilename,int col,int row,
	     float outlet_lat,float outlet_lon,int nodata)
{
  FILE *outfile = NULL;
  FILE *outfile2 = NULL;
  FILE *outfile3 = NULL;
  char filename[BUFSIZ+1];
  char filename2[BUFSIZ+1];
  char name[5];
  int i,j,k,m,n;
  int count=0;
  int dams_in_basin=0;
  int gages_in_basin=0;

  /* Open outfile */
  sprintf(filename,"%s",outfilename);
  outfile = fopen(filename, "w");
  if (outfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  else printf("FindDams Outfile opened (VIC routing expects *.reservoirs.firstline): %s\n",outfilename);

  /* Find dams within basin. Merge if two at same row/col location */
  for (i = 1; i <= rows; i++) {
    for (j = 1; j <= cols; j++) {
      if(BASIN[i][j].direction>nodata) {
	for(k=0;k<ndams;k++) {
	  if(DAM[k].lat>=(BASIN[i][j].lat-EPS) && DAM[k].lat<(BASIN[i][j].lat+EPS)
	     && DAM[k].lon>=(BASIN[i][j].lon-EPS) && DAM[k].lon<(BASIN[i][j].lon+EPS)
             && DAM[k].exist==1) {
	    BASIN[i][j].dam=DAM[k].id;
	    strcpy(BASIN[i][j].damname,DAM[k].damname);
	    dams_in_basin+=1;
	    if(DAM[k].height_fall>0) DAM[k].height=(int)DAM[k].height_fall;
	    fprintf(outfile,"%d %d %.4f %.4f %d %s \t",
		    BASIN[i][j].col,BASIN[i][j].row, BASIN[i][j].lon,BASIN[i][j].lat,
		    DAM[k].id,DAM[k].damname);		   
	    fprintf(outfile,"%d %.1f %.1f %.1f ",
		    DAM[k].year,DAM[k].capacity,DAM[k].area,DAM[k].catcharea);
	    fprintf(outfile,"%d %d %d %d %s\n",
		    DAM[k].instcap,DAM[k].annenergy,
		    DAM[k].irrareas,DAM[k].height,DAM[k].purpose);
	  }
	}
	for(k=0;k<ngages;k++) {
	  if(GAGE[k].lat>=(BASIN[i][j].lat-EPS) && GAGE[k].lat<(BASIN[i][j].lat+EPS)
	     && GAGE[k].lon>=(BASIN[i][j].lon-EPS) && GAGE[k].lon<(BASIN[i][j].lon+EPS)
             && GAGE[k].exist==1) {
	    if(BASIN[i][j].dam==MISSING) {
	      gages_in_basin+=1;
	      strcpy(BASIN[i][j].damname,GAGE[k].name);	      
	      BASIN[i][j].dam=GAGE[k].id;
	    }
	  }
	}
      }
    }
  }

  fclose(outfile);
  printf("Dams within basin: %d\n",dams_in_basin);

  /* Rank the dams and gages based on location in basin. Upstream cells get low rank numbers */
  if((dams_in_basin+gages_in_basin)>0) {
    do {
      for(i=1;i<=rows;i++) {
	for(j=1;j<=cols;j++) {
	  if(BASIN[i][j].flag!=MISSING) {
	    if(BASIN[i+1][j].direction==5 || BASIN[i+1][j].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i+1][j+1].direction==6 || BASIN[i+1][j+1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i][j+1].direction==7 || BASIN[i][j+1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i-1][j+1].direction==8 || BASIN[i-1][j+1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i-1][j].direction==1 || BASIN[i-1][j].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i-1][j-1].direction==2 || BASIN[i-1][j-1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i][j-1].direction==3 || BASIN[i][j-1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i+1][j-1].direction==4 || BASIN[i+1][j-1].direction==9) BASIN[i][j].flag=1;
	    if(BASIN[i][j].flag==0 && ( BASIN[i][j].dam!=MISSING || BASIN[i][j].gage!=MISSING )) {
	      count++;
	      BASIN[i][j].rank=count;
	    }
	  }
	}
      }
      for(m=0;m<rows+1;m++) { // Reset cells
	for(n=0;n<cols+1;n++) {
	  if(BASIN[m][n].flag==0) {
	    BASIN[m][n].flag=MISSING;
	    BASIN[m][n].direction=MISSING;
	  }
	  if(BASIN[m][n].flag!=MISSING) BASIN[m][n].flag=0;
	}
      }
    }
      while(count<(dams_in_basin+gages_in_basin));
  }

  /* Open outfile2 - pointfile, location of dam and outlet cells */
  sprintf(filename2,"%s",pointsfilename);
  outfile2 = fopen(filename2, "w");
  if (outfile2 == NULL) {
    status = errno;
    strcpy(message, filename2);
    goto error;
  }
  else printf("FindDams Pointsfile opened (VIC routing expects *.points) for writing: %s\n",pointsfilename);

  outfile3 = fopen(stafilename, "w");
  if (outfile3 == NULL) {
    status = errno;
    strcpy(message, stafilename);
    goto error;
  }
  else printf("FindDams stafile opened for writing (VIC routing expects *.sta): %s\n",stafilename);

  for(count=1;count<=(dams_in_basin+gages_in_basin);count++) {
    for (i = 1; i <= rows; i++) {
      for (j = 1; j <= cols; j++) {
	  if(BASIN[i][j].rank==count) {
	      strncpy(name,BASIN[i][j].damname,6);
	      name[5]='\0';
	      fprintf(outfile2,"%d %d\t %.4f %.4f %s\t %d\n",
		      BASIN[i][j].col,BASIN[i][j].row,BASIN[i][j].lon,
		      BASIN[i][j].lat,BASIN[i][j].damname,count);	
	      fprintf(outfile3,"1 0\t %s %d %d -999 2\n",
		      name,BASIN[i][j].col,BASIN[i][j].row);	
	      fprintf(outfile3,"NONE\n");
	  }
      }
    }
  }
  /*fprintf(outfile2,"%d %d\t %.4f %.4f Outlet\t %d\n",
	  col,row,outlet_lon,outlet_lat,count);
  fprintf(outfile3,"1 0\t outle %d %d %d %d\n",
	  col,row,-999,1);
  fprintf(outfile3,"NONE\n");    commented by Tian May 2013*/ 

  fclose(outfile2);
  fclose(outfile3);

  return ENOERROR;

 error:
  if (outfile != NULL)
    fclose(outfile);
  return status;
}
/******************************************************************************/
/*				  SetToMissing                                */
/******************************************************************************/
int SetToMissing(int format, void *value)
{
  switch (format) {
  case double_f:
    *((double *)value) = MISSING;
    break;
  case int_f:
    *((int *)value) = MISSING;
    break;
  case float_f:
    *((float *)value) = MISSING;
    break;
  case long_f:
    *((long *)value) = MISSING;
    break;
  case short_f:
    *((short *)value) = MISSING;
    break;
  default:
    strcpy(message, "Unknown number format");
    status = ENOCASE;
    goto error;
    break;
  }    
  return ENOERROR;
  
 error:
  return status;
}
/********************************************/
/* Function returns number of lines in file */
/********************************************/
int LineCount(const char *file)
{
  FILE *fp;
  int c, lines;

  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open file %s, exiting \n",file);exit(0);}

  lines = 0;

  while((c = fgetc(fp)) !=EOF)  if (c=='\n') lines++;
  fclose(fp);

  return lines;
}/* END function int line_count(char *file)   */
