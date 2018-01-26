/*
 * Purpose: Find cells directly upstream each cell in a basin. 
            The basin is defined by it's direction file. 
 * Usage  : "\nUsage:\n%s\t"
	    "\t[direction file] "
	    "\t[output file defining upstream cells]\n\n"; 
	    BASIN[][]: rows/cols: 1-nrows/ncols
 * Output:  File incl:
            row (from bottom?) col direction?
 * Author : Ingjerd Haddeland
 * E-mail : ingjerd.haddeland@geo.uio.no, iha@nve.no
 * Created: December 2004, January 2009

 *
 * gcc -lm -Wall ~/watch/programs/C/find.upstreamcells.c

./a.out rout/input/29646.dir upstreamcells.txt

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
#define DECIMAL_PLACES 4
#define TRUE 1
/******************************************************************************/
/*			TYPE DEFINITIONS, GLOBALS, ETC.                       */
/******************************************************************************/

typedef enum {double_f, int_f, float_f, long_f, short_f} FORMAT_SPECIFIER;

typedef struct { /* BASIN[][] */
  int direction;
  int row;
  int col;
  float lat;
  float lon;
  int tocol;
  int torow;
  int upstream[3][3];
} CELL;

const float RES = 0.25;
const double DEGTORAD = M_PI/180.;
const double EARTHRADIUS = 6378.;

const char *usage = 
"\nUsage:\n%s\t"
"\t[direction file] "
"\t[fraction file] "
"\t[input damfile] "
"\t[input soilfile] "
"\t[indir fluxfiles] "
"\t[output water demand file]\n\n";
int status = ENOERROR;
char message[BUFSIZ+1] = "";

/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
int ProcessCommandLine(int argc,char **argv,char *dirfilename,
		       char *outfilename);
int FindRowsCols(char *dirfilename,int *rows,int *cols,
		 float *south,float *west);
int ProcessError(void);
int ReadDirFile(char *dirfilename,CELL **BASIN,
		const float RES,int *ncells);
int FindUpstreamCells(CELL **BASIN,int rows,int cols);
int WriteOutput(char *outfilename,CELL **BASIN,int rows,int cols);
/******************************************************************************/
/******************************************************************************/
/*				      MAIN                                    */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
  char dirfilename[BUFSIZ+1];
  char outfilename[BUFSIZ+1];
  int rows,cols,i,j,k,l,ncells;
  float south,west,north,east;
  CELL **BASIN = NULL;

  status = ProcessCommandLine(argc,argv,dirfilename,outfilename);
  if (status != ENOERROR)
    goto error;

  status = FindRowsCols(dirfilename,&rows,&cols,&south,&west);
  if (status != ENOERROR)
    goto error;
  north = south + rows*RES;
  east = west + cols*RES;
  //printf("RowsCols found, rows=%d cols=%d south=%.2f north=%.2f west=%.2f east=%.2f\n",
  // rows,cols,south,north,west,east);
 
  /* Initialize BASIN */
  BASIN = calloc((rows+2), sizeof(CELL));
  if (BASIN == NULL) {
    status = errno;
    sprintf(message, "%s: %d", __FILE__, __LINE__);
    goto error;
  }
  for (i = 0; i <= rows+2; i++) {
    BASIN[i] = calloc((cols+2), sizeof(CELL));
    if (BASIN[i] == NULL) {
      status = errno;
      sprintf(message, "%s: %d", __FILE__, __LINE__);
      goto error;
    }
  }

  for (i = 0; i <= rows+1; i++) {
    for (j = 0; j <= cols+1; j++) {
      BASIN[i][j].lat = (south + (i-0.5)*RES);
      BASIN[i][j].lon = west + (j-.5)*RES;
      BASIN[i][j].row = i;
      BASIN[i][j].col = j;
      BASIN[i][j].direction = MISSING;
      BASIN[i][j].tocol = 0;  
      BASIN[i][j].torow = 0;   
      for(k=0;k<3;k++)
	  for(l=0;l<3;l++)	  
	      BASIN[i][j].upstream[k][l] = 0;
    }
  }

  /* Read basin's direction file */
  status = ReadDirFile(dirfilename,BASIN,RES,&ncells);
  if (status != ENOERROR)
    goto error;
  //printf("Dirfile read, ncells=%d\n",ncells);

  /* Find upstream cells (only those one cell away) */
  status = FindUpstreamCells(BASIN,rows,cols);

  /* Write output */
  status = WriteOutput(outfilename,BASIN,rows,cols);
  if (status != ENOERROR)
    goto error;
  //printf("Output written\n");

  return EXIT_SUCCESS;

 error:

  ProcessError();
  exit(EXIT_FAILURE);
} 

