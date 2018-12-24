#!/bin/bash

usage()
{
	echo $0 raw_ppm out_ppm
	exit 0
}

RAW_PNM=$1
OUT_PPM=$2

[ -n "${RAW_PNM}" ] || usage
[ -n "${OUT_PPM}" ] || usage

ppmquant 224 ${RAW_PNM} > /tmp/${RAW_PNM}.tmp || usage
pnmnoraw ${RAW_PNM}.tmp > ${OUT_PPM} 

