/*
 * Purpose: 
 * Usage  : soilfile.rearrangepoints.c <basin> <input soil file> <location of new soil files> <arcinfo type irrpercent file> <routing input files location>
 * Result : One file for each cell to be run/routed. 
 *          Name = 10001 and upwards, numbered from upstream to downstream location.
 *          Location of files to be written: Given on command line.
 * Author : 
 * E-mail : 
 * Created:
 * Last Changed: 
 * Notes  : 
 * Disclaimer: Feel free to use or adapt any part of this program for your own
 *             convenience.  However, this only applies with the understanding
 *             that YOU ARE RESPONSIBLE TO ENSURE THAT THE PROGRAM DOES WHAT YOU
 *             THINK IT SHOULD DO.  The author of this program does not in any
 *             way, shape or form accept responsibility for problems caused by
 *             the use of this code.  
 *

gcc  -lm -Wall soilfile.rearrangepoints.c

eks:
 $BinPath/soilfile.rearrangepoints $Basin ./input/soil/soil.current $SoilPointsPath/ ./irr.$Basin.asc $DirhPath

 */

/******************************************************************************/
/*			    PREPROCESSOR DIRECTIVES                           */
/******************************************************************************/
#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************************************************************************/
/******************************************************************************/
/*				      MAIN                                    */
/******************************************************************************/
/******************************************************************************/
int main(int argc, char **argv)
{
  char infilename[200];
  char outfilename[200];
  char *tmpdata;
  char *buffer;
  FILE *fp1;
  FILE *fp2;
  int skipdays;
  int ndays;
  int totaldays;
  int lsize;
  int i, n, count;
  long filesize;
  long result1, result2;
  
  if (argc != 6) {
    printf("usage: input file, output file, skip days, model run days, \n");
	printf("       total record days\n");
    exit(0);
  }
  
  strcpy(infilename, argv[1]);
  strcpy(outfilename, argv[2]);
  skipdays = atoi(argv[3]);
  ndays = atoi(argv[4]);
  totaldays = atoi(argv[5]);

  /* Open the input file */
  if((fp1 = fopen(infilename,"rb")) == NULL) { 
    printf("Cannot open input file %s \n", infilename);
    exit(0); 
  }
  fseek (fp1 , 0 , SEEK_END);
  filesize = ftell (fp1);
 /* printf("The file size is %d bytes. \n", filesize); */
  rewind (fp1);

  lsize = filesize/totaldays;
  
  /* allocate memory for the variables */
  if ((buffer = (char*) malloc(sizeof(char)*lsize*ndays)) == NULL){
    printf("memory error\n");
	exit (2);
  }
  if ((tmpdata = (char*) malloc(sizeof(char)*lsize)) == NULL){
    printf("memory error\n");
	exit (2);
  } 
  
  /* Open the output file */
  if((fp2 = fopen(outfilename,"wb")) == NULL) { 
    printf("Cannot open output file %s \n", outfilename);
    exit(0); 
  } 

  /* Skip data */
  for (i = 0; i < skipdays; i++) { 
    result1 = fread(tmpdata, 1, lsize*sizeof(char), fp1);
    /*printf("result1 = %d bytes\n", result1);*/
  }
	
  result1 = fread(buffer, 1, sizeof(char)*lsize*ndays, fp1);
 /* printf("result1 = %d bytes\n", result1); */
  
  if (result1 != sizeof(char)*lsize*ndays) {
    printf("Failed to read proper numbers of inputs file\n");
	exit (0);
  }
  result2 = fwrite(buffer, 1, sizeof(char)*lsize*ndays, fp2);
  if (result2 != sizeof(char)*lsize*ndays) {
    printf("Failed to write out proper numbers of output file\n");
	exit (0);
  } 
  
  /* output the size of the output file */
  fseek (fp2 , 0 , SEEK_END);
  filesize = ftell (fp2);
 /* printf("The output file size is %d bytes. \n", filesize); */
  rewind (fp2);

		
  fclose(fp1);
  fclose(fp2);
  
 /* printf("closing files \n"); */

  return EXIT_SUCCESS;

} 
