all:
	g++ main.cpp -o a.out
run:
	./parse_query.py ../model queries/query-test.xml parsed_query topic_num
	./a.out rank_file parsed_query ../model ../CIRB010
	./finalize.py topic_num rank_file final_rank_file.txt
clean:
	rm -rf a.out parsed_query topic_num rank_file
