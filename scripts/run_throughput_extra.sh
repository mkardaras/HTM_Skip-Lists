# USAGE: ./run.sh var
#  var: print statistics for indicated variable (default #txs)
#       need complete enough name for grep to find unambiguously (e.g. '#txs' works, 'txs' doesn't)

if [ -z $1 ]
then
var="\#txs"
else
var=$1
fi

bin1=./../bin/sq-sl
bin2=./../bin/lb-sl_herlihy
bin3=./../bin/lf-sl_herlihy
bin4=./../bin/htm-sl

#num_procs=(1 2 3 4 5 6 7 14 28 56)
#num_procs=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 35 42 49 56)
#num_procs=(1 2 3 4 5 6 7 8 10 12 14 16 20 24 28 32 36 40 44 48 52 56)
#num_procs=(1 2 7 14 28 35 37 39 40 41 42 43 44 45 49 56)
#num_procs=(1 7 14 28 42 56 70 84 98)
num_procs=(1 7 14 21 28 35 42 49 56)
#num_procs=(1 2 3 4 5 6 7 8)
#num_procs=(1 2 4 8 16 32 40 64 80)
#num_procs=(8 16 32 64)
#num_procs=(1 2 11 22 44)
#num_procs=(80)

#update_rates=(0 5 20 50 80 100)
update_rates=(5 50 100)
#update_rates=(50 20 5)

#initial_size=(1000000 100000 10000 1000)
initial_size=(1000000 100000)

for k in "${initial_size[@]}"
do
    m=100000000
    #m=`expr 2 \\* $k`
    for j in "${update_rates[@]}"
    do
	echo "update rate : $j % , initial length : $k , range : $m , alternate"
	#echo "update rate : $j % , initial length : $k , range : $m"
	echo "#procs, sl_sec, sl_lb, sl_lf, sl_tx_fg, sl_tx_cg"
	for i in "${num_procs[@]}"    
	do
	    skip1[$i]=`$bin1  -i $k -r $m -u $j -d 1000 -n $i -s 60 -A| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    skip2[$i]=`$bin2  -i $k -r $m -u $j -d 1000 -n $i -s 60 -A| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    skip3[$i]=`$bin3  -i $k -r $m -u $j -d 1000 -n $i -s 60 -A| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    skip4[$i]=`$bin4  -i $k -r $m -u $j -d 1000 -n $i -f 15 -x 30 -A| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    skip5[$i]=`$bin4  -i $k -r $m -u $j -d 1000 -n $i -f 15 -x 30 -c -A| grep $var | grep '(?<= )[0-9]+\.?[0-9]+' -Po`
	    echo $i, ${skip1[$i]}, ${skip2[$i]}, ${skip3[$i]}, ${skip4[$i]}, ${skip5[$i]}  
	done
	echo ""
    done
    echo ""
    echo ""
done

