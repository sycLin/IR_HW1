#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // for sqrt(), log2()
#define UNI_RATIO 0.8

using namespace std;

//////////////////////
// global variables //
//////////////////////

/* for global settings */
string MODEL_DIR = "";
string OUTPUT_FILE_NAME = "";
string QUERY_FILE_NAME = "";
string NTCIR_DIR = "";
string DOC_LEN_FILE_NAME = "";

/* for global data */

// to store the total number of documents
int DOC_COUNT = 0;

// to store the list of documents, with
// key: document id (starting from 0)
// value: relative paths of the document
map<int, string> DOC_LIST;

// to store the vocabulary of unigram, with
// key: term id (starting from 1)
// value: the term represented with a string
map<int, string> UNI_VOCAB;

// to store the vocabulary of bigram, with
// key: std::pair of 2 int's indicating the bigram
// value: the term represented with a string
map<pair<int,int>, string> BI_VOCAB; // TODO: not yet fill in this. (do we have to?)

// to store the postings list for unigram in 2d map
// 1st key: term id
// 2nd key: doc id
// ultimate value: the number of times that term appears in that doc
map<int, map<int, int> > UNI_POSTINGS;

// to store the postings list for bigram in 2d map
// 1st key: std::pair of the 2 ids of the bigram
// 2nd key: doc id
// ultimate value: the number of times that bigram appears in that doc
map<pair<int, int>, map<int, int> > BI_POSTINGS;

// to store the document frequencies of terms for unigram
// key: the term id
// value: the document frequency of the term
map<int, int> UNI_DF;

// to store the document frequencies of terms for bigram
// key: the std::pair of the 2 term ids of the bigram
// value: the document frequency of the bigram term
map<pair<int, int>, int> BI_DF;

// to store the document lengths for unigram
// key: the doc id
// value: the length of that document
map<int, double> UNI_DOC_LENGTHS;

// to store the document lengths for unigram
// key: the doc id
// value: the length of that document
map<int, double> BI_DOC_LENGTHS;

// to store the physical lengths of every document
map<int, int> REAL_DOC_LENGTHS;


//////////////////////
// helper functions //
//////////////////////

void print_usage();
void build_vocabulary();
void build_document_list();
void build_real_doc_len();
void build_postings();
void build_document_lengths();
void read_query_and_search();
double compute_uni_similarity(vector<int> uni_query, int doc_id);
double compute_bi_similarity(vector< pair<int, int> > bi_query, int doc_id);
bool compare_score_pair(const pair<string, double>&i, const pair<string, double>&j);

///////////////////////////
// program entry: main() //
///////////////////////////

int main(int argc, char* argv[]) {
	// check argument
	// argv[1]: output file name
	// argv[2]: query file name
	// argv[3]: model dir
	// argv[4]: NTCIR dir
	// argv[5]: doc_len file
	if(argc != 6) {
		print_usage();
		exit(-1);
	}

	
	// set up from arguments

	cerr << "before set up: " << endl;
	cerr << "\tout = " << OUTPUT_FILE_NAME << endl;
	cerr << "\tq = " << QUERY_FILE_NAME << endl;
	cerr << "\tmodel = " << MODEL_DIR << endl;
	cerr << "\tNTCIR = " << NTCIR_DIR << endl;
	cerr << "\tdoc_len_file = " << DOC_LEN_FILE_NAME << endl;


	OUTPUT_FILE_NAME = string(argv[1]);
	QUERY_FILE_NAME = string(argv[2]);
	MODEL_DIR = string(argv[3]);
	NTCIR_DIR = string(argv[4]);
	DOC_LEN_FILE_NAME = string(argv[5]);

	cerr << "after set up: " << endl;
	cerr << "\tout = " << OUTPUT_FILE_NAME << endl;
	cerr << "\tq = " << QUERY_FILE_NAME << endl;
	cerr << "\tmodel = " << MODEL_DIR << endl;
	cerr << "\tNTCIR = " << NTCIR_DIR << endl;
	cerr << "\tdoc_len_file = " << DOC_LEN_FILE_NAME << endl;

	// build vocabulary (here, for unigram only)
	
	cerr << "build_vocabulary()" << endl;
	build_vocabulary();

	// build document list

	cerr << "build_document_list()" << endl;
	build_document_list();

	// get real document lengths from file

	cerr << "build_real_doc_len()" << endl;
	build_real_doc_len();

	cerr << "after build_read_doc_len, REAL_DOC_LENGTHS[100] = " << REAL_DOC_LENGTHS[100] << endl;
	exit(0);

	// build postings lists, DF, and DOC_LENGTHS

	build_postings();

	// read the query file, and do the searching

	read_query_and_search();

	
	return 0;
}

