#!/bin/bash
#SBATCH -p comp
#SBATCH -N 1
#SBATCH --exclusive
source /home/PAC20214270/env.sh
source /opt/intel/oneapi/vtune/latest/vtune-vars.sh
timestamp=$(date +%Y%m%d_%H%M)
vtune -collect hotspots -result-dir "profile_$timestamp" -- ../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out combine_read.fq.gz --mismatch 2 --thread 64 > stat_$timestamp.txt
