#!/bin/bash

#PBS -l walltime=00:10:00,nodes=7:ppn=4
#PBS -N doctor_job
#PBS -q batch

cd $PBS_O_WORKDIR
export OMP_NUM_THREADS=$PBS_NUM_PPN
mpirun --hostfile $PBS_NODEFILE -pernode ./test p46gun.cfg "p46gun_20.dat"

# for np in {18..28}
# do
# 	mpirun --hostfile $PBS_NODEFILE -pernode ./test p46gun.cfg "p46gun_20.dat"
#done