/******************************************************************************/
/*			       ProcessCommandLine                             */
/******************************************************************************/
int ProcessCommandLine(int argc,char **argv,char *dirfilename,char *outfilename)
{
  if (argc != 3) {
    status = EUSAGE;
    strcpy(message, argv[0]);
    goto error;
  }

  strcpy(dirfilename, argv[1]);
  strcpy(outfilename, argv[2]);

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
		 float *south,float *west)
{
  FILE *dirfile = NULL;
  char filename[BUFSIZ+1];
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
  //else printf("Direction file opened: %s\n",filename);

  /* Read header */
  fscanf(dirfile,"%*s %d ",&(*cols));
  fscanf(dirfile,"%*s %d ",&(*rows));
  fscanf(dirfile,"%*s %f ",&(*west));
  fscanf(dirfile,"%*s %f ",&(*south));
  fscanf(dirfile,"%*s %f ",&cellsize);
  fscanf(dirfile,"%*s %f ",&nodata);

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
		const float RES,int *ncells)
{
  FILE *dirfile = NULL;
  char filename[BUFSIZ+1];
  int i;
  int j;
  int rows,cols;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open input fraction file */
  sprintf(filename,"%s",dirfilename);
  dirfile = fopen(filename, "r");
  if (dirfile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  //  else printf("Direction file opened: %s\n",filename);

  /* Read header */
  fscanf(dirfile,"%*s %d ",&cols);
  fscanf(dirfile,"%*s %d ",&rows);
  fscanf(dirfile,"%*s %f ",&west);
  fscanf(dirfile,"%*s %f ",&south);
  fscanf(dirfile,"%*s %f ",&cellsize);
  fscanf(dirfile,"%*s %f ",&nodata);

  (*ncells)=0;

  for (i = rows; i >= 1; i--) {
    for (j = 1; j <= cols; j++) {
       fscanf(dirfile,"%d ",&BASIN[i][j].direction);
       if(BASIN[i][j].direction>nodata) {
	   (*ncells)+=1;
       }
    }
  }

  fclose(dirfile);

  for(i=1;i<=rows;i++) {
    for(j=1;j<=cols;j++) {
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
/*********************************************/
/* FindUpstreamCells                         */
/*********************************************/
int FindUpstreamCells(CELL **BASIN,int rows,int cols)
{
  int i,j,k,l;

  /* Find upstream cells */
  for(i=1;i<=rows;i++) {
      for(j=1;j<=cols;j++) {
	  for(k=0;k<3;k++) {
	      for(l=0;l<3;l++) {
		  if(BASIN[i+k-1][j+l-1].torow==i && BASIN[i+k-1][j+l-1].tocol==j)
		      BASIN[i][j].upstream[k][l]=1;
	      }
	  }
      }
  }

  return ENOERROR;
}
/*****************************************************************/
/*                   WriteOutput                                 */
/*****************************************************************/
int WriteOutput(char *outfilename,CELL **BASIN,int rows,int cols)
{
  FILE *outfile = NULL;
  int i,j,k,l,ncells;

 /* Open outfile used by none */
  outfile = fopen(outfilename, "w");
  if (outfile == NULL) {
    status = errno;
    strcpy(message, outfilename);
    goto error;
  }
  //else printf("outfile opened: %s\n",outfilename);

 /* Print to file. */
  for(i=0;i<=rows;i++) {
      for(j=0;j<=cols;j++) {
	  if(BASIN[i][j].direction>=0) {
	      fprintf(outfile,"%d %d %.4f %.4f %d ",
		      i,j,BASIN[i][j].lat,BASIN[i][j].lon,BASIN[i][j].direction);     
	      ncells=0;
	      for(k=0;k<3;k++) {
		  for(l=0;l<3;l++) {
		      if(BASIN[i][j].upstream[k][l]>0) 
			  ncells+=1;
		  }
	      }
	      fprintf(outfile,"%d\t",ncells);   
	      for(k=0;k<3;k++) {
		  for(l=0;l<3;l++) {
		      if(BASIN[i][j].upstream[k][l]>0) 
			  fprintf(outfile,"%d %d %.4f %.4f\t",
				  i+k-1,j+l-1,BASIN[i+k-1][j+l-1].lat,BASIN[i+k-1][j+l-1].lon);
		  }
	      }
	      fprintf(outfile,"\n");  
	  }
      }
  }

  fclose(outfile);

  return ENOERROR;

 error:
  if (outfile != NULL)
    fclose(outfile);
  return status;
}