void print_usage() {
	cerr << "Usage:" << endl;
	cerr << "\t./a.out <output_path> <query_path> <model_dir_path> <NTCIR_dir_path> <doc_len_file>" << endl;
}

void build_vocabulary() {
	// construct the path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "vocab.all") : (MODEL_DIR + "/vocab.all");
	// open file and read it
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "cannot open file at: " << path << endl;
		exit(-1);
	}
	int counter = 0;
	while(!feof(fp)) {
		// read a line into tmp
		char* tmp = new char[1000];
		fscanf(fp, "%s", tmp);
		if(string(tmp) == "")
			break;
		// put into the UNI_VOCAB
		UNI_VOCAB[counter] = string(tmp);
		counter += 1;
	}
}

void build_document_list() {
	// construct the path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "file-list") : (MODEL_DIR + "/file-list");
	// open file and read it
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "cannot open file at: " << path << endl;
		exit(-1);
	}
	int counter = 0;
	while(!feof(fp)) {
		// read a line into tmp
		char* tmp = new char[1000];
		fscanf(fp, "%s", tmp);
		if(string(tmp) == "")
			break;
		// put into the DOC_LIST
		DOC_LIST[counter] = string(tmp);
		counter += 1;
	}
	DOC_COUNT = counter;
}

void build_real_doc_len() {
	// open file and read it
	FILE* fp = fopen(DOC_LEN_FILE_NAME.c_str(), "r");
	if(!fp) {
		cerr << "cannot open file at: " << DOC_LEN_FILE_NAME << endl;
		exit(-1);
	}
	int counter = 0;
	while(!feof(fp)) {
		int tmp;
		if(fscanf(fp, "%d", &tmp) == EOF)
			break;
		REAL_DOC_LENGTHS[counter] = tmp;
		counter += 1;
	}
}

void build_postings() {
	
	// initialize the doc lengths to 0.0
	for(int i=0; i<DOC_COUNT; i++) {
		UNI_DOC_LENGTHS[i] = 0.0;
		BI_DOC_LENGTHS[i] = 0.0;
	}

	// construct the path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "inverted-file") : (MODEL_DIR + "/inverted-file");
	
	// open file and read it
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "cannot open file at: " << path << endl;
		exit(-1);
	}
	int line_counter = 0;
	while(!feof(fp)) {
		
		// read the title line
		int t1, t2, N;
		if(fscanf(fp, "%d%d%d", &t1, &t2, &N) == EOF)
			break;
		line_counter += 1;
		if(line_counter % 10000 == 9999)
			cerr << "\rProcessed " << line_counter << " lines.";
		
		// fill in UNI_DF and BI_DF with title line
		if(t2 == -1) {
			// fill in UNI_DF
			UNI_DF[t1] = N;
		} else {
			// fill in BI_DF
			BI_DF[make_pair(t1, t2)] = N;
		}

		// read the content lines
		for(int i=0; i<N; i++) {
			int doc_id, count;
			fscanf(fp, "%d%d", &doc_id, &count);
			line_counter += 1;
			if(line_counter % 100000 == 99999)
				cerr << "\rProcessed " << line_counter << " lines.";
			// put into the postings lists: UNI_POSTINGS and BI_POSTINGS
			// and accumulate the doc len: UNI_DOC_LENGTHS and BI_DOC_LENGTHS
			// map<int, map<int, int> > UNI_POSTINGS;
			// map<pair<int, int>, map<int, int> > BI_POSTINGS;
			// map<int, double> UNI_DOC_LENGTHS;
			// map<int, double> BI_DOC_LENGTHS;
			if(t2 == -1) {
				// it's unigram
				UNI_POSTINGS[t1][doc_id] = count;
				UNI_DOC_LENGTHS[doc_id] += ((count * log2(DOC_COUNT / UNI_DF[t1])) * (count * log2(DOC_COUNT / UNI_DF[t1])));
			} else {
				// it's bigram
				BI_POSTINGS[make_pair(t1, t2)][doc_id] = count;
				BI_DOC_LENGTHS[doc_id] += ((count * log2(DOC_COUNT / BI_DF[make_pair(t1, t2)])) * (count * log2(DOC_COUNT / BI_DF[make_pair(t1, t2)])));
			}
		}
	}
	cerr << "\rProcessed " << line_counter << " lines." << endl;
	// finalize: doc length
	for(int i=0; i<DOC_COUNT; i++) {
		UNI_DOC_LENGTHS[i] = sqrt(UNI_DOC_LENGTHS[i]);
		BI_DOC_LENGTHS[i] = sqrt(BI_DOC_LENGTHS[i]);
	}
}

