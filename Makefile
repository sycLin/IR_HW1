all:
	g++ -O2 vsm.cpp -o a.out
run:
	@echo "parsing query with <parse_query.py> ..."
	@./parse_query.py model/ query-test.xml parsed_query topic_num
	@echo "(file created: <parsed_query> and <topic_num>)"
	@echo "running VSM with <a.out> ..."
	@./a.out rank_file parsed_query model/
	@echo "(file created: <rank_file>)"
	@echo "finalizing the ranking list with <finalize.py> ..."
	@./finalize.py topic_num rank_file final_rank_file.txt
	@echo "(file created: <final_rank_file.txt>)"
clean:
	rm -rf a.out parsed_query topic_num rank_file
