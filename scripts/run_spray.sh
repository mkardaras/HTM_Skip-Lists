# USAGE: ./run.sh var
#  var: print statistics for transactions


var="lock"


bin=./../bin/htm-spray

#num_procs=(1 2 7 14 21 28 35 42 49 56)
num_procs=(1 7 14 21 28 42 56)

up_r=100

range=100000000 

initial_size=(1000000 10000 1000)


for k in "${initial_size[@]}"
do
	echo "-i $k -r $range -u $up_r"
	echo "#procs, locks, commits, aborts, forced, conflict, capacity, explicit, other"
	for i in "${num_procs[@]}"
	do
	    spray[$i]=`$bin -i $k -r $range -u $up_r -d 2000 -n $i | grep -A 7 $var | grep -o '[0-9.]*[0-9]*'`
	    echo $i, ${spray[$i]}
	done
	echo ""
done

