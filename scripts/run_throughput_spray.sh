# USAGE: ./run_throughput_spray.sh var
#  var: print statistics for indicated variable (default #txs)
#       need complete enough name for grep to find unambiguously (e.g. '#txs' works, 'txs' doesn't)

if [ -z $1 ]
then
var="\#txs"
else
var=$1
fi

bin1=./../bin/spray
bin2=./../bin/htm-spray

#num_procs=(1 2 7 14 21 28 35 42 49 56)
num_procs=(2 7 14 21 28 35 42 49 56)

up_r=100

range=100000000 

initial_size=(1000000 10000 1000)

for k in "${initial_size[@]}"
do
	echo "update rate : $up_r % , initial length : $k , range : $range"
	echo "#procs, spray, spray_htm_fg, spray_htm_cg"
	for i in "${num_procs[@]}"    
	do
	    spray1[$i]=`$bin1  -i $k -r $range -u $up_r -d 1000 -n $i -s 60 -l| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    spray2[$i]=`$bin2  -i $k -r $range -u $up_r -d 1000 -n $i -s 60 | grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    spray3[$i]=`$bin2  -i $k -r $range -u $up_r -d 1000 -n $i -s 60 -c | grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    echo $i, ${spray1[$i]}, ${spray2[$i]}, ${spray3[$i]}
	done
	echo ""
done
    

