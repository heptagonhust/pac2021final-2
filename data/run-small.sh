#!/bin/bash
../ST_BarcodeMap-0.0.1 --in DP8400016231TR_D1.barcodeToPos --in1 V300091300_L03_read_1.fq.gz --in2 V300091300_L04_read_1.fq.gz --out combine_read.fq.small --mismatch 2 --thread 4 1> stat-small.txt 2>err.txt
