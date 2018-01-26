/*
 * Purpose: Modify stationfile for a basin so runoff upstream current 
            cell is routed. 
 * Usage  : routing.modify.infiles <basin number> <soilfile (w/one cell)> 
                                   <new station_file>
 * Author : Ingjerd Haddeland
 * E-mail : iha@nve.no
 * Created: January 2009
 * Last Changed: 
 * Notes  : If in doubt, read the disclaimer.
 *          
 *          
 * Disclaimer: Feel free to use or adapt any part of this program for your own
 *             convenience.  However, this only applies with the understanding
 *             that YOU ARE RESPONSIBLE TO ENSURE THAT THE PROGRAM DOES 
 *             WHAT YOU
 *             THINK IT SHOULD DO.  The author of this program does not in any
 *             way, shape or form accept responsibility for problems caused by
 *             the use of this code.  
 *
 * gcc -lm -Wall -o ../bin/routing.modifystationfile routing.modifystationfile.c

 * gcc -lm -Wall ~/watch/programs/C/routing.modifystationfile.c

run from run/rout/

eks:

./a.out 29646 ../input/soil/points/10373 test.tmp ../upstreamcells.txt $Reservoirs $ReservoirFileName 


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

#define EPS 1e-7		/* precision */
#define EMPTY_STR ""		/* empty string */
#define MISSING -999		/* missing value indicator */
#define SOILCOLS 54             /* number of cols in soilfile */
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

typedef struct _CELL_ *CELLPTR;
typedef struct _CELL_ {
  int id;
  int row;
  int col;
  int flag;
  int flag_reservoir;
  int rank;
  int upstream[8];
  float lat;
  float lon;
  float area;
} CELL;


const char *dirh_path = "./rout/input";
const char *frac_path = "./rout/input";
const float RES = 0.25;

const char *usage = 
"\nUsage:\n%s\t"
"\t[basin number]\t"
"\t[input soilfile]\t"
"\t[input (and output) station file]\t"
"\t[input (and output) fraction file]\n\n";
int status = ENOERROR;
char message[BUFSIZ+1] = "";

/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
int GetNumber(char *str, int format, int start, int end, void *value);
int ProcessCommandLine(int argc,char **argv,int *basin,
		       char *infilename,char *outfilename,
		       char *upstreamcellsfilename,
                       int *reservoirs_included,
                       char *reservoirfilename);
int ProcessError(void);
int ReadStaFileAndWriteOutput(char *infilename,char *outfilename,char *,
			      CELL **incells,int nrows,int ncols,int basin,
			      int reservoirs_included);
int ReadDirhFile(const char *dirh_path, int basin, const float RES, CELL ***incells,
		 int *nrows,int *ncols);
int ReadReservoirFile(const char *dirh_path,char *reservoirfilename,
		      CELL **incells,int nrows,int ncols);
