/*
 * Purpose: Extract cells of interest from soilfile, make one file for each cell to be run/routed. Cell of interest = Irrigated cell, outlet, gage, reservoir. 
 * Usage  : soilfile.rearrangepoints.c <basin> <input soil file> <location of new soil files> <arcinfo type irrpercent file> <routing input files location>
 * Result : One file for each cell to be run/routed. 
 *          Name = 10001 and upwards, numbered from upstream to downstream location.
 *          Location of files to be written: Given on command line.
 * Author : Ingjerd Haddeland
 * E-mail : iha@nve.no
 * Created: February 2003
 * Last Changed: 2010
 * Notes  : If in doubt, read the disclaimer.
 *          The spatial resolution is set in program (const float).
 *          This program counts rows from top to bottom (row and col starts at 1)
 *          Number of soil columns is defined in program (define SOILCOLS). 
 *          Writing of files is fixed, and may have to be changed for your needs.  
 *          Direction file to be read must be named <basin>.dir
 *          Expects VIC type direction numbers (1-8, where 1=north)
 *          Points file to be read must be named <basin>.points
 *          Irrigated cells are given in  <arcinfo type irrpercent file>
 *          Reservoir locations, gage(s), outlet cell is given in  <basin>.points (must be loacted in  <routing input files location>)
 *          The outlet location must be included in input points file!
 * Disclaimer: Feel free to use or adapt any part of this program for your own
 *             convenience.  However, this only applies with the understanding
 *             that YOU ARE RESPONSIBLE TO ENSURE THAT THE PROGRAM DOES WHAT YOU
 *             THINK IT SHOULD DO.  The author of this program does not in any
 *             way, shape or form accept responsibility for problems caused by
 *             the use of this code.  
 *
 * 

gcc  -lm -Wall soilfile.rearrangepoints.c

eks:
 $BinPath/soilfile.rearrangepoints $Basin ./input/soil/soil.current $SoilPointsPath/ ./irr.$Basin.asc $DirhPath

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
#define SOILCOLS 56             /* number of cols in soilfile */
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
  int row;
  int col;
  int torow;
  int tocol;
  int dir;
  int localdir;
  int flag;
  int outskirt;
  int rank;
  int newrank;
  int point;
  int inside;
  int withdrawal;
  float lat;
  float lon;
  float irrigation;
} CELL;

const float RES = 0.25;

const char *usage = 
  "\nUsage:\n%s\t"
  "\t[basin number]\t"
  "\t[input soilfile]\t"
  "\t[location of new soil files]\t"
  "\t[arcinfo type irrpercent file]\t"
  "\t[routing input files location]\n\n";
int status = ENOERROR;
char message[100] = "";

/******************************************************************************/
/*			      FUNCTION PROTOTYPES                             */
/******************************************************************************/
void ProcessCommandLine(int argc,char **argv,int *basin,char infilename[100],
			char outfilename[100],char irrfilename[100],
			char dirh_path[100]);
int ProcessError(void);
void SearchCatchment(CELL **incells,int nrows,int ncols,int ncells,int ipoint,
		    int *upstream_cells,int withdrawal_cell);
void SortAgain(CELL **incells,int nrows,int ncols,int ncells,int ipoint,
		int upstream_cells,int *local_cells,int *withdrawal_cell);
void SortBasin(CELL **incells,int nrows,int ncols,int ncells);
void WriteOutput(char infilename[100],char outfilename[100],CELL **incells,
	      int nrows,int ncols,int ncells);
void ReadDirhFile(char *dirh_path,int basin,const float RES, 
		 CELL **incells,int *nrows,int *ncols,int *ncells);
void ReadPointsFile(char *dirh_path,int basin,const float RES, 
		   CELL **incells,int nrows,int *npoints);
