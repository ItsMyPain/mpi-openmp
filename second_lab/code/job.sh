#!/bin/bash

#PBS -l walltime=0:10:00,nodes=7:ppn=4
#PBS -N doctor_job
#PBS -q batch

cd $PBS_O_WORKDIR
export OMP_NUM_THREADS=$PBS_NUM_PPN

for np in {20..28}
do
     	mpirun --hostfile $PBS_NODEFILE -np $np ./test config/p46gun.cfg "output/p46gun_20.dat"
done