#!/bin/bash

if [ "$#" -ne "4" ]; then
	echo "Please give root directory, text file, number of websites and number of pages"; exit 1
else
	rootDir=$1
	txtFile=$2
	wNumber=$3
	pNumber=$4
fi

# Check if wNumber and pNumber are integers

re='^[0-9]+$'

if ! [[ $wNumber =~ $re ]]; then
   echo "Error: ${wNumber} is not a number"; exit 1
fi

if ! [[ $pNumber =~ $re ]]; then
   echo "Error: ${pNumber} is not a number"; exit 1
fi

# Check if file exists
if [ ! -f "${txtFile}" ]; then
    echo "${txtFile} not found!"; exit 1
fi

# Check if there are enough lines in text file

if [ $(wc -l < ${txtFile}) -lt 10000 ]; then
  echo "Not enough lines in text file"; exit 1
fi

if [ -d "${rootDir}" ]; then
  # Control will enter here if directory exists
  if [ "$(ls -A "$rootDir")" ]; then
    echo "Directory is full, purging ..."
    rm -rf "${rootDir}"/*
  fi

total_pages=$((wNumber*pNumber))

page_ids=($(shuf -i 0-35000 -n $total_pages | sort -n))


k=$(shuf -i 2-$(( $(wc -l < ${txtFile})-1999)) -n 1) 	# 1 < k < lines in txtFile - 2000
m=$(shuf -i 1000-2000 -n 1) 	# 1000 < m < 2000

start=0

# Creating directories and storing (html) page names

for i in $( seq 0 $((wNumber-1)) )
do 	
	dir_array[i]="${rootDir}/site${i}"
	for j in $( seq $start $((pNumber-1+start)) )
	do
		array[j]="${rootDir}/site${i}/page${i}_${page_ids[$j]}.html"
	done
	
	start=$((j+1))
done

start=0
f=$(( pNumber/2 + 1))
q=$(( wNumber/2 + 1))

# Creating inLinks and exLinks and writing content to pages

for i in $( seq 0 $((wNumber-1)) )
do 
	echo "Creating web site ${i} ..."
	mkdir "${rootDir}/site${i}"
	for j in $( seq $start $((pNumber-1+start)) )
	do
		inLink_array=( $(for l in $( seq $start $((pNumber-1+start)) )
		do
			if [ "${array[l]}" != "${array[j]}" ]
			then 
				echo "${array[l]}"
			fi
		done ))
		
		inLink_array=( $(shuf -e "${inLink_array[@]}" -n $(( pNumber/2 + 1)))) # f=$(( pNumber/2 + 1))
		
		
		#echo "${array[j]}"
		#echo "inLinks"
		#echo "${inLink_array[@]}"
		#echo
		
		start2=0
		exLink_array=( $(for n in $( seq 0 $((wNumber-1)) )
		do
			if [ "${dir_array[n]}" != "${rootDir}/site${i}" ]; then
				for l in $( seq $start2 $((pNumber-1+start2)) )
				do
					echo "${array[l]}"
				done
				start2=$((l+1))
			else
				start2=$((start2 + pNumber*(n+1)))
			fi	
		done))
		
		exLink_array=( $(shuf -e "${exLink_array[@]}" -n $(( wNumber/2 + 1)))) # q=$(( wNumber/2 + 1))
		
		#echo "exLinks"
		#echo "${exLink_array[@]}"
		#echo
		echo "	Creating page ${array[j]} with $(((m/(f + q)) * (f + q) + f + q)) lines starting at line ${k} ..."
		touch "${array[j]}"
		
		header=$'<!DOCTYPE html>\n<html>\n<body>\n'
		echo "${header}" > "${array[j]}"
		
		
		temp1="$k"
		temp2="$((temp1 + m/(f + q)))"
		
		
		for n in $( seq 0 $((f+q-1)) )
		do
			sed -n ''"${temp1}"','"${temp2}"p'' "${txtFile}" >> "${array[j]}"
			echo "<br>" >> "${array[j]}"
			if [ "${n}" -lt "$f" ]; then
				echo "	Adding Inlink to ${inLink_array[${n}]}"
				echo "<a href=\"${inLink_array[${n}]}\">${inLink_array[${n}]}</a>" >> "${array[j]}"
				echo "<br>" >> "${array[j]}"
			else
				counter=$((n-f))
				echo "	Adding Exlink to ${exLink_array[${counter}]}"
				echo "<a href=\"${exLink_array[${counter}]}\">${exLink_array[${counter}]}</a>" >> "${array[j]}"
				echo "<br>" >> "${array[j]}"	
			fi
		
			temp1="$temp2"
			temp2=$((temp1 + m/(f + q)))
		done
		
		header=$'</body>\n</html>'
		echo "${header}" >> "${array[j]}"
		
	done
	
	start=$((j+1))
done

echo "Finished"
  
else
	echo "Directory ${rootDir} does not exist"; exit 1
fi

exit 0