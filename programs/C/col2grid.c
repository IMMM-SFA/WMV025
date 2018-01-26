/*
 * SUMMARY:      This programs reads a file with latitude,longitude, value
 *               and grids it into an asciigrid file. This file can then be 
 *               imported into ArcView or ArcInfo.
 *
 * USAGE:        Used together with the VIC model.
 *
 * AUTHOR:       Bernt Viggo Matheussen
 * ORG:          University of Washington, Department of Civil Engineering
 * ORIG-DATE:    13-Oct-98 at 11:52:06
 * LAST-MOD:     Fri Nov 20 16:37:39 1998 by Bernt Viggo Matheussen
 *               Tue Oct 24 2000 by Ed Maurer
 * DESCRIPTION:  
 * DESCRIP-END.
 * FUNCTIONS:    
 * COMMENTS:     The final maskfile is rectangular even though the 
 *               latitude and longitudes given in input file do not
 *               necessarily make a rectangle. 
 *               The program seeks the maximum long/lat and minimum long/lat 
 *               and makes a rectangle out of these max and min coordinates.
 *               Alternatively, a defined box can be input for the max/min
 *               dimensions for the final grid.
 *               All the gridcells are given the value that corresponds to 
 *               the lat/long in the input file. 
 *               Gridcells with no value associated are set to the void number.
 *


gcc -lm /home/iha/programs/C/col2grid.c

./a.out infile resolution void_nr > outfile

or: ./a.out infile resolution void_nr miny maxy minx maxx > outfile

./a.out infile resolution void_nr -90 90 -180 180 > outfile

./a.out wb1_landmask.gmt 0.5 0  > wb1_landmask_submitarea.asc


 */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

/* Returns number of lines in file */
int line_count(char file[200]);

/* Calloc memory to a two dimensional array */
float **allocate_memory(int rows, int cols);

/* Free memory */
void free_memory(float **LATLON, int rows, int cols);

/* Reads in data into array */
int read_data(float **LATLON, int cells, char latlonfile[200]);

/* Function returns cell value given latitude and longitude */
/* If the cell is not present the function returns the void number */
float find_gridcellvalue(float lat, float lon, float void_nr, int cells, float **LATLON);

int main(int argc, char **argv)
{ 
  char latlong[200];
  float resolution;
  int cells, i,j,test;
  float **LATLON;
  float void_nr;
  float maxlat, minlat, maxlong, minlong;
  int rows,cols;
  float latitude,longitude, cellvalue;
  float MINLAT, MINLONG;
  float min_lat1,max_lat1,min_lon1,max_lon1;

  if (argc!=4 && argc!=8) {           /* Must be exactly 3 or 7 arguments behind the program name */
  fprintf(stderr,"Incorrect number of commandline arguments \n");
  fprintf(stderr,"usage: grid_ll infile resolution void_nr > outfile\n"); 
  fprintf(stderr,"   or: grid_ll infile resolution void_nr min_lat max_lat min_lon max_lon > outfile\n");
  fprintf(stderr,"   longitude uses -180 to 180 convention\n");
  exit(EXIT_FAILURE); }

  strcpy(latlong,argv[1]);    /* Infile with latitude,longitude and value */
  //fprintf(stderr,"Inputfile:    %s\n",latlong);
  resolution = atof(argv[2]); /* Converts argument 2 to resolution */
  //fprintf(stderr,"Resolution:   %2.4f\n",resolution);
  void_nr = atof(argv[3]);    /* Converts argument 3 to void number */
  //fprintf(stderr,"void_nr:      %6.0f\n",void_nr);

  min_lat1=max_lat1=min_lon1=max_lon1=0.0;
  if(argc==8){ /* optional declaration of box bounds for arc/info ascii grid */
    min_lat1 = atof(argv[4]);
    max_lat1 = atof(argv[5]);
    min_lon1 = atof(argv[6]);
    max_lon1 = atof(argv[7]);
    //fprintf(stderr,"miny %f maxy %f minx %f maxx %f\n",min_lat1,max_lat1,min_lon1,max_lon1); 
  }

  cells = line_count(latlong);
  //fprintf(stderr,"lines %d\n",cells);

  /* Allocates memory to a matrix   LATLON[Cells][3]  */
  LATLON = allocate_memory(cells,3);

  /* Read Latlong data */
  test=read_data(LATLON,cells,latlong);
  //fprintf(stderr,"lines %d %s\n",test,latlong);

  /* initialize the outer grid dimensions */
    maxlat  = minlat  =  LATLON[0][0];  /* Set initial values */
    maxlong = minlong =  LATLON[0][1];  /* Set initial values */

  for(i=0;i<cells;i++)  /* Search to find maxlat, minlat, maxlong, minlong */
    {
    if (maxlat  < LATLON[i][0]) maxlat = LATLON[i][0];
    if (minlat  > LATLON[i][0]) minlat = LATLON[i][0];
    if (maxlong < LATLON[i][1]) maxlong= LATLON[i][1];
    if (minlong > LATLON[i][1]) minlong= LATLON[i][1];
    }

  /* check if bounding box has been defined, compare with data area */
  if(argc==8){
    if(maxlat>max_lat1 || minlat<min_lat1 || maxlong>max_lon1 ||
       minlong<min_lon1){
      fprintf(stderr,"Defined bounding box is smaller than dimensions of data\n");
      fprintf(stderr,"Bounding box is being expanded to contain all of the data\n");
      if(maxlat>max_lat1) max_lat1=maxlat+(resolution/2.0);
      if(minlat<min_lat1) min_lat1=minlat-(resolution/2.0);
      if(maxlong>max_lon1) max_lon1=maxlong+(resolution/2.0);
      if(minlong<min_lon1) min_lon1=minlong-(resolution/2.0);
    }
    //fprintf(stderr,"maxlat   %.4f minlat  %.4f\n",max_lat1,min_lat1);
    //fprintf(stderr,"maxlong  %.4f minlong %.4f\n",max_lon1,min_lon1);    
    maxlat=max_lat1-(resolution/2.0);
    minlat=min_lat1+(resolution/2.0);
    maxlong=max_lon1-(resolution/2.0);
    minlong= min_lon1+(resolution/2.0);
  }

  fprintf(stderr,"maxlat   %.4f minlat  %.4f\n",maxlat,minlat);
  fprintf(stderr,"maxlong  %.4f minlong %.4f\n",maxlong,minlong);

  cols = (int)((maxlong - minlong)/resolution)+1; /* Calculating rows and colons */
  rows = (int)((maxlat - minlat)/resolution)+1;

  MINLAT = minlat - (resolution/2.0);  /* Going from cell centers to corner of cell */
  MINLONG = minlong - (resolution/2.0);

  printf("ncols         %d\n",cols);
  printf("nrows         %d\n",rows);
  printf("xllcorner     %6.4f\n",MINLONG);
  printf("yllcorner     %6.4f\n",MINLAT);
  printf("cellsize      %6.4f\n",resolution);
  printf("NODATA_value  %6.0f\n",void_nr);

  latitude  = maxlat;
  longitude = minlong; 
  
  for(i=0;i<rows;i++)             
    {
    for(j=0;j<cols;j++)
      {
      cellvalue = find_gridcellvalue(latitude,longitude,void_nr,cells,LATLON);
      if(cellvalue==void_nr)printf("%.2f ",cellvalue);
      else printf("%.2f ",cellvalue);
      longitude = longitude + resolution;  
      }
    printf(" \n");
    latitude = latitude - resolution;
    longitude = minlong;
    } /* END  for(i=0;i<rows;i++)  */

  return 0;

}/* END main **************************/


