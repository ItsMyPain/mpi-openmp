#!/bin/bash

#PBS -l walltime=0:10:00,nodes=7:ppn=4
#PBS -N life

cd life
for config in *.cfg
do
    for np in {1..28}
    do
    	mpirun -hostfile $PBS_NODEFILE -np $np ./a.out $config "output/${config%%.*}.dat"
    done
done
