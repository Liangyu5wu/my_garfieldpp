#!/bin/bash
#
#SBATCH --account=atlas:default
#SBATCH --partition=roma
#SBATCH --job-name=he_ic4h10_gen
#SBATCH --output=output_he_ic4h10_gen-%j.txt
#SBATCH --error=error_he_ic4h10_gen-%j.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=40g
#SBATCH --time=10:00:00

cd /sdf/data/atlas/u/liangyu/on_chip_driftchamber/my_Garfield
source setup.sh
cd /sdf/data/atlas/u/liangyu/on_chip_driftchamber/my_Garfield/Examples/gas_generation/build

./generate_he_ic4h10