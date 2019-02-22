## This is a shell script for running Smoldyn on a cluster. Replace 'mydir' with your directory name.

## declare a variable: name of the Smoldyn configuration file; needs to start with a letter
declare cnum="c1"

## declare an array variable: e.g. readfiles with different molecule starting positions
declare -a arr=("sphere" "triangle" "cube")

## loop through the above array; loop through 
for i in "${arr[@]}"; do
	for j in {1..30}; do
	sname="$cnum$i$j"
	qsub -V -cwd -o /mydir/"$cnum"/logs/ -e /mydir/"$cnum"/logs/ -N $sname -b y smoldyn /mydir/"$cnum"/"$cnum"_run.txt --define SIMNAME=$i --define SIMNUMBER=$j;
	echo $i,$j;
done
done
