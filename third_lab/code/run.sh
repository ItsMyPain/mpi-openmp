#!/bin/bash

mpicc -o a.out life2d.c

for config in *.cfg
do
    mpirun ./a.out $config "output/${config%%.*}.dat"
done

# for config in *.cfg
# do
#     for np in {1..1}
#     do
#         for i in {1..1}
#         do
#         mpirun -np $np ./a.out $config "output/${config%%.*}.dat"
#         done
#     done
# done
