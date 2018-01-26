#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rout.h"

void ReadWaterDemand(char *demandpath,
		     char *resname,
		     float **WATER_DEMAND,
		     int start_year,
		     int stop_year,
		     int ndays,
			 float *monthlyFlow,
			 float mean_inflow,
             int adj_water_demand)
{
  FILE *fp;
  char filename[150];
  char fmtstr[150];
  int i,j;
  float Monthly_WATER_DEMAND[13];
  float Ratio[13];
  float sum;
  float yearly_ratio;
  float total_dem;
  float total_flow;

  strcpy(filename,demandpath);
  sprintf(fmtstr,"/%s.calc.irrdemand.monthly",resname);
  strcat(filename,fmtstr);

  if((fp = fopen(filename, "r")) == NULL) {
    printf("Cannot open %s\n",filename);
    exit(1);
  }
  else printf("File opened %s\n",filename);

  /* Read water demand. Numbers in file are in m3s-1,
     recalculate to m3 day-1 */
  for(i=0;i<stop_year-start_year+1;i++) {
    for(j=1;j<=12;j++) {
      fscanf(fp,"%*d %*d %*f %f",&WATER_DEMAND[i][j]);
      if(i<1) 
        printf("readwaterdemand YY[%2d] MM[%d] %f m3s-1\t",i,j,WATER_DEMAND[i][j]);
      WATER_DEMAND[i][j]*=CONV_M3S_CM;
      if(i<1) 
        printf("%.2f m3day-1\n",WATER_DEMAND[i][j]);
    }
  }
  

  /* Reduce monthly mean water demand proportional to the monthly mean flow  
  availability. This is a temporary solution to skip over the absurdly large water 
  demand (>10 times monthly mean flow) from simulation. This approach doesn't account 
  for storage availability (Ning 10/10). */
  printf("\n\n adjust water demand by avaiable mean monthly flow\n");
  if (adj_water_demand > 0) {
    /* calculate the ratio of monthly mean water demand to monthly mean naturalized flow
     -- edited by Ning */
    printf("\n\n\nPre-adjusted Mean Monthly Irr Demand/ Mean Monthly Inflow:\n");
    Ratio[0] = 0.;
    Monthly_WATER_DEMAND[0] = 0.;
    for (j = 1; j <= 12; j++) {
      Monthly_WATER_DEMAND[j] = 0.;
      Ratio[j] = 0.;
      for (i = 0; i < stop_year - start_year + 1; i++) {
        Monthly_WATER_DEMAND[j] += WATER_DEMAND[i][j];
      }
      Monthly_WATER_DEMAND[j] /= stop_year - start_year + 1;
    }

total_dem = 0; total_flow = 0;   
for (j = 1; j<=12; j++){
      total_dem += Monthly_WATER_DEMAND[j];
      total_flow += monthlyFlow[j];
}
 for (j = 1; j<=12; j++){    
      //Ratio[j] = Monthly_WATER_DEMAND[j] / monthlyFlow[j];
    Ratio[j] = total_dem / (total_flow);

}

    for (i = 0; i < stop_year - start_year + 1; i++) {
      for (j = 1; j <= 12; j++) {
        printf("YY[%2d]MM[%2d]: ratio=%5.0f, meanflow=%.1f\t, pre=%.1f\t,", 
          i, j, Ratio[j], monthlyFlow[j], WATER_DEMAND[i][j]);
        if (Ratio[j] > 1)
          WATER_DEMAND[i][j] /= Ratio[j];
        printf("adj=%.1f\n", WATER_DEMAND[i][j]);
      }
    }
  }

  
  /* calculate the ratio of annual mean water demand to annual mean naturalized flow 
   -- edited by Ning */
  //sum = 0.;
  //for(j=1;j<=12;j++) 
  //  sum += Monthly_WATER_DEMAND[j];
  //sum /= 12;
  //yearly_ratio = sum/mean_inflow;
  //printf("yearly adj ratio is %.2f\n", yearly_ratio);
  //
  ///* adjusted water demand by the mean annual ratio */
  //for(i=0;i<stop_year-start_year+1;i++) {
  //  for(j=1;j<=12;j++) 
  //    WATER_DEMAND[i][j]/=yearly_ratio;
  //}
  
  
  
  fclose(fp);
}
