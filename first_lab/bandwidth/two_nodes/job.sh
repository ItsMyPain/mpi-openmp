#!/bin/bash

#PBS -l walltime=00:10:00,nodes=2:ppn=3
#PBS -N doctor_job
#PBS -q batch

cd $PBS_O_WORKDIR
export OMP_NUM_THREADS=$PBS_NUM_PPN
mpirun --hostfile $PBS_NODEFILE -np 6 ./test