#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rout.h"

/*********************************************/
float FindStartOfOperationalYear(float mean_inflow,
  int ndays,
  char *name,
  char *naturalpath,
  int *start_month,
  int *end_month,
  float latitude,
  float longitude)

{
  FILE *fp_day;
  int i, j, year;
  int month = 0;
  int years;
  int days;
  int startyear = 0;
  int count;
  int longest;
  float *MONTHFLOW;
  int *DRYMONTH;
  int *WETMONTH;
  float *flood;
  float mean_flood;
  float streamflow;
  char filename[250];
  char filename2[250];
  int DaysInMonth[13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };

  sprintf(filename, "%sstreamflow_%.4f_%.4f", naturalpath, latitude, longitude);
  if ((fp_day = fopen(filename, "r")) == NULL) {
    printf("Cannot open %s (FindStartOfOperationalYear)\n", filename);
    exit(1);
  }
  else printf
    ("Naturalized streamflow file opened for reading: %s\n\n\n", filename);

  MONTHFLOW = (float*)calloc(14, sizeof(float));
  DRYMONTH = (int*)calloc(14, sizeof(int));
  WETMONTH = (int*)calloc(14, sizeof(int));
  years = (int)ndays / 365;
  flood = (float*)calloc(years, sizeof(float));

  for (i = 1; i <= ndays; i++) {
    fscanf(fp_day, "%d %d %*d %f", &year, &month, &streamflow);
    if (i == 1)
      startyear = year;
    /* find annual max flow from daily flow records */
    if (flood[year - startyear] < streamflow)
      flood[year - startyear] = streamflow;
    days = DaysInMonth[month];
    /* calculate mean monthly flow */
    MONTHFLOW[month] += streamflow / (days*years);
  }
  fclose(fp_day);

  mean_flood = 0;
  for (i = 0; i < years; i++) {
    mean_flood += flood[i] * CONV_M3S_CM / years; //mean_flood in m3day-1
    printf("year[%d] flood=%.2f (m3day-1)\n", i, flood[i]);
  }

  for (i = 1; i < 13; i++) {
    //printf("Naturalized flow, FindStartOfOperationalYear (m3s-1):%f\n",MONTHFLOW[i]);
    if (MONTHFLOW[i] < mean_inflow / CONV_M3S_CM)  //remember mean_inflow is in m3day-1
      DRYMONTH[i] = 1;
    else
      WETMONTH[i] = 1;
  }

  /*Find the longest period of consecutive wet months */
  count = 0;
  longest = 0;
  for (i = 1; i < 13; i++) {
    if (WETMONTH[i] == 1) {
      count += 1;
    }
    else {
      if (count > longest) {
        longest = count;
        month = i;
      }
      else count = 0;
    }
  }

  /* Go through once more to take care of new year's */
  for (i = 1; i < 13; i++) {
    if (WETMONTH[i] == 1) {
      count += 1;
    }
    else {
      if (count > longest) {
        longest = count;
        month = i;
      }
      else count = 0;
    }
  }

  if (month > 12) 
    month = 12;

  (*start_month) = month;

  /* Find the longest period of consecutive dry months */
  /* Why loop again? (Ning) */
  count = 0;
  longest = 0;
  for (i = 1; i < 13; i++) {
    if (DRYMONTH[i] == 1) {
      count += 1;
    }
    else {
      if (count > longest) {
        longest = count;
        month = i;
      }
      else count = 0;
    }
  }

  /* Go through once more to take care of new year's */
  for (i = 1; i < 13; i++) {
    if (DRYMONTH[i] == 1) {
      count += 1;
    }
    else {
      if (count > longest) {
        longest = count;
        month = i;
      }
      else count = 0;
    }
  }

  if (month > 12) 
    month = 12;

  free(WETMONTH);
  free(DRYMONTH);
  free(MONTHFLOW);
  free(flood);

  (*end_month) = month;

  return(mean_flood);
}

