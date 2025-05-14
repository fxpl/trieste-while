#!/bin/bash

# This ugly script runs the partial and full randomization generation pyton script 
# and following data runs the Trieste parser and static analysier on it
# Most of the ugly commands are to easy format the output for latex
# Feel free reformat this

function gather_data() {
	cat tmp.txt | awk '!/Starting/ && (/INST POST NORM:/ || /VARS POST NORM:/ || /functions/ || /expressions/ || /statements/ || /check_refs/ || /unique_variables/ || /gather/ ||/constant_folding/ || /dead_code_el/) ' \
		| awk '{
			if ($1 == "INST") inst += $4;
			else if ($1 == "VARS") vars += $4;
			else if ($1 == "functions" || $1 == "expressions" || $1 == "statements" || $1 == "check_refs" || $1 == "unique_variables") pr += $4;
			else if ($1 == "gather_functions" || $1 == "gather_instructions" || $1 == "gather_flow_graph") cfg += $4;
			else if ($1 == "constant_folding") cp += $4;
			else if ($1 == "dead_code_elimination") dce += $4;
			}
		END {
			print "INST ", int(inst);
			print "VARS ", int(vars);
			print "PARSE ", int(pr / 1000);
			print "CONTROL ", int(cfg / 1000);
			print "CP ", int(cp / 1000);
			print "DCE ", int(dce / 1000);
		}' >> tmp2.txt
}


function to_stats() {
	local divisor=${1}

	cat tmp2.txt | gawk -v div="$divisor" '{
			if ($1 == "INST") { inst += $2; inst2 += $2 * $2; }
			else if ($1 == "VARS") { vars += $2; vars2 += $2 * $2; }
			else if ($1 == "PARSE") { pr += $2; pr2 += $2 * $2; }
			else if ($1 == "CP") { cp += $2; cp2 += $2 * $2; }
			else if ($1 == "CONTROL") { cfg += $2; cfg2 += $2 * $2; }
			else if ($1 == "DCE") { dce += $2; dce2 += $2 * $2; }

		}
		function stderr(sum, sumsq, n) {
			return (n > 1) ? sqrt(sumsq/n - (sum/n)^2) : 0
		}	
		END {
			printf "INST %d+=%.0f \n", int(inst / div), stderr(inst, inst2, div);
			printf "VARS %d+=%.0f \n", int(vars / div), stderr(vars, vars2, div);
			printf "PARSE %d+=%.0f \n", int(pr / div), stderr(pr, pr2, div);
			printf "CONTROL %d+=%.0f \n", int(cfg / div), stderr(cfg, cfg2, div);
			printf "CP %d+=%.0f \n ", int(cp / div), stderr(cp, cp2, div);
			printf "DCE %d+=%.0f \n", int(dce / div), stderr(dce, dce2, div);

		}' >> stats_result_graph.txt
}

runs=20;
loc=(1000 2500 5000 7500 10000 12500 15000 17500 20000)

graph_header="LOC Inst Inst-Std Vars Vars-Std Parsing Parsing-Std CFG CFG-Std Constant-Prop Constant-Prop-Std Live-Dead Live-Dead-Std end"
table_header="LOC Inst Vars Parsing CFG Constant-Prop Live-Dead  end"

echo "Running partial generation"
echo "Partial mode" > stats_result_graph.txt
echo $graph_header >> stats_result_graph.txt
for i in ${loc[@]}; do
	echo "" > tmp2.txt
	echo "With -loc = $i"
	echo "$i " >> stats_result_graph.txt
	for ((j = 1; j <= $runs; j++)); do
		echo "round: $j"
		./program_generation -loc $i -p true > tmp.txt
		gather_data
	done
	to_stats $runs
	echo "end" >> stats_result_graph.txt
done

echo "Running full generation"
echo "Full mode" >> stats_result_graph.txt
echo $graph_header >> stats_result_graph.txt
for i in ${loc[@]}; do
	echo "" > tmp2.txt
	echo "With -loc = $i"
	echo "$i " >> stats_result_graph.txt
	for ((j = 1; j <= $runs; j++)); do
		echo "round: $j"
		./program_generation -loc $i -f true > tmp.txt
		gather_data
	done
	to_stats $runs
	echo "end" >> stats_result_graph.txt
done
#
#
# # Formatting for latex
sed -i -e 's/\(CP\|DCE\|PARSE\|CONTROL\|VARS\|INST\) //'  -e 's/end/\\\\/'  ./stats_result_graph.txt
tr -d '\n'  < stats_result_graph.txt | sed -e 's/\\\\\|mode/\n/g'> tmp.txt && cat tmp.txt > stats_result_graph.txt

sed -e 's/[A-Za-z-]\+-Std//g' ./stats_result_graph.txt > ./stats_result_table.txt
sed -i -e 's/+=/ /g' ./stats_result_graph.txt
sed -i -e 's/+=/$\\pm$/g' ./stats_result_table.txt

rm tmp.txt
rm tmp2.txt
cat stats_result_graph.txt
cat stats_result_table.txt
