
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rout.h"


void ReservoirRouting(char *filename,
  char *demandpath,
  char *moscem_path,
  char *moscem_outfile,
  double *FLOW,
  float *R_FLOW,
  float *STORAGE,
  float *LEVEL,
  float *PowerProd,
  float **WATER_DEMAND,
  float **RESEVAPDATA,
  TIME *DATE,
  char *name,
  int row,
  int col,
  int start_year,
  int stop_year,
  int ndays,
  int demand,
  int basin_number,
  char *naturalpath)
{
  FILE *fp, *fin, *fmin, *fres, *fmoscem;

  char resname[25];
  char purpose[7];
  char fmtstr[50];
  char file_name[BUFSIZ];
  char infile_path[BUFSIZ];     /* input (forcing) data */
  char rescarfile_path[BUFSIZ]; /* file holding reservoir characteristics */
  char rescarfile_irr_path[BUFSIZ];
  char valfile_path[BUFSIZ];   /* input (forcing) data */
  char junk[BUFSIZ];
  char *purpose1, *purpose2, *purpose3;
  char *purptest;

  double inflow, oldstorage, oldflow;
  double temp_storage;
  double release, surplus_storage, spill;
  double possible_flow;
  double *res_evap; /* m/day */
  double K_E;

  float dummy;
  float sum_actual_water_demand;
  float capacity; /* input capacity must be given in m3 */
  float *max_release;
  float min_head = 0;
  float *power_demand;  /* Input demand must be given in MW */
  float *actual_water_demand;
  float *outflow; /* m3 (daily) */
  float *minflow; /* m3 (daily) */
  float *avail_water;  /* m3 */
  float *waterdemanddaily; /*(m3 daily)*/
  float *storage;  /* Read in Mm3, multiply by 1000, use in m3 */
  float *flow_accumulated;  /* m3 (daily) */
  float *res_evap_accumulated;  /* m3 (daily) */
  float storage_optimal_start_hyd_year = 0.8;
  float storage_optimal_start_hyd_year_flood = 0.8;
  float storage_min_start_hyd_year;
  float storage_min_res_evap = 0.2;  //arbitrary
  float storage_fraction;
  float power_prod;
  float head;
  float otmcm;
  float mean_inflow; /* m3 (daily) */
  float month_inflow[13]; /* m3 month-1 */
  float year_mean_inflow;
  float this_year_inflow;
  float mean_flood;
  float this_year_demand, dummy_demand;
  float year_mean_water_demand;
  float fraction, factor;
  float q7_10; /* m3s-1 */
  float end_storage, start_storage;
  float available;
  float tot_res_evap = 0;
  float inst_cap; /* in MW */
  float dummy_inst_cap;
  float surf_area;  /* input surface area is given in 1000 m2 */
  float latitude;
  float longitude;

  int i; /* i runs from 0 to ndays */
  int j; /* j runs from 0 to 365+leapyear */
  int k; /* k also runs from 0 to 365+leapyear */
  int m; /* m runs from 1 to 12 */
  int yy, year, month, day;
  int height;
  int irow, icol, type;
  int start_month;
  int end_month;
  int DaysInMonth[13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
  int oldmonth, newmonth;
  int test, leapyear;
  int purp_irr;
  int purp_flood;
  int purp_hydro;
  int moscem;
  int status;

  int nyear_fill = 5;
  int year_fill_start, year_fill_end;
  int fill_flag; //0 for pre-filling, 1 for during filling, 2 for after filling


  moscem = 1;
  /* options: adjust water demand by flow availability*/
  int adj_water_demand = 1;  //if want to adjust wate demand by mean natural inflow -- Ning

  if ((fp = fopen(filename, "r")) == NULL) {
    printf("Cannot open %s (ReservoirRouting)\n", filename);
    exit(1);
  }
  else printf("File opened, reservoir routing information: %s %s\n", filename, name);

  year = 0;

  /* Allocate memory */
  res_evap = (double*)calloc(ndays + 1, sizeof(double));
  outflow = (float*)calloc(ndays + 1, sizeof(float));
  minflow = (float*)calloc(ndays + 1, sizeof(float));
  storage = (float*)calloc(ndays + 1, sizeof(float));
  max_release = (float*)calloc(ndays + 1, sizeof(float));
  avail_water = (float*)calloc(ndays + 1, sizeof(float));
  flow_accumulated = (float*)calloc(ndays + 1, sizeof(float));
  power_demand = (float*)calloc(13, sizeof(float));
  actual_water_demand = (float*)calloc(13, sizeof(float));
  res_evap_accumulated = (float*)calloc(367, sizeof(float));
  waterdemanddaily = (float*)calloc(367, sizeof(float));


  /* Find reservoir located in current cell */
  while (fscanf(fp, "%d %d %f %f %*d %s %d %f %f %*f %f %*d %*d %d ", 
    &icol, &irow, &longitude, &latitude, resname, &year_fill_start, &capacity, &surf_area, &inst_cap, &height) != EOF) {

    fgets(purpose, 7, fp);

    capacity *= 1000.; //go from 1000m3 to m3
    surf_area *= 1000.; //go from 1000m2 to m2. 

    year_fill_end = year_fill_start + nyear_fill - 1;  //for storage initialization only

    if (irow == row && icol == col) { /* Reservoir found */

      printf("ResRout: row[%d] col[%d]\n name: %s\n capacity:%.1f(m3)\n Instant Capacity:%.1f\n Height:%d SurfArea:%.1f(m2)\n fill start year %d\n",
        row, col, resname, capacity, inst_cap, height, surf_area, year_fill_start);

      if (surf_area == 0)
        surf_area = 1e6*exp(log(capacity / (1e6*9.208)) / 1.114); /* Takeuchi, 1997 */
      if (capacity / (surf_area*height) > 2)
        surf_area = 1e6*exp(log(capacity / (1e6*9.208)) / 1.114); /* Some ICOLD areas seem to be unrealistically small. Possible unit errors */

      /* Initialize reservoir and waterdemands */
      for (i = 1; i <= ndays; i++)
        max_release[i] = 0.;
      for (i = 0; i < stop_year - start_year + 1; i++)
        for (j = 0; j < 13; j++)
          WATER_DEMAND[i][j] = 0.;

      purp_irr = purp_flood = purp_hydro = 0;
      purpose1 = strchr(purpose, 'I');
      purpose2 = strchr(purpose, 'H');
      purpose3 = strchr(purpose, 'C');
      if (purpose1 != NULL) purp_irr = 1;
      if (purpose2 != NULL) purp_hydro = 1;
      if (purpose3 != NULL) purp_flood = 1;
      purp_flood = 0; //because in this version you do not allow outflow larger 
                    //than mean annual flood anyway.

      /* Initialize storage */
      if (start_year > year_fill_end) {
        fill_flag = 2;//after filled
        storage[0] = storage_optimal_start_hyd_year*capacity;  //optimal storage
      }
      else if (start_year <= year_fill_start) {
        fill_flag = 0;//before filling
        storage[0] = 0;  //no storage
      }
      else {
        fill_flag = 1;//filling
        storage[0] = (storage_optimal_start_hyd_year*capacity)*(start_year - year_fill_start) / (float)nyear_fill;
      }

      /* set the storage threshold*/
      if (purp_flood == 1)
        storage_optimal_start_hyd_year = storage_optimal_start_hyd_year_flood;
      storage_fraction = storage_optimal_start_hyd_year;
      oldstorage = storage[0];

      printf("\tCapacity %.1f; Initial Storage: %.1f m3; storage[0]/capacity=%.1f%%; purpose:%s\n",
        capacity, storage[0], storage[0]/capacity*100, purpose);
      printf("\tStartYear[%d] & Stopyear[%d];  ndays=%d\n", start_year, stop_year, ndays);


      /* Calculate mean inflow for the entire simulation period
      (annual and monthly). Calculated based on current routing simulation */
      CalculateMeanInflow(FLOW, DATE, ndays, &mean_inflow, flow_accumulated, month_inflow);


      /* Read water demands if appropriate. [nyears][month]. year starts at 0, month goes from 1 to 12
     Only happens when you have irrigation = TRUE or FREE and purp_irr==1
     Numbers in file you read are in m3s-1, but are recalculated to m3day-1 */
     /* Normalized monthly water demand by monthly mean naturalized flow -- edited by Ning */
      if (demand && purp_irr == 1)
        ReadWaterDemand(demandpath, resname, WATER_DEMAND, start_year, stop_year, 
          ndays, month_inflow, mean_inflow, adj_water_demand);

      printf("ResRout: Mean annual inflow=%.1f (m3day-1) %.2f (m3/s), or %.2f x capacity\n",
        mean_inflow, mean_inflow / CONV_M3S_CM, mean_inflow*365/capacity);

      /* If instcap not given, or outside "reasonable" limits:
     Set to mean_inflow * height..... hm. ok assumption???? */
      printf("\n\nResRout: instcap=%.2f; Calc instcap:%.2f\n",
        inst_cap, mean_inflow/CONV_M3S_CM*height*EFF*GE / 1000.);
      dummy_inst_cap = mean_inflow / CONV_M3S_CM*height*EFF*GE / 1000.; //MW

      /* If instcap not given or possibly not plauseible, use calculated.
     Comment out if you know that given InstCap is OK! */
      if (inst_cap < 0.1 || dummy_inst_cap / inst_cap>10 || dummy_inst_cap / inst_cap < 0.1)
        inst_cap = mean_inflow / CONV_M3S_CM*height*EFF*GE / 1000.; //MW 
      printf("ResRout: Final instcap:%.2f; Calc instcap:%.2f\n\n",
        inst_cap, mean_inflow / CONV_M3S_CM*height*EFF*GE / 1000.);

      /* Find time of year when reservoir filling is likely to be at its
     highest and lowest. Highest defines start of operational year.
     In addition, the mean annual flood is calculated.
     All based on calculated naturalized flow.  */
      mean_flood = FindStartOfOperationalYear(mean_inflow,
        ndays, name, naturalpath,
        &start_month, &end_month,
        latitude, longitude);

      /* Calc 7Q10 (m3day-1), based on simulated daily naturalized flow for entire simulation period.
     This value is set as minimum water release from reservoir (environmental flow). */
      q7_10 = Find7Q10(ndays, stop_year - start_year, name, basin_number, naturalpath, latitude, longitude);
      printf("ResRout: Start of operational wet month: MM[%d] ; end dry month: MM[%d]\n",
        start_month, end_month);
      printf("ResRout: mean_flood:%.1f (m3day-1) or %.1f (m3/s)\n q710:%.1f (m3/day) or %.2f (m3/s)\n\n",
        mean_flood, mean_flood / CONV_M3S_CM, q7_10, q7_10 / CONV_M3S_CM);

      /* Calculate inflow and demand and outflow.
      One operational year at a time,
      i.e. start at beginning of operational year.
      Reservoir evaporation taken into account in optimization scheme.
      i: #day in the entire simulation period (1-ndays)
      j: #day in current year (operational year) (0-365/366)
      k: month-counter (1-12)
      */
      for (i = 1; i <= ndays; i++) {
        yy = DATE[i].year;
        month = DATE[i].month;
        year_mean_inflow = 0.;
        j = 0;

        if (yy < year_fill_end) {
          storage_min_start_hyd_year = 0.15*(yy - year_fill_start + 1);
          storage_optimal_start_hyd_year = 0.15*(yy - year_fill_start + 1);
        }
        else {
          storage_min_start_hyd_year = 0.1; // change it --Tian
          storage_optimal_start_hyd_year = 0.8;
        }

        if (yy == year_fill_start && month == 1) {
          fill_flag = 1;//start filling reservoir
          printf("Reservoir filling begun: %d %f %f \n",
            yy, storage_min_start_hyd_year, storage_optimal_start_hyd_year);
        }
        else {
          if (month == 1 && DATE[i].day == 1)
            printf("Reservoir filling flag=%d; storage_min=%.2f; storag_opt=%.2f\n",
              fill_flag, storage_min_start_hyd_year, storage_optimal_start_hyd_year);
        }
        /* if first day */
        if (i == 1) {
          oldstorage = storage[0];
          storage[i] = oldstorage;
        }
        else storage[i] = storage[i - 1];

        avail_water[i] = storage[i];

        if (month != start_month || i > (ndays - 365) || yy < year_fill_start) {
          /* Not yet approached beginning of first operational year,
             or after the last operational year.
             Let inflow go directly to outflow.
          */
          outflow[i] = FLOW[i] * CONV_M3S_CM; //m3day-1
          storage[i] = oldstorage;
          oldstorage = storage[i];
          head = height - (capacity - storage[i]) / surf_area;
          R_FLOW[i] = outflow[i] / CONV_M3S_CM; /*R_FLOW in m3s-1 */
          STORAGE[i] = storage[i]; /* storage is in m3 */
          LEVEL[i] = head;
          
          printf("storage[%d]=%.2f\n", i, storage[i]);
        } /* end if not yet at beginning of operational year, or before dam is built */

        else { /*Start calculations for the coming operational year.
             Calculating inflow the coming operational year
             At this point q7_10 is only known outflow */
          printf("\n==================================================\n");
          printf("\n\nStart calculation for the operational year:");
          printf("\nYY[%d]MM[%d]JDAY[%d]\nstorage=%.1f; availwater=%.1f; storage_min=%.2f; storage_optimal=%.2f\n",
            yy, month, i, storage[i], avail_water[i], storage_min_start_hyd_year, storage_optimal_start_hyd_year);
          printf("\n==================================================\n");

          if (month <= 2) 
            leapyear = IsLeapYear(yy);
          else 
            leapyear = IsLeapYear(yy + 1);

          for (j = 0; j < 365 + leapyear; j++) {
            /* Calculate next year's inflow, reservoir evaporation,
              and available water each day in the coming year.
              Reservoir evaporation calculated in VIC.
              Taken from naturalized VIC simulations
              RESEVAPDATA[i][0]=actual evaporation in cell, before reservoir (mm/day)
              RESEVAPDATA[i][1]=airtemp (C)
              RESEVAPDATA[i][2]=wind (m/s)
              RESEVAPDATA[i][3]=potential lake evaporation (mm/day) */
              //if(RESEVAPDATA[i+j][0] > RESEVAPDATA[i+j][3]) 
            res_evap[i + j] = 0.;
            //else 
            //res_evap[i+j]=(RESEVAPDATA[i+j][3]-RESEVAPDATA[i+j][0])/1000.;  
            /* m/day. additional evaporation caused by reservoir (mm/day) from vic simulations.
            i.e. what is extracted is additional evap caused by
           conversion to open water. Nothing is added/subtracted if VIC grid cell
            evapotranspiration is higher than open water evaporation. */
            tot_res_evap += res_evap[i + j]; // m
            if (j > 0)
              res_evap_accumulated[j] +=
              res_evap_accumulated[j - 1] + res_evap[i + j] * surf_area; // m3 ok?
            else
              res_evap_accumulated[j] = res_evap[i + j] * surf_area; // m3 ok?
            year_mean_inflow += FLOW[i + j] * CONV_M3S_CM; // FLOW in m3s-1
            if (j == 0)
              avail_water[i + j] =
              storage[i + j] + FLOW[i + j] * CONV_M3S_CM - q7_10 - surf_area*res_evap[i + j]; // m3
            else
              avail_water[i + j] =
              avail_water[i + j - 1] + FLOW[i + j] * CONV_M3S_CM - q7_10 - surf_area*res_evap[i + j]; // m3
          } // end inflow and reservoir evaporation calculations coming year ......

          year_mean_inflow /= j; /*units m3day-1, i.e. mean inflow the coming year */
          printf("ResRout - YY[%d]MM[%d]: mean inflow coming year=%.2f (m3 day-1);\n",
            yy, month, year_mean_inflow);

          /* First: Set irrigation water demands = 0 m3day-1 */
          year_mean_water_demand = 0;
          this_year_demand = 0;
          for (k = 1; k <= 12; k++)
            actual_water_demand[k] = 0.;

          for (k = start_month; k <= 12; k++)
            year_mean_water_demand += (WATER_DEMAND[yy - start_year][k])*DaysInMonth[k] / (365 + leapyear);
          for (k = 1; k < start_month; k++)
            year_mean_water_demand += (WATER_DEMAND[yy + 1 - start_year][k])*DaysInMonth[k] / (365 + leapyear);
          fraction = year_mean_water_demand / (year_mean_inflow - q7_10); //All these numbers are in m3 day-1

          /* Split possible deficit (demand - inflow - 7q10) between outflow og storage.
             If reservoir more than 80% full: Everything is taken from storage.
             If  <80%: Half of it taken from storage and half from release (i.e available water lower than demand).
                 Minimum storage at end = 60 percent
          */
          if (fraction > 1) { /*Demand higher than inflow - allow storage to descrease a little */
            printf("\n*****ResRout: demand>infow*****\n");
            printf("mean annual demand=%.0f m^3/day\ninflow=%.0f m^3/day\nq7_10=%.0f m^3/day\n", 
              year_mean_water_demand,  year_mean_inflow, q7_10);
            printf("Pre:fraction = %.2f, storage_fractions : %.2f\n",
              fraction, storage_fraction);
            storage_fraction =
              storage_fraction - (fraction - ((fraction-1)/2- 1)*year_mean_water_demand) / capacity;
            printf("Adjusted:storage_fractions: %.2f \n", storage_fraction);

            /* if current storage level is less than optimal level, split deficit between storage 
            and release, and adjust water demand if needed to maintain min storage level of reservoir */
            if ((storage[i] / capacity) <= (storage_optimal_start_hyd_year + 0.001)) {
              dummy_demand = 0.;
              factor = fraction - ((fraction - 1) / 2);
              for (k = start_month; k <= 12; k++) {
                /* reduce the demand */
                actual_water_demand[k] = WATER_DEMAND[yy - start_year][k] / factor;
                /* original water demand */
                dummy_demand += WATER_DEMAND[yy - start_year][k] * DaysInMonth[k] / (365 + leapyear);
              }
              for (k = 1; k < start_month; k++) {
                actual_water_demand[k] = WATER_DEMAND[yy + 1 - start_year][k] / factor;
                dummy_demand += WATER_DEMAND[yy + 1 - start_year][k]
                  * DaysInMonth[k] / (365 + leapyear); //corrected demand
              }
              dummy_demand *= (365 + leapyear);

              sum_actual_water_demand = 0.;
              for (k = 1; k <= 12; k++)
                sum_actual_water_demand += 
                actual_water_demand[k] * DaysInMonth[k] / (365 + leapyear);
              printf("Adjusted annual demand=%.0f m^3/day\n", sum_actual_water_demand);
              

              /* Comment out the original code - The conversion factor shouldn't be used again in the 
                 equation given that the unit of year_mean_inflowis m^3/day-- Ning */
              /* dummy = (storage[i] - dummy_demand + year_mean_inflow*CONV_M3S_CM*(365 + leapyear)) / capacity;
              if (dummy < storage_min_start_hyd_year) {
                //factor = (storage[i] - storage_min_start_hyd_year*capacity + year_mean_inflow*CONV_M3S_CM*(365 + leapyear)) / dummy_demand;
                factor = (storage[i] - storage_min_start_hyd_year*capacity + year_mean_inflow*(365 + leapyear)) / dummy_demand;
                for (k = 1; k <= 12; k++)
                  actual_water_demand[k] = WATER_DEMAND[yy - start_year][k] * factor;
              } /*


              /* edited (ning) */              
              /* If current storage plus flow of next year minus next year's (pre-adjusted) demand is less than
                 storage_min_start_hyd_year*storage capaciaty: reduce demand so that the storage stays above 
                 storage=storage_min_start_hyd_year. If doing so still cannot maintain min storage, reduce demand
                 to zero.  */
              printf("\n**Further adjustment due to low water storage level**\n");
              dummy = (storage[i] - dummy_demand + year_mean_inflow*(365 + leapyear)) / capacity;
              printf("Pre: storage=%.3f, inflow=%.3f, capacity=%.3f, demand=%.3f km^3\n", 
                storage[i]/km3tom3, year_mean_inflow*(365+leapyear)/km3tom3, 
                capacity/km3tom3, dummy_demand/km3tom3);
              if (dummy < storage_min_start_hyd_year) {
                factor = (storage[i] - storage_min_start_hyd_year*capacity + year_mean_inflow*(365 + leapyear)) / dummy_demand;
                
                /* if factor<0, set factor to 0. This happens before reservoir is filled to optimal level. */
                if (factor < 0)
                  factor = 0.;
                for (k = 1; k <= 12; k++)
                  actual_water_demand[k] = WATER_DEMAND[yy - start_year][k] * factor;
              }
              sum_actual_water_demand = 0.;
              for (k = 1; k <= 12; k++)
                sum_actual_water_demand += actual_water_demand[k] * DaysInMonth[k];
              printf("Adjusted: storage=%.3f, inflow=%.3f, capacity=%.3f, demand=%.3f km^3\n",
                storage[i]/km3tom3, year_mean_inflow*(365+leapyear)/km3tom3, 
                capacity/km3tom3, sum_actual_water_demand/km3tom3);
            }
            else { /* fraction >1, i.e. demand > inflow, but 
                 storage/capacity > storage_optimal_start_hyd_year
                 In this case, allow demand to be 100% met/released */
              for (k = start_month; k <= 12; k++)
                actual_water_demand[k] = WATER_DEMAND[yy-start_year][k];
              for (k = 1; k < start_month; k++)
                actual_water_demand[k] = WATER_DEMAND[yy+1-start_year][k];
            }
          } /* end if(fraction > 1)*/

          else { /*Demand less than inflow, i.e. fraction <1
               Allow entire demand to be released */
            storage_fraction =
              storage_fraction - (fraction - ((fraction - 1) / 2 - 1)*year_mean_water_demand) / capacity;
            printf("ResRout: demand<inflow (storage_fractions=%.2f; demand/inflow=%.2f)\n",
              storage_fraction, fraction);
            /* if in or after operational month, adjust according to current-year monthly demand */
            for (k = start_month; k <= 12; k++)
              actual_water_demand[k] = WATER_DEMAND[yy-start_year][k];
            /* if before operational month, adjust according to demand of next year*/
            for (k = 1; k < start_month; k++)
              actual_water_demand[k] = WATER_DEMAND[yy+1-start_year][k];
          } /* End if else fraction>1 */;

          storage_fraction = max(storage_fraction, storage_min_start_hyd_year);
          storage_fraction = min(storage_fraction, storage_optimal_start_hyd_year);
          printf("ResRout: storage_fractions=%.2f (adjusted by storage threshold)\n", storage_fraction);

          /* Compare original demand to corrected demand */
          this_year_demand = 0;
          for (k = 1; k <= 12; k++) {
            /* if in or after operational month, adjust according to current-year monthly demand */
            if (k < start_month)
              printf("orig water demand MM[%d]=%.1f; corr_demand=%.1f (m3day-1);\n",
                k, WATER_DEMAND[yy + 1 - start_year][k], actual_water_demand[k]);
            /* if before operational month, adjust according to demand of next year*/
            else 
              printf("orig water demand MM[%d]=%.1f; corr_demand=%.1f (m3day-1);\n",
              k, WATER_DEMAND[yy - start_year][k], actual_water_demand[k]);
            this_year_demand += actual_water_demand[k] * DaysInMonth[k] / (365 + leapyear);
          }

          printf("This year's original demand YY[%d]=%.2f; corrected demand YY[%d] = %.2f m3/day \n", yy, year_mean_water_demand, yy, this_year_demand);
          printf("Demand is %.1f%% of inflow (%.1f m3/day) this year\n", this_year_demand*100/year_mean_inflow, year_mean_inflow);
          printf("ResRout storage_fractions B: %.3f min %f optimal %.3f\n\n",
            storage_fraction, storage_min_start_hyd_year, storage_optimal_start_hyd_year);

          for (j = 0; j < 366; j++)
            minflow[j] = q7_10;

          oldmonth = DATE[i].month;
          newmonth = i;
          start_storage = storage[i];

          /*****************end water demand calculations this year*************************************/

          /*****************start moscem IRR this year**************************************************/
          if (purp_irr == 1 && moscem == 1) {
            /* print to file for use in moscem: inflow, minoutflow, irrigation water demands,
               reservoir evaporation, storage, and mean_flood for each day. All numbers in m or m3day-1 */
            sprintf(infile_path, "%s%s", moscem_path, "infile.txt");
            sprintf(junk, "%s%s", "rm -rf ", infile_path);
            if ((status = system(junk)) == 0) {
              printf("status(rm -rf %s) = %d\n", infile_path, status);
            }
            if ((fin = fopen(infile_path, "w")) == NULL) {
              printf("Cannot open file %s for writing (ReservoirRouting IRR)\n", infile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s (ReservoirRouting IRR)\n", infile_path);

            for (j = 0; j < 365 + leapyear; j++) {
              month = DATE[i + j].month;
              day = DATE[i + j].day;
              year = DATE[i + j].year;
              fprintf(fin, "%d %d %d %f %f %f %f %f %f\n",
                year, month, day, FLOW[i + j] * CONV_M3S_CM, minflow[j], actual_water_demand[month], res_evap[i + j], mean_flood, 0.);
            }
            fclose(fin);
            sprintf(valfile_path, "%s%s", moscem_path, "valfile.txt");
            sprintf(junk, "%s %s  %s", "cp", infile_path, valfile_path);
            system(junk);

            sprintf(rescarfile_path, "%s%s", moscem_path, "rescarfile.txt");
            sprintf(junk, "%s%s", "rm -rf ", rescarfile_path);
            system(junk);
            if ((fres = fopen(rescarfile_path, "w")) == NULL) { // Used by moscem
              printf("Cannot open file for writing: %s (ReservoirRouting IRR)\n", rescarfile_path);
              exit(1);
            }
            else printf("File opened for writing: %s\n", rescarfile_path);
            fprintf(fres, "%.1f %.1f %.1f %.1f %d %d %.3f %.2f  %d\n",
              surf_area, capacity, start_storage, storage_fraction*capacity, height, 0,
              inst_cap, mean_flood, 6); //opt scheme = 6 = IRR
            fclose(fres);

            sprintf(rescarfile_irr_path, "%s%s", moscem_path, "rescarfile.irr.total");
            sprintf(junk, "%s %s %s %s", "cat", rescarfile_path, ">>", rescarfile_irr_path);
            system(junk);
            end_storage = storage_fraction*capacity;
            printf("ReservoirRouting.c YY[%d] start_storage=%.2f km3; end_storage aims at %.2f km3 which is %.2f of capacity\n",
              yy, start_storage/km3tom3, end_storage/km3tom3, storage_fraction);

            if (leapyear == 0)
              status = system("./run_moscem_normalyear");
            else
              status = system("./run_moscem_leapyear");

            /* read moscem run results: year,month,day,sim_inflow,sim_outflow,power_production,spill,storage,
               storage/max_storage,minflow (7q10),waterdemand(irrig).
               All numbers in m3 or m3day-1. Not all is read, though! */
            if ((fmoscem = fopen(moscem_outfile, "r")) == NULL) {
              printf("Cannot open file %s (ReservoirRouting IRR)\n", moscem_outfile);
              exit(1);
            }
            else  printf("File opened for reading: %s (ReservoirRouting IRR)\n", moscem_outfile);
            for (j = 0; j < (365 + leapyear); j++) {
              fscanf(fmoscem, "%d %d %d %*f %f %*f %*f %f %*f %f %f\n",
                &year, &month, &day, &outflow[j], &storage[i + j], &minflow[j], &waterdemanddaily[j]);

              if (outflow[j] < (waterdemanddaily[j] + minflow[j])) 
                minflow[j] = outflow[j];
              else
                minflow[j] += waterdemanddaily[j]; //update minflow for later use! i.e. increase if appropriate. 
            }
            fclose(fmoscem);
          }

          /*********************end moscem IRR this year *******************************************/

          /*****************start moscem FLOOD this year**********************************************/
          if (purp_flood == 1 && moscem == 1) {

            /* print to file for use in moscem: inflow, minoutflow, irrigation water demands,
               reservoir evaporation, storage, and mean_flood for each day.
               All numbers in m or m3day-1 */
            sprintf(infile_path, "%s%s", moscem_path, "infile.txt");
            sprintf(junk, "%s%s", "rm -rf ", infile_path);
            system(junk);
            if ((fin = fopen(infile_path, "w")) == NULL) {
              printf("Cannot open file %s for writing (ReservoirRouting Flood Control)\n", infile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s (ReservoirRouting Flood Control)\n", infile_path);
            for (j = 0; j < 365 + leapyear; j++) {
              month = DATE[i + j].month;
              day = DATE[i + j].day;
              year = DATE[i + j].year;
              fprintf(fin, "%d %d %d %f %f %f %f %f %f\n",
                year, month, day, FLOW[i + j] * CONV_M3S_CM, minflow[j], 0.,
                res_evap[i + j], mean_flood, 0.);
              /* remember: minflow is now possibly q7_10 + irr water release!!!!
             after previous moscem. and hence col6=waterdemand=0. */
            }
            fclose(fin);

            /* copy file to file */
            sprintf(valfile_path, "%s%s", moscem_path, "valfile.txt");
            sprintf(junk, "%s %s  %s", "cp", infile_path, valfile_path);
            system(junk);

            /* writing file holding reservoir characteristics */
            sprintf(rescarfile_path, "%s%s", moscem_path, "rescarfile.txt");
            sprintf(junk, "%s%s", "rm -rf ", rescarfile_path);
            system(junk);
            if ((fres = fopen(rescarfile_path, "w")) == NULL) { // Used by moscem
              printf("Cannot open file for writing: %s (ReservoirRouting FLOODing)\n", rescarfile_path);
              exit(1);
            }
            else printf("File opened for writing: %s\n", rescarfile_path);

            fprintf(fres, "%.1f %.1f %.1f %.1f %d %d %.3f %.2f  %d\n",
              surf_area, capacity, start_storage, storage_fraction*capacity, height, 0,
              inst_cap, mean_flood, 7); //opt scheme = 7 = FLOOD
            fclose(fres);

            end_storage = storage_fraction*capacity;
            printf("ReservoirRouting.c start_storage %f endstorage aims at %.2f which is %.1f of capacity\n",
              start_storage, end_storage, storage_fraction);

            if (leapyear == 0)
              system("./run_moscem_normalyear");
            else
              system("./run_moscem_leapyear");

            /* read results from moscem runs. in file: year,month,day,inflow,sim_outflow,power_prod (MW),
               spill,storage,storage/max_storage,minoutflow,waterdemand. All numbers in m or m3day-1 */
            if ((fmoscem = fopen(moscem_outfile, "r")) == NULL) {
              printf("Cannot open file %s (ReservoirRouting FLOOD)\n", moscem_outfile);
              exit(1);
            }
            else  printf("File opened: %s (ReservoirRouting FLOOD)\n", moscem_outfile);
            for (j = 0; j < (365 + leapyear); j++) {
              fscanf(fmoscem, "%d %d %d %*f %f %*f %*f %f %*f %f %f\n",
                &year, &month, &day, &outflow[j], &storage[i + j], &minflow[j], &waterdemanddaily[j]);
            }
            fclose(fmoscem);
          }

          /*********************end moscem FLOOD this year *******************************************/

          /*****************start moscem POW this year************************************************/
          if (purp_hydro == 1 && moscem == 1) {
            /* print to file for use in moscem: inflow, minoutflow, irrigation water demands,
               reservoir evaporation, storage, and mean_flood for each day.
                   All numbers in m or m3day-1 */
            sprintf(infile_path, "%s%s", moscem_path, "infile.txt");
            sprintf(junk, "%s%s", "rm -rf ", infile_path);
            printf("infile.txt path: %s\n", infile_path);
            system(junk);
            if ((fin = fopen(infile_path, "w")) == NULL) {
              printf("Cannot open file %s for writing (ReservoirRouting HYDROPOWER)\n", infile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s (ReservoirRouting HYDROPOWER)\n", infile_path);

            for (j = 0; j < 365 + leapyear; j++) {
              month = DATE[i + j].month;
              day = DATE[i + j].day;
              year = DATE[i + j].year;
              fprintf(fin, "%d %d %d %f %f %f %f %f %f\n",
                year, month, day, FLOW[i + j] * CONV_M3S_CM, minflow[j], 0., res_evap[i + j], mean_flood, 0.);
              /* FLOW: inflow to res, in m3 day-1 (index 1 tom ndays....).
             minflow can now be q7_10 + irr water release!!!!
             6th col=0 (waterdemand), beacuase this happens after IRR. */
            }
            fclose(fin);

            //copy file to file
            sprintf(valfile_path, "%s%s", moscem_path, "valfile.txt");
            sprintf(junk, "%s %s  %s", "cp", infile_path, valfile_path);
            system(junk);

            /* writing file holding reservoir characteristics */
            sprintf(rescarfile_path, "%s%s", moscem_path, "rescarfile.txt");
            sprintf(junk, "%s%s", "rm -rf ", rescarfile_path);

            printf("rescarfile.txt path: %s\n", rescarfile_path);

            system(junk);
            if ((fres = fopen(rescarfile_path, "w")) == NULL) { // Used by moscem
              printf("Cannot open file for writing: %s (ReservoirRouting HYDROPOWER)\n", rescarfile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s\n", rescarfile_path);
            fprintf(fres, "%.1f %.1f %.1f %.1f %d %d %.3f %.2f  %d\n",
              surf_area, capacity, start_storage, storage_fraction*capacity, height, 0,
              inst_cap, mean_flood, 5); //opt scheme = 5 = POW
            fclose(fres);

            end_storage = storage_fraction*capacity;
            printf("ResRout start_storage %.1f endstorage aims at %.1f which is %.1f of capacity\n",
              start_storage, end_storage, storage_fraction);
            printf("ResRout now it's time for moscem (POW) year %d\n", year);

            if (leapyear == 0) 
              system("./run_moscem_normalyear");
            else  
              system("./run_moscem_leapyear");

            /* read results from moscem runs. in file: year,month,day,inflow,sim_outflow,power_prod (MW),
               spill,storage,storage/max_storage,minoutflow,waterdemand. All numbers in m or m3day-1 */
            if ((fmoscem = fopen(moscem_outfile, "r")) == NULL) {
              printf("Cannot open file for reading %s (ResRout POW)\n", moscem_outfile);
              exit(1);
            }
            else  printf("File opened for reading: %s (ResRout POW)\n", moscem_outfile);
            for (j = 0; j < (365 + leapyear); j++) {
              fscanf(fmoscem, "%d %d %d %*f %f %f %*f %f %*f %f %f\n",
                &year, &month, &day, &outflow[j], &PowerProd[i + j], &storage[i + j], &minflow[j], &waterdemanddaily[j]);
            }
            fclose(fmoscem);
          }

          /*********************end moscem POW this year ************************************************/

          /***************** finally, for all reservoirs without purp=hydro: start moscem WAT this year*****/
          if (purp_hydro != 1 && moscem == 1) {

            /* print to file for use in moscem: inflow, minoutflow, irrigation water demands,
               reservoir evaporation, storage, and mean_flood for each day. All numbers in m or m3day-1 */
            sprintf(infile_path, "%s%s", moscem_path, "infile.txt");
            sprintf(junk, "%s%s", "rm -rf ", infile_path);
            system(junk);
            if ((fin = fopen(infile_path, "w")) == NULL) {
              printf("Cannot open file %s for writing (ReservoirRouting WAT)\n", infile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s (ReservoirRouting WAT)\n", infile_path);


            for (j = 0; j < 365 + leapyear; j++) {
              month = DATE[i + j].month;
              day = DATE[i + j].day;
              year = DATE[i + j].year;
              fprintf(fin, "%d %d %d %f %f %f %f %f %f\n",
                year, month, day, FLOW[i + j] * CONV_M3S_CM, minflow[j], 0., res_evap[i + j], mean_flood, 0.);
              /* minflow can now be q7_10 + irr water release!!!!
             after moscem IRR. Which is why you set 6th column=0 (waterdemand) */
            }
            fclose(fin);

            /* copy file to file */
            sprintf(valfile_path, "%s%s", moscem_path, "valfile.txt");
            sprintf(junk, "%s %s  %s", "cp", infile_path, valfile_path);
            system(junk);

            /* writing file holding reservoir characteristics */
            sprintf(rescarfile_path, "%s%s", moscem_path, "rescarfile.txt");
            sprintf(junk, "%s%s", "rm -rf ", rescarfile_path);
            system(junk);
            if ((fres = fopen(rescarfile_path, "w")) == NULL) { // Used by moscem
              printf("Cannot open file for writing: %s (ReservoirRouting WATDEMAND)\n", rescarfile_path);
              exit(1);
            }
            else
              printf("File opened for writing: %s\n", rescarfile_path);
            fprintf(fres, "%.1f %.1f %.1f %.1f %d %d %.3f %.2f  %d\n",
              surf_area, capacity, start_storage, storage_fraction*capacity, height, 0,
              inst_cap, mean_flood, 9); //opt scheme = 9 = WAT
            fclose(fres);

            end_storage = storage_fraction*capacity;
            printf("ReservoirRouting.c start_storage %f endstorage aims at %.2f which is %.1f of capacity\n",
              start_storage, end_storage, storage_fraction);

            if (leapyear == 0)
              system("./run_moscem_normalyear");
            else
              system("./run_moscem_leapyear");

            /* read results from moscem runs. in file: year,month,day,inflow,sim_outflow,power_prod (MW),
               spill,storage,storage/max_storage,minoutflow,waterdemand. All numbers in m or m3day-1 */
            if ((fmoscem = fopen(moscem_outfile, "r")) == NULL) {
              printf("Cannot open file for reading %s (ReservoirRouting WAT)\n", moscem_outfile);
              exit(1);
            }
            else  printf("File opened for reading: %s (ReservoirRouting WAT)\n", moscem_outfile);
            for (j = 0; j < (365 + leapyear); j++) {
              fscanf(fmoscem, "%d %d %d %*f %f %*f %*f %f %*f %f %f\n",
                &year, &month, &day, &outflow[j], &storage[i + j], &minflow[j], &waterdemanddaily[j]);
            }
            fclose(fmoscem);
          }

          /*********************end moscem WAT this year *******************************************/

          /* Finally: End results for current operational year */
          for (j = 0; j < 365 + leapyear; j++) {
            outflow[j] /= CONV_M3S_CM; //go back to units m3s-1
            R_FLOW[i + j] = outflow[j];
            STORAGE[i + j] = storage[i + j]; // m3
            head = height - (capacity - storage[i + j]) / surf_area;
            LEVEL[i + j] = head;      //m 
          }  /* end final results this operational year. j=0,365 */

          printf("\nResRout Storage at end of operational year YY[%d]= %.2f which is %.2f%% of capacity\n",
            year, storage[i+j-1], storage[i+j-1]*100/capacity);

        } /* end loop of this operational year (if month=start_month) */

        /* move a year forward. i.e after start hyd year first year
          and before end hyd year last year */
        if (j > 0) 
          i += j - 1; 

      } /* end for i=0;i,ndays */


      printf("\tStorage at last time step in reservoir/dam %s: %f which is %.2f percent of capacity\n",
        resname, storage[ndays - 1], storage[ndays - 1] * 100 / capacity);
      printf("\tMean inflow %f (m3/year) is %f percent of capacity\n",
        mean_inflow * 365, mean_inflow * 365 * 100. / capacity);

    } //if(irow== & icol==)
  } //end while fscanf(fp)

  /* Free memory */
  free(outflow);
  free(storage);
  free(avail_water);
  free(max_release);
  free(power_demand);
  free(actual_water_demand);
}



