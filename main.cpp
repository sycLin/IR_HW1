#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // for sqrt()

using namespace std;

//////////////////////
// global variables //
//////////////////////

/* for global settings */
string MODEL_DIR = "";
string OUTPUT_FILE_NAME = "";
string QUERY_FILE_NAME = "";
string NTCIR_DIR = "";

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


//////////////////////
// helper functions //
//////////////////////

void print_usage();
void build_vocabulary();
void build_document_list();
void build_postings();

///////////////////////////
// program entry: main() //
///////////////////////////

int main(int argc, char* argv[]) {
	// check argument
	// argv[1]: output file name
	// argv[2]: query file name
	// argv[3]: model dir
	// argv[4]: NTCIR dir
	if(argc != 5) {
		print_usage();
		exit(-1);
	}

	
	// set up from arguments

	cerr << "before set up: " << endl;
	cerr << "\tout = " << OUTPUT_FILE_NAME << endl;
	cerr << "\tq = " << QUERY_FILE_NAME << endl;
	cerr << "\tmodel = " << MODEL_DIR << endl;
	cerr << "\tNTCIR = " << NTCIR_DIR << endl;


	OUTPUT_FILE_NAME = string(argv[1]);
	QUERY_FILE_NAME = string(argv[2]);
	MODEL_DIR = string(argv[3]);
	NTCIR_DIR = string(argv[4]);

	cerr << "after set up: " << endl;
	cerr << "\tout = " << OUTPUT_FILE_NAME << endl;
	cerr << "\tq = " << QUERY_FILE_NAME << endl;
	cerr << "\tmodel = " << MODEL_DIR << endl;
	cerr << "\tNTCIR = " << NTCIR_DIR << endl;

	// build vocabulary (here, for unigram only)
	
	build_vocabulary();

	// build document list

	build_document_list();

	// build postings lists and DF

	build_postings();
	
	return 0;
}

void print_usage() {
	cerr << "Usage:" << endl;
	cerr << "\t./a.out <output_path> <query_path> <model_dir_path> <NTCIR_dir_path>" << endl;
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
}

void build_postings() {
	// construct the path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "inverted-file") : (MODEL_DIR + "/inverted-file");
	// open file and read it
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "cannot open file at: " << path << endl;
		exit(-1);
	}
	while(!feof(fp)) {
		
		// read the title line
		int t1, t2, N;
		if(fscanf(fp, "%d%d%d", &t1, &t2, &N) == EOF)
			break;
		
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
			// put into the postings lists: UNI_POSTINGS and BI_POSTINGS
			// map<int, map<int, int> > UNI_POSTINGS;
			// map<pair<int, int>, map<int, int> > BI_POSTINGS;
			if(t2 == -1) {
				// it's unigram
				UNI_POSTINGS[t1][doc_id] = count;
			} else {
				// it's bigram
				BI_POSTINGS[make_pair(t1, t2)][doc_id] = count;
			}
		}
	}
}