int ReadIrrigation(char *irrfilename,CELL **incells,const float RES);
/******************************************************************************/
/******************************************************************************/
/*				      MAIN                                    */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
  char infilename[100];
  char outfilename[100];
  char irrfilename[100];
  char dirh_path[100];
  int ipoint;
  int basin;
  int nrows;
  int ncols;
  int ncells;
  int npoints;
  int upstream_cells;
  int local_cells;
  int total_cells;
  int withdrawal_cell;
  int i;
  CELL **incells = NULL;

  ProcessCommandLine(argc,argv,&basin,infilename,outfilename,irrfilename,dirh_path);
  //printf("Processed commandline. basin %d\n",basin);
  
  incells = calloc(200, sizeof(CELLPTR));
  for (i = 0; i < 200; i++) 
    incells[i] = calloc(200, sizeof(CELL));

  ReadDirhFile(dirh_path,basin,RES,incells,&nrows,&ncols,&ncells);
  //printf("Direction file read,ncells=%d\n",ncells);

  ReadPointsFile(dirh_path,basin,RES,incells,nrows,&npoints);
  //printf("Pointsfile read, npoints=%d\n",npoints);

  ReadIrrigation(irrfilename,incells,RES);
  //printf("Irrigationfile read\n");

  printf("soilfile.rearrangepoints Basin:%d nrows:%d ncols:%d ncells:%d npoints:%d\n",
	 basin,nrows,ncols,ncells,npoints);

  // Sort from upstream to downstream cells, do not take reservoir locations into account
  SortBasin(incells,nrows,ncols,ncells);
  printf("Basin sorted, reservoirs not taken into account yet\n");

  withdrawal_cell=0;
  total_cells=0;
  local_cells=0;

  for(ipoint=1;ipoint<=npoints;ipoint++) {
    //Find cells upstream current reservoir (or outlet cell)
    SearchCatchment(incells,nrows,ncols,ncells,ipoint,&upstream_cells,
		    withdrawal_cell);
    //Sort cells within subcatchment
    SortAgain(incells,nrows,ncols,ncells,ipoint,total_cells,&local_cells,
    	      &withdrawal_cell);
    total_cells+=local_cells;
  }

  if(total_cells!=ncells) 
    printf("Warning! Some cells in original soilfile missing from sort procedure!\n");
  else printf("Soilfile rearrangepoints counting a success\n");

  WriteOutput(infilename,outfilename,incells,nrows,ncols,ncells);

  if(total_cells!=ncells) printf("Warning! Some cells in original soilfile missing from sort procedure!\n");

  return EXIT_SUCCESS;

} 

