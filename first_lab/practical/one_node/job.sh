#!/bin/bash

#PBS -l walltime=00:10:00,nodes=1:ppn=2
#PBS -N doctor_job
#PBS -q batch

cd $PBS_O_WORKDIR
export OMP_NUM_THREADS=$PBS_NUM_PPN
mpirun --hostfile $PBS_NODEFILE -np 2 ./test