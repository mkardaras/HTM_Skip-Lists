# USAGE: ./run.sh var
#  var: print statistics for transactions


var="lock"


bin=./../bin/htm-sl

#num_procs=(1 2 3 4 5 6 7 14 28 56)
#num_procs=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 35 42 49 56)
#num_procs=(1 2 3 4 5 6 7 8)
#num_procs=(1 2 4 8 16 32 40 64 80)
#num_procs=(8 16 32 64)
num_procs=(1 7 14 21 28 42 56)
#num_procs=(1 7 14 28 42 56 70 84 98)
#num_procs=(1 2 11 22 44)
#num_procs=(80)

#update_rates=(100 80 50 20 5 0)
#update_rates=(5 20 50)
update_rates=(5 50 100)

initial_size=(1000000 100000 10000 1000)
#initial_size=(1000000 100000)


for k in "${initial_size[@]}"
do
    #m=`expr 2 \\* $k`
    for j in "${update_rates[@]}"
    do
	echo ""
	echo "-i $k -r 100000000 -u $j -A"
	echo "#procs, locks, commits, aborts, forced, conflict, capacity, explicit, other"
	for i in "${num_procs[@]}"
	do
	    skip1[$i]=`$bin -i $k -r 100000000 -u $j -d 1000 -n $i -f 15 -x 30 -A| grep -A 7 $var | grep -o '[0-9.]*[0-9]*'`
	    echo $i, ${skip1[$i]}
	done
    done
done

for k in "${initial_size[@]}"
do
    #m=`expr 2 \\* $k`
    for j in "${update_rates[@]}"
    do
	echo ""
	echo "-i $k -r 100000000 -u $j -A -c"
	echo "#procs, locks, commits, aborts, forced, conflict, capacity, explicit, other"
	for i in "${num_procs[@]}"
	do
	    skip1[$i]=`$bin -i $k -r 100000000 -u $j -d 1000 -n $i -f 15 -x 30 -A -c| grep -A 7 $var | grep -o '[0-9.]*[0-9]*'`
	    echo $i, ${skip1[$i]}
	done
    done
done
