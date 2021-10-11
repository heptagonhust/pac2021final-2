#!/bin/bash
#SBATCH -p comp
#SBATCH -N 1
#SBATCH --exclusive
source /opt/soft/hdf5-1.10.6-gnu/env.sh
source /opt/soft/boost_1_70_0-gnu/env.sh
../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos.h5 --in1 V300091300_L03_read_1.fq.gz  --in2 V300091300_L04_read_1.fq.gz --out combine_read.fq.gz --mismatch 2 --thread 64 > stat.txt