int ReadUpstream(char *,CELL **,int,int,int *,int *,int *);
int SetToMissing(int format, void *value);
int LineCount(char *);
/******************************************************************************/
/******************************************************************************/
/*				      MAIN                                    */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
  char infilename[BUFSIZ+1];
  char outfilename[BUFSIZ+1];
  char upstreamcellsfilename[BUFSIZ+1];
  char reservoirfilename[BUFSIZ+1];
  int basin;
  int nrows;
  int ncols;
  int reservoirs_included;
  CELL **incells = NULL;

  status = ProcessCommandLine(argc,argv,&basin,infilename,outfilename,
			      upstreamcellsfilename,&reservoirs_included,
			      reservoirfilename);
  if (status != ENOERROR)
    goto error;
  
  status = ReadDirhFile(dirh_path,basin,RES,&incells,&nrows,&ncols);
  if (status != ENOERROR)
    goto error;

  status = ReadReservoirFile(dirh_path,reservoirfilename,incells,nrows,ncols);
  if (status != ENOERROR)
    goto error;

  status = ReadStaFileAndWriteOutput(infilename,outfilename,upstreamcellsfilename,
				     incells,nrows,ncols,basin,reservoirs_included);

  if (status != ENOERROR)
    goto error;

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
int ProcessCommandLine(int argc,char **argv,int *basin,
		       char *infilename,char *outfilename,
		       char *upstreamcellsfilename,
                       int *flag_reservoirs,
                       char *reservoirfilename)
{
  if (argc != 7) {
    status = EUSAGE;
    strcpy(message, argv[0]);
    goto error;
  }
  
  status = GetNumber(argv[1], int_f, 0, strlen(argv[1]), basin);
  if (status != ENOERROR)
    goto error;
  if (*basin == 0) {
    status = EBASIN;
    strcpy(message, argv[1]);
    goto error;
  }
 
  strcpy(infilename, argv[2]);
  strcpy(outfilename, argv[3]);
  strcpy(upstreamcellsfilename, argv[4]);
  (*flag_reservoirs)=atoi(argv[5]);
  strcpy(reservoirfilename, argv[6]);

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
/*			       ReadStaFileAndWriteOutput                      */
/******************************************************************************/
int ReadStaFileAndWriteOutput(char *infilename,char *outfilename,
			      char *upstreamcellsfilename,CELL **incells,
			      int nrows,int ncols,int basin,int reservoirs_included)
{
  FILE *outfile = NULL;
  FILE *infile = NULL;
  float **soilcells;
  int i;
  int j;
  int k;
  int cell;
  int rownumber;
  int colnumber;
  int irow,icol;
  int upstream;
  int upstreamcol[8];
  int upstreamrow[8];

  infile = fopen(infilename, "r");
  if (infile == NULL) {
    status = errno;
    strcpy(message, infilename);
    goto error;
  }
  else printf("Infile opened: %s\n",infilename);

  /* Open output file */
  outfile = fopen(outfilename, "w");
  if (outfile == NULL) {
    status = errno;
    strcpy(message, outfilename);
    goto error;
  }
  else printf("Outfile opened: %s\n",outfilename);

  /* Allocate memory. Only one cell */
  soilcells = calloc(1, sizeof(float));
  if (soilcells == NULL) {
    status = errno;
    sprintf(message, "%s: %d", __FILE__, __LINE__);
    goto error;
  }
  for (i = 0; i < 1; i++) {
    soilcells[i] = calloc(SOILCOLS, sizeof(float));
    if (soilcells[i] == NULL) {
      status = errno;
      sprintf(message, "%s: %d", __FILE__, __LINE__);
      goto error;
    }
  }
 
  /* Read infile (soil) */
  for(i=0;i<1;i++) {
    for(j=0;j<SOILCOLS;j++) {
      fscanf(infile,"%f ",&soilcells[i][j]);
    }
  }
  fclose(infile);

  /* Find appropriate row/col for current cell. */
  for (i = 1; i <= nrows; i++) {
    for (j = 1; j <= ncols; j++) {
      if((fabs(soilcells[0][2]-incells[i][j].lat)<EPS) && 
	 (fabs(soilcells[0][3]-incells[i][j].lon)<EPS)) {
	rownumber=incells[i][j].row;
	colnumber=incells[i][j].col;
	cell=soilcells[0][1];
      }
    }
  }

  printf("%d %d %f %f\n",rownumber,colnumber,soilcells[0][2],soilcells[0][3]);
  status = ReadUpstream(upstreamcellsfilename,incells,rownumber,colnumber,
		       &upstream,upstreamrow,upstreamcol);
 if (status != ENOERROR)
    goto error;

 printf("upstream current location:%d\n",upstream);

  /* Write station file */
 for(i=0;i<upstream;i++) {
   if(reservoirs_included==0) {
     fprintf(outfile,"1 0 test  %d %d %d 1\n",upstreamcol[i],upstreamrow[i],-999);
     fprintf(outfile,"NONE\n");
   }
   else { /*reservoirs included*/
     irow=upstreamrow[i];
     icol=upstreamcol[i];
     for(j=0;j<(nrows+2);j++) {
       for(k=0;k<(ncols+2);k++) {
	 if(incells[j][k].row==irow && incells[j][k].col==icol && incells[j][k].flag_reservoir==0) {
	   fprintf(outfile,"1 0 test  %d %d %d 1\n",upstreamcol[i],upstreamrow[i],-999);
	   fprintf(outfile,"NONE\n");
	 }
       }
     }
   }
 }

 fclose(outfile);

 return ENOERROR;

 error:
 return status;
}

/******************************************************************************/
/*				 ReadDirhFile                                 */
/******************************************************************************/
int ReadDirhFile(const char *dirh_path, int basin, const float RES, CELL ***incells,
		 int *nrows,int *ncols)
{
  FILE *dirhfile = NULL;
  char filename[BUFSIZ+1];
  char dummy[25];
  int i,j,k;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open direction file */
  sprintf(filename,"%s/%d.dir",dirh_path,basin);
  dirhfile = fopen(filename, "r");
  if (dirhfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  else printf("Direction file opened: %s\n",filename);

  /* Read header */
  fscanf(dirhfile,"%s %d ",dummy,&(*ncols));
  fscanf(dirhfile,"%s %d ",dummy,&(*nrows));
  fscanf(dirhfile,"%s %f ",dummy,&west);
  fscanf(dirhfile,"%s %f ",dummy,&south);
  fscanf(dirhfile,"%s %f ",dummy,&cellsize);
  fscanf(dirhfile,"%s %f ",dummy,&nodata);

  *incells = calloc((*nrows)+2, sizeof(CELLPTR));
  if (*incells == NULL) {
    status = errno;
    sprintf(message, "%s: %d", __FILE__, __LINE__);
    goto error;
  }
  for (i = 0; i < (*nrows)+2; i++) {
    (*incells)[i] = calloc((*ncols)+2, sizeof(CELL));
    if ((*incells)[i] == NULL) {
      status = errno;
      sprintf(message, "%s: %d", __FILE__, __LINE__);
      goto error;
    }
  }
  
  /* initialize all the cells */
  for (i = 0; i < (*nrows)+2; i++) {
    for (j = 0; j < (*ncols)+2; j++) {
      (*incells)[i][j].lat = (south + (*nrows)*RES) - (i-.5)*RES;
      (*incells)[i][j].lon = west + (j-.5)*RES;
      (*incells)[i][j].row = (*nrows)+1-i;
      (*incells)[i][j].col = j;
      (*incells)[i][j].flag_reservoir = 0;
      for(k=0;k<8;k++) (*incells)[i][j].upstream[k]=0;
      //if( i==10 && j==9 ) printf("readdirh initialize: %d %d %f %f\n",i,j,
//				 (*incells)[i][j].lat,(*incells)[i][j].lon);
    }
  }
  
  fclose(dirhfile);

  return ENOERROR;
 error:
  if (dirhfile != NULL)
    fclose(dirhfile);
  return status;
}
/******************************************************************************/
/*				 ReadUpstream                                 */
/******************************************************************************/
int ReadUpstream(char *filename,CELL **incells,int irow,int icol,int *upstream,
		 int upstreamrow[8],int upstreamcol[8])
{
  FILE *infile = NULL;
  float dummy;
  int i;
  int row,col;
  int direction;
  int nloc;

  /* Open upstreamfilename */
  infile = fopen(filename, "r");
  if (infile == NULL) {
      status = errno;
      strcpy(message, filename);
      goto error;
  }
  else printf("Upstream file opened: %s\n",filename);
  
  for(i=0;i<8;i++) {
      upstreamrow[i]=0;
      upstreamcol[i]=0;
  }
  
  (*upstream)=0;
  printf("irow %d icol %d\n",irow,icol);
 
  /* Read file */
  while ((fscanf(infile, "%d %d %f %f %d %d", &row,&col,&dummy,&dummy,&direction,&nloc)) == 6)  {
      for(i=0;i<nloc;i++) 
	  fscanf(infile,"%d %d %*f %*f",&upstreamrow[i],&upstreamcol[i]);
      if(row==irow && col==icol) break;
  }
	 
  fclose(infile);

  (*upstream)=nloc;

  return ENOERROR;
 error:
  if (infile != NULL)
    fclose(infile);
  return status;
}

/******************************************************************************/
/*				 ReadReservoirFile                            */
/******************************************************************************/
int ReadReservoirFile(const char *dirh_path,char *filename,CELL **incells,
		      int nrows,int ncols)
{
  FILE *fp = NULL;
  char filename2[BUFSIZ+1];
  int row,col;
  int i,j;

  /* Open reservoirfilename */
  sprintf(filename2,"%s/%s",dirh_path,filename);
  fp = fopen(filename2, "r");
  if (fp == NULL) {
      status = errno;
      strcpy(message, filename2);
      goto error;
  }
  else printf("Reservoir file opened: %s\n",filename2);
  
  /* Read file */
  while(fscanf(fp,"%d %d %*f %*f %*d %*s %*d %*f %*f %*f %*f %*d %*d %*d %*s ",&col,&row)!=EOF) {
    for(i=0;i<(nrows+2);i++) {
      for(j=0;j<(ncols+2);j++) {
	if(incells[i][j].row==row && incells[i][j].col==col) incells[i][j].flag_reservoir=1;
      }
    }
  }
	 
  fclose(fp);

  return ENOERROR;
 error:
  if (fp != NULL)
    fclose(fp);
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
int LineCount(char *file)
{
  FILE *fp;
  int c, lines;
 if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open file %s, exiting \n",file);exit(0);}
  lines = 0;
  while((c = fgetc(fp)) !=EOF)  if (c=='\n') lines++;
  fclose(fp);
  return lines;
}
/* END function int LineCount   */
