
# default values
MODEL_DIR=""
QUERY_PATH=""
OUTPUT_PATH=""
ROCCHIO_FLAG=""

# temp file names
PARSED_QUERY="tmp_parsed_query"
TOPIC_NUM="tmp_topic_num"
TMP_RANK_LIST="tmp_rank_list"

all:
	g++ -O2 wrap.cpp -o wrap.out
	g++ -O2 vsm.cpp -o a.out
run:
	@echo "parsing query with <parse_query.py> ..."
	@./parse_query.py $(MODEL_DIR) $(QUERY_PATH) $(PARSED_QUERY) $(TOPIC_NUM)
	@echo "(file created: <$(PARSED_QUERY)> and <$(TOPIC_NUM)>)"
	@echo "running VSM with <a.out> ..."
	@./a.out $(TMP_RANK_LIST) $(PARSED_QUERY) $(MODEL_DIR) $(ROCCHIO_FLAG)
	@echo "(file created: <$(TMP_RANK_LIST)>)"
	@echo "finalizing the ranking list with <finalize.py> ..."
	@./finalize.py $(TOPIC_NUM) $(TMP_RANK_LIST) $(OUTPUT_PATH)
	@echo "(file created: <$(OUTPUT_PATH)>)"
clean:
	@echo "cleaning up..."
	@rm -rf wrap.out a.out $(PARSED_QUERY) $(TOPIC_NUM) $(TMP_RANK_LIST)
	@echo "(temporary files removed)"
test:
	@echo "MODEL_DIR = $(MODEL_DIR)"
	@echo "QUERY_PATH = $(QUERY_PATH)"