void read_query_and_search() {
	FILE* fp = fopen(QUERY_FILE_NAME.c_str(), "r");
	FILE* outfile = fopen(OUTPUT_FILE_NAME.c_str(), "w");
	if(!fp) {
		cerr << "cannot open query file at: " << QUERY_FILE_NAME << endl;
		exit(-1);
	}
	while(!feof(fp)) {
		
		// read the number of topics
		int topic_count;
		if(fscanf(fp, "%d", &topic_count) == EOF)
			break;
		cerr << "topic_count = " << topic_count << endl;
		
		// read in each topic
		for(int t = 0; t < topic_count; t++) {
			
			// read the unigram count and bigram count for this topic
			int u_count, b_count;
			fscanf(fp, "%d%d", &u_count, &b_count);
			
			// prepare two vectors to store the unigrams and bigrams
			vector<int> unigrams;
			vector< pair<int, int> > bigrams;

			// read in the unigrams and bigrams into the 2 vectors
			for(int i = 0; i < u_count; i++) {
				int tmp;
				fscanf(fp, "%d", &tmp);
				unigrams.push_back(tmp);
			}
			for(int i = 0; i < b_count; i++) {
				int tmp1, tmp2;
				fscanf(fp, "%d%d", &tmp1, &tmp2);
				bigrams.push_back(make_pair(tmp1, tmp2));
			}

			// compute the similarity (both unigrams and bigrams) w.r.t. each document
			vector< pair<string, double> > scores;
			for(map<int, string>::iterator it = DOC_LIST.begin(); it != DOC_LIST.end(); ++it) {
				// it->first: the key of the map, i.e., document id
				// it->second: the value of the map, i.e., the relative path of the doc
				double u_sim = compute_uni_similarity(unigrams, it->first);
				double b_sim = compute_bi_similarity(bigrams, it->first);
				// cerr << "b_sim = " << b_sim << endl;
				// the ratio is defined as a constant at the beginning of this file
				double total = (0.8) * u_sim + (0.2) * b_sim;
				// double total = b_sim;
				// double total = u_sim;
				// push into the vector: scores
				scores.push_back(make_pair(it->second, total));
			}

			// sort the scores vector
			sort(scores.begin(), scores.end(), compare_score_pair); // user-defined compare function

			// output the result
			int counter = 0;
			for(vector< pair<string, double> >::iterator it = scores.begin(); it != scores.end(); ++it) {
				fprintf(outfile, "%s\n", (it->first).c_str());
				counter += 1;
				if(counter == 100)
					break;
			}
			fprintf(outfile, "********\n");
			cerr << "\rProcessed " << t+1 << " topics.";
		}
		cerr << endl;
	}
	fclose(fp);
	fclose(outfile);
}

double compute_uni_similarity(vector<int> uni_query, int doc_id) {
	double similarity = 0.0;
	/*
	for term in query:
        if term in dictionary:
            similarity += inverse_document_frequency(term)*imp(term,id)
    similarity = similarity / length[id]
	*/
	for(vector<int>::iterator it = uni_query.begin(); it != uni_query.end(); ++it) {
		// similarity += (IDF) * (importance w.r.t. doc_id)
		// importance = postings[term][id]*inverse_document_frequency(term)
		similarity = similarity + (log2(DOC_COUNT / UNI_DF[(*it)])) * (UNI_POSTINGS[(*it)][doc_id]) * (log2(DOC_COUNT / UNI_DF[(*it)]));
	}
	return similarity;
}

double compute_bi_similarity(vector< pair<int, int> > bi_query, int doc_id) {
	double similarity = 0.0;
	for(vector< pair<int, int> >::iterator it = bi_query.begin(); it != bi_query.end(); ++it) {
		similarity = similarity + (log2(DOC_COUNT / (double)BI_DF[(*it)])) * ((double)BI_POSTINGS[(*it)][doc_id]) * (log2(DOC_COUNT / (double)BI_DF[(*it)]));
	}
	return similarity;
}

bool compare_score_pair(const pair<string, double>&i, const pair<string, double>&j) {
    return i.second > j.second;
}