/******************************************************************************/
/*			       ProcessCommandLine                             */
/******************************************************************************/
void ProcessCommandLine(int argc,char **argv,int *basin,char infilename[100],
			char outfilename[100],char irrfilename[100],
			char dirh_path[100])
{
  if (argc != 6) {
    exit(0);
  }
  
  *basin = atoi(argv[1]);
  strcpy(infilename, argv[2]);
  strcpy(outfilename, argv[3]);
  strcpy(irrfilename, argv[4]);
  strcpy(dirh_path, argv[5]);
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
/*********************************************/
/* SearchCatchment                           */
/* Purpose: Find cells upstream              */ 
/*          current point location           */
/*********************************************/
void SearchCatchment(CELL **incells,int nrows,int ncols,
		    int ncells,int ipoint,int *upstream_cells,
		    int withdrawal_cell)
{
  int i,j;
  int ii,jj,iii,jjj;
  int count;
  int irow,icol;
 
  count = 0;
  (*upstream_cells)=0;

  /* Find row and col in question */
  for(i=1;i<=nrows;i++) {
    for(j=1;j<=ncols;j++) {
      if(incells[i][j].point==ipoint) {
	irow=i;
	icol=j;
      }
    }
  }

  for(i=1;i<=nrows;i++) {
    for(j=1;j<=ncols;j++) {
      ii=i;
      jj=j;
    loop:
      if(ii>nrows || ii<1 || jj>ncols || jj<1) {
//	printf("Outside basin %d %d %d %d\n",ii,jj,nrows,ncols);
      }
      else {
	  if(ii==irow && jj==icol) { 
	    count+=1;
	    if(incells[i][j].inside==MISSING) {
	      incells[i][j].inside=ipoint;
	      incells[i][j].withdrawal=withdrawal_cell;
	    }
	  }
	  else { 
	    /*check if the current ii,jj cell routes down
	      to the subbasin outlet point, following the
	      flow direction from each cell;
	      if you get here no_of_cells increment and you 
	      try another cell*/ 
	    if(incells[ii][jj].tocol!=0 &&    
	       incells[ii][jj].torow!=0) { 
	      iii = incells[ii][jj].torow;         
	      jjj = incells[ii][jj].tocol;         
	      ii  = iii;                  
	      jj  = jjj;                  
	      goto loop;
	    }
	  }
      }
    }
  }

  (*upstream_cells)=count;
  printf("Upstream grid cells from present station: %d\n", 
	 (*upstream_cells));
	 
}
/******************************************************************************/
/*			       SortAgain                                      */
/******************************************************************************/
void SortAgain(CELL **incells,int nrows,int ncols,
	      int ncells,int ipoint,int total_cells,
	      int *local_cells,int *withdrawal_cell)
{
  int i;
  int j;
  int m;
  int n;
  int count=0;
  int dummy;
  int *RANKNUMBER;

  /* Allocate memory for RANKNUMBER */
  RANKNUMBER=(int *)calloc(ncells+1,sizeof(int));
  for(i=0;i<ncells;i++) 
    RANKNUMBER[i]=0;

  /* Find number of cells within this local catchment */
  for(i=1;i<=nrows;i++) {
    for(j=1;j<=ncols;j++) {
      if(incells[i][j].inside==ipoint) { 
	count+=1;
	RANKNUMBER[count]=incells[i][j].rank;
      }
    }
  }

  /* Sort ranknumbers */
  for(m=1;m<count;m++) {
    for(n=count;n>=m;n--) {
      if(RANKNUMBER[n-1]>RANKNUMBER[n]) {
	dummy=RANKNUMBER[n-1];
	RANKNUMBER[n-1]=RANKNUMBER[n];
	RANKNUMBER[n]=dummy;
      }
    }
  }

  for(i=1;i<=nrows;i++) {
    for(j=1;j<=ncols;j++) {
      if(incells[i][j].inside==ipoint) { 
	for(m=1;m<=count;m++) {
	  if(incells[i][j].rank==RANKNUMBER[m]) {
	    incells[i][j].newrank=m+total_cells;
	  }
	}
      }
    }
  }
  
  (*local_cells)=count;
  (*withdrawal_cell)=m+total_cells-1;
  
  free(RANKNUMBER);
  
}
/******************************************************************************/
/*			       SortBasin                                      */
/******************************************************************************/
void SortBasin(CELL **incells,
	      int nrows,int ncols,int ncells)
{
  int i;
  int j;
  int m;
  int n;
  int count=0;

  /* Initialize */
  for(i=0;i<nrows+2;i++) {
    for(j=0;j<ncols+2;j++) {
      incells[i][j].outskirt=incells[i][j].flag;
      incells[i][j].localdir=incells[i][j].dir;
    }
  }

  /* Rank all cells in basin, based on location. Upstream cells get low rank numbers */
  do {
    for(i=1;i<=nrows;i++) {
      for(j=1;j<=ncols;j++) {
	if(incells[i][j].outskirt!=MISSING) {
	  if(incells[i-1][j].localdir==5) incells[i][j].outskirt=1;
	  if(incells[i-1][j+1].localdir==6) incells[i][j].outskirt=1;
	  if(incells[i][j+1].localdir==7) incells[i][j].outskirt=1;
	  if(incells[i+1][j+1].localdir==8) incells[i][j].outskirt=1;
	  if(incells[i+1][j].localdir==1) incells[i][j].outskirt=1;
	  if(incells[i+1][j-1].localdir==2) incells[i][j].outskirt=1;
	  if(incells[i][j-1].localdir==3) incells[i][j].outskirt=1;
	  if(incells[i-1][j-1].localdir==4) incells[i][j].outskirt=1;
	  if(incells[i][j].outskirt==0) {
	    count++;
	    incells[i][j].rank=count;
	  }
	}
      }
    }
    for(m=1;m<=nrows;m++) { // Reset cells
      for(n=1;n<=ncols;n++) {
	if(incells[m][n].outskirt==0) {
	  incells[m][n].outskirt=MISSING;
	  incells[m][n].localdir=MISSING;
	}
	if(incells[m][n].outskirt!=MISSING) incells[m][n].outskirt=0;
      }
    }
  }
  while(count<ncells);
}
/******************************************************************************/
/*			       WriteOutput                                    */
/******************************************************************************/
void WriteOutput(char infilename[100],char outfilename[100],CELL **incells,
		int nrows,int ncols,int ncells)
{
  FILE *outfile;
  FILE *fp;
  char filename[100];
  float **soilcells;
  float latitude,longitude;
  int i,j,k,l,m,n,o,p;

  printf("Infile (WriteOutput): %s\n",infilename);
  fp = fopen(infilename, "r");

  /* Allocate memory */
  soilcells = (float **)calloc(ncells, sizeof(float *));
  if (soilcells == NULL) {
    status = errno;
    sprintf(message, "%s: %d", __FILE__, __LINE__);
  }
  for (i = 0; i < ncells; i++) {
    soilcells[i] = (float *)calloc(SOILCOLS, sizeof(float));
    if (soilcells[i] == NULL) {
      status = errno;
      sprintf(message, "%s: %d", __FILE__, __LINE__);
    }
  }

  /* Read infile */
  for(i=0;i<ncells;i++) {
    for(j=0;j<SOILCOLS;j++) {
      fscanf(fp,"%f ",&soilcells[i][j]);
    }
  }
  fclose(fp);

  /* Write output, one soilfile for each cell/point where routing will be performed */
  for(k=1;k<=ncells;k++) {
   for(i=1;i<=nrows;i++) {
      for(j=1;j<=ncols;j++) {
	if(incells[i][j].newrank==k) {
	  for(m=0;m<ncells;m++) {
	    if((fabs(soilcells[m][2]-incells[i][j].lat)<EPS) && 
	       (fabs(soilcells[m][3]-incells[i][j].lon)<EPS)) {
	      for(l=1;l<nrows;l++) {
		for(o=1;o<=ncols;o++) {
		  if(incells[i][j].withdrawal==incells[l][o].newrank && incells[i][j].withdrawal!=0)
		    for(p=0;p<ncells;p++) {
		      if((fabs(soilcells[p][2]-incells[l][o].lat)<EPS) && 
			 (fabs(soilcells[p][3]-incells[l][o].lon)<EPS)) {
			latitude=soilcells[p][2];
			longitude=soilcells[p][3];
		      }
		    }
		}
	      }
	      if(incells[i][j].point>0 || incells[i][j].irrigation>0.01) {
		sprintf(filename,"%s%d",outfilename,k+10000); //+10000 for run order purposes
		outfile = fopen(filename, "w");
		if (outfile == NULL) {
		  status = errno;
		  strcpy(message, filename);
		}
		for(n=0;n<28;n++) {
		  if(n<2 || ( n>14 && n<18)) 
		    fprintf(outfile,"%d ",(int)soilcells[m][n]);
		  else  {
		    if(n==5) fprintf(outfile,"%.6f ",soilcells[m][n]);
		    else fprintf(outfile,"%.4f ",soilcells[m][n]);
		  }
		}
		for(n=28;n<52;n++)
		  fprintf(outfile," %.4f",soilcells[m][n]);
		fprintf(outfile," %d %7.4f ",
			(int)soilcells[m][52],incells[i][j].irrigation);
		fprintf(outfile," %d %d ",
			(int)soilcells[m][54],(int)soilcells[m][55]);
		fprintf(outfile,"\n");
		fclose(outfile);
	      }
	    }
	  } 
	}
      }
   }
  }
}
/******************************************************************************/
/*				 ReadDirhFile                                 */
/******************************************************************************/
void ReadDirhFile(char *dirh_path, int basin, const float RES, 
		 CELL **incells,int *nrows,int *ncols,int *ncells)
{
  FILE *dirhfile = NULL;
  char filename[100];
  char dummy[25];
  int i;
  int j;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open direction file */
  sprintf(filename,"%s/%d.dir",dirh_path,basin);
  dirhfile = fopen(filename, "r");
  if (dirhfile == NULL) {
    printf("Direction file not found: %s\n",filename);
    exit(0);
  }

  /* Read header */
  fscanf(dirhfile,"%s %d ",dummy,&(*ncols));
  fscanf(dirhfile,"%s %d ",dummy,&(*nrows));
  fscanf(dirhfile,"%s %f ",dummy,&west);
  fscanf(dirhfile,"%s %f ",dummy,&south);
  fscanf(dirhfile,"%s %f ",dummy,&cellsize);
  fscanf(dirhfile,"%s %f ",dummy,&nodata);
  (*ncells)=0;

  /* initialize all the cells */
  for (i = 0; i < (*nrows)+2; i++) {
    for (j = 0; j < (*ncols)+2; j++) {
      incells[i][j].dir = MISSING;
      incells[i][j].lat = (south + (*nrows)*RES) - (i-.5)*RES;
      incells[i][j].lon = west + (j-.5)*RES;
      incells[i][j].flag = MISSING;
      incells[i][j].rank = MISSING;
      incells[i][j].point = MISSING;
      incells[i][j].torow = MISSING;    
      incells[i][j].tocol = MISSING;    
      incells[i][j].inside = MISSING;    
      incells[i][j].withdrawal = MISSING;    
    }
  }

  /* Now read the cells */
  for (i = 1; i <= (*nrows); i++) {
    for (j = 1; j <= (*ncols); j++) {
      fscanf(dirhfile, "%d", &incells[i][j].dir);
      if(incells[i][j].dir<0 || incells[i][j].dir>9 ) 
	incells[i][j].flag=MISSING; 
      else {
	(*ncells)+=1;
	incells[i][j].flag=0; 
      }
    }
  }
  
  fclose(dirhfile);

  /* Find each cell's torow and tocol */
  for(i=0;i<(*nrows)+2;i++) {
    for(j=0;j<(*ncols)+2;j++) {
      if(incells[i][j].dir==0 || incells[i][j].dir==MISSING || incells[i][j].dir==9) {
	incells[i][j].tocol=0;
	incells[i][j].torow=0;
      } 
      else if(incells[i][j].dir==1) {
         incells[i][j].tocol=j;
         incells[i][j].torow=i-1;
      } 
      else if(incells[i][j].dir==2) {
         incells[i][j].tocol=j+1;
         incells[i][j].torow=i-1;
      } 
      else if(incells[i][j].dir==3) {
         incells[i][j].tocol=j+1;
         incells[i][j].torow=i;
      } 
      else if(incells[i][j].dir==4) {
         incells[i][j].tocol=j+1;
         incells[i][j].torow=i+1;
      } 
      else if(incells[i][j].dir==5) {
         incells[i][j].tocol=j;
         incells[i][j].torow=i+1;
      } 
      else if(incells[i][j].dir==6) {
         incells[i][j].tocol=j-1;
         incells[i][j].torow=i+1;
      } 
      else if(incells[i][j].dir==7) {
         incells[i][j].tocol=j-1;
         incells[i][j].torow=i;
      } 
      else if(incells[i][j].dir==8) {
         incells[i][j].tocol=j-1;
         incells[i][j].torow=i-1;
      } 
    }
  }

}
/******************************************************************************/
/*				 ReadPointsFile                               */
/******************************************************************************/
void ReadPointsFile(char *dirh_path, int basin, const float RES, 
		 CELL **incells,int nrows,int *npoints)
{
  FILE *pointsfile = NULL;
  char filename[100];
  int col;
  int row;
  int count=0;

  /* Open points file */
  sprintf(filename,"%s/%d.points",dirh_path,basin);
  pointsfile = fopen(filename, "r");
  if (pointsfile == NULL) {
    exit(0);
  }
  //else printf("Points file opened: %s\n",filename);

  /* Now read the cells */
  while(fscanf(pointsfile, " %d %d", &col,&row)!=EOF) {
    fscanf(pointsfile, "%*f %*f %*s %d", &(incells[nrows-row+1][col].point));
    if(incells[nrows-row+1][col].point>0) count+=1;
  }
  
  (*npoints)=count;
  printf("Readpoints npoints: %d\n",(*npoints));

  fclose(pointsfile);
}
/******************************************************************************/
/*				 ReadIrrigation                               */
/******************************************************************************/
int ReadIrrigation(char *filename,CELL **incells,const float RES)
{
  FILE *infile = NULL;
  char dummy[25];
  int i;
  int j;
  int rows,cols;
  float west;
  float south;
  float cellsize;
  float nodata;

  /* Open irrigation file */
  infile = fopen(filename, "r");
  if (infile == NULL) {
    status = errno;
    strcpy(message, filename);
    goto error;
  }
  //  else printf("Irrigation file opened: %s\n",filename);

  /* Read header */
  fscanf(infile,"%s %d ",dummy,&cols);
  fscanf(infile,"%s %d ",dummy,&rows);
  fscanf(infile,"%s %f ",dummy,&west);
  fscanf(infile,"%s %f ",dummy,&south);
  fscanf(infile,"%s %f ",dummy,&cellsize);
  fscanf(infile,"%s %f ",dummy,&nodata);

  for (i = 1; i <= rows; i++) {
    for (j = 1; j <= cols; j++) {
       fscanf(infile,"%f ",&incells[i][j].irrigation);
    }
  }

  return ENOERROR;
 error:
  if (infile != NULL)
    fclose(infile);
  return status;
}
/*************************************************/