int line_count(char file[200])
{
  FILE *fp;
  int c, lines;
  if((fp = fopen(file,"r"))==NULL){
    printf("Cannot open file %s \n",file);exit(0);}
  lines = 0;
  while((c = fgetc(fp)) !=EOF)  if (c=='\n') lines++;
  fclose(fp);
  return lines;
}/* END function int line_count(char file[200])   */



float **allocate_memory(int rows, int cols)
{
  int i;
  /* ALLOCATE MEMORY *************************************************/
  float **LATLON;
  /* LATLON[cells][3] will be allocated */
  LATLON = malloc(rows*sizeof(float*));
  if (NULL==LATLON) return NULL;

  for (i=0;i<rows;i++)
    {
    LATLON[i] = malloc (cols*sizeof(float));
    if (NULL==LATLON[i]) 
      {
      return NULL;
      free_memory(LATLON,rows,cols);
      }
    }
  return LATLON;
    /* FINSIH ALLOCATING MEMORY *****************************************/
}/* END function void read_latlon()  */

/* Free memory */
void free_memory(float **LATLON, int rows, int cols)
{
  int i;
  for(i=0;i<cols;i++) free(LATLON[i]);
  free(LATLON);
}/* END function void free_memory()  ******/

int read_data(float **LATLON, int cells, char latlonfile[200])
{
  FILE *fp;
  int i;
  float dummy;

  if((fp = fopen(latlonfile,"r"))==NULL){       
    printf("Cannot open file %s \n",latlonfile);
    exit(0);}
  else fprintf(stderr,"File opened %s\n",latlonfile);
  
  for(i=0;i<cells;i++) { 
      //fscanf(fp,"%*f %f %f ",&LATLON[i][0],&LATLON[i][1]); //lat=0,lon=1
      //LATLON[i][2]=1;
      fscanf(fp,"%f %f %f",&LATLON[i][0],&LATLON[i][1],&LATLON[i][2]);
  }
  fclose(fp);

  return i;
}/* END function void read_data() */

float find_gridcellvalue(float lat, float lon, float void_nr, int cells, float **LATLON)
{
  int i;
  float return_value;  /* This is the value that will be reutnred by the function */
  return_value = void_nr;
  for (i=0;i<cells;i++)
    {
    if ((1000*(LATLON[i][0])) == (1000*lat)  && (1000*(LATLON[i][1])) == 1000*lon)
      {
      return_value = LATLON[i][2];
      }
    }
  return return_value;  /* If return_value is not assigned a value it returns the void_nr */
}/* END function */


