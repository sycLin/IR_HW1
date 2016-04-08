#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // for sqrt(), log2()
#include <algorithm>
#define OKAPI_PARAM_B 0.7

using namespace std;

// global settings
string OUTPUT_FILE_NAME = "";
string QUERY_FILE_NAME = "";
string MODEL_DIR = "";
bool ROCCHIO_FLAG = false;

// global data
// document related
int DOC_COUNT = 0;
map<int, string> DOC_LIST;
map<int, int> DOC_LEN;
int AVG_DOC_LEN = 0;
// postings lists
map<int, map<int, int> > UNI_POSTINGS;
map<pair<int, int>, map<int, int> > BI_POSTINGS;
// document frequencies
map<int, int> UNI_DF;
map<pair<int, int>, int> BI_DF;
// query related
int QUERY_COUNT;
map<int, map<int, int> > Q_UNI_POSTINGS;
map<pair<int, int>, map<int, int> > Q_BI_POSTINGS;
map<int, int> Q_UNI_DF;
map<pair<int, int>, int> Q_BI_DF;

// helper functions
void print_usage();
void doc_build(); // fill in: DOC_COUNT and DOC_LIST
void inverted_file_build(); // fill in: UNI_POSTINGS, BI_POSTINGS, UNI_DF, BI_DF
void query_build(); // fill in: QUERY_COUNT, Q_UNI_POSTINGS, Q_BI_POSTINGS, Q_UNI_DF, Q_BI_DF
double compute_similarity(int q_id ,int doc_id);
double compute_similarity_rocchio(int q_id ,int doc_id, map<int, int> uni_map, map<pair<int, int>, int> bi_map);
bool compare_score_pair(const pair<int, double>&i, const pair<int, double>&j);

int main(int argc, char* argv[]) {
	// ---------- check arguments ---------- //
	// argv[1]: output file name
	// argv[2]: query file name
	// argv[3]: model directory
	if(argc != 4) {
		print_usage();
		exit(-1);
	}
	OUTPUT_FILE_NAME = string(argv[1]);
	QUERY_FILE_NAME = string(argv[2]);
	MODEL_DIR = string(argv[3]);

	// ---------- building up documents ---------- //
	cerr << "building up documents ..." << endl;
	doc_build();
	inverted_file_build();

	// ---------- building up queries ---------- //
	cerr << "building up queries ..." << endl;
	query_build();

	// ---------- perform searching ---------- //
	cerr << "performing searching ..." << endl;
	FILE* outfile = fopen(OUTPUT_FILE_NAME.c_str(), "w");
	for(int q_id = 0; q_id < QUERY_COUNT; q_id++) {
		cerr << "query" << q_id + 1 << ":" << endl;
		vector< pair<int, double> > scores; // pair(doc_id, similarity)
		for(int doc_id = 0; doc_id < DOC_COUNT; doc_id++) {
			scores.push_back(make_pair(doc_id, compute_similarity(q_id, doc_id)));
			if(doc_id % 1000 == 999)
				cerr << "\rprocessed " << doc_id + 1 << " doc.";
		}
		cerr << "\rprocessed " << DOC_COUNT << " doc." << endl;
		// sort by descending order
		sort(scores.begin(), scores.end(), compare_score_pair);

		// do Rocchio Pseudo Feedback
		ROCCHIO_FLAG = true; // debug use
		if(ROCCHIO_FLAG) {
			// performing Rocchio Pseudo Feedback
			cerr << "performing Rocchio..." << endl;
			// constructing more important terms, and store into 2 maps
			cerr << "constructing more important terms..." << endl;
			map<int, int> uni_rocchio_map; // store unigrams that should be out-weighting others
			map<pair<int, int>, int> bi_rocchio_map; // store bigrams that should out-weights
			// top 5 doc_id:
			// scores[0].first
			// scores[1].first
			// scores[2].first
			// scores[3].first
			// scores[4].first
			for(map<int, map<int, int> >::iterator it = UNI_POSTINGS.begin(); it != UNI_POSTINGS.end(); ++it) {
				int term_id = it->first;
				if(UNI_POSTINGS[term_id][scores[0].first] > 0
					|| UNI_POSTINGS[term_id][scores[1].first] > 0
					|| UNI_POSTINGS[term_id][scores[2].first] > 0
					|| UNI_POSTINGS[term_id][scores[3].first] > 0
					|| UNI_POSTINGS[term_id][scores[4].first] > 0)
					uni_rocchio_map[term_id] += 1; // mark as important
			}
			for(map<pair<int, int>, map<int, int> >::iterator it = BI_POSTINGS.begin(); it != BI_POSTINGS.end(); ++it) {
				pair<int, int> term = it->first;
				if(BI_POSTINGS[term][scores[0].first] > 0
					|| BI_POSTINGS[term][scores[1].first] > 0
					|| BI_POSTINGS[term][scores[2].first] > 0
					|| BI_POSTINGS[term][scores[3].first] > 0
					|| BI_POSTINGS[term][scores[4].first] > 0)
					bi_rocchio_map[term] += 1; // mark as important
			}
			// calculate the score all over again, with 2 rocchio maps
			scores.clear();
			for(int doc_id = 0; doc_id < DOC_COUNT; doc_id++) {
				scores.push_back(make_pair(doc_id, compute_similarity_rocchio(q_id, doc_id, uni_rocchio_map, bi_rocchio_map)));
				if(doc_id % 1000 == 999)
					cerr << "\rprocessed " << doc_id + 1 << " doc.";
			}
			cerr << "\rprocessed " << DOC_COUNT << " doc." << endl;
			// sort by descending order
			sort(scores.begin(), scores.end(), compare_score_pair);
		}

		// output the rank

		for(int i=0; i<100; i++) {
			fprintf(outfile, "%s\n", DOC_LIST[scores[i].first].c_str());
		}

		fprintf(outfile, "********\n");
	}

	fclose(outfile);

	return 0;
}

void print_usage() {
	cerr << "Usage:" << endl;
	cerr << "\t./a.out <Output> <Query> <ModelDir>" << endl;
}

void doc_build() {
	// construct path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "file-list") : (MODEL_DIR + "/file-list");
	// open file for reading
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "doc_build(): cannot open file at: " << path << endl;
		return;
	}
	int doc_counter = 0;
	while(!feof(fp)) {
		char* tmp = new char[100];
		if(fscanf(fp, "%s", tmp) == EOF)
			break;
		DOC_LIST[doc_counter] = string(tmp);
		doc_counter += 1;
	}
	DOC_COUNT = doc_counter;
}

void inverted_file_build() {
	// construct path
	string path = (MODEL_DIR[MODEL_DIR.length() - 1] == '/') ? (MODEL_DIR + "inverted-file") : (MODEL_DIR + "/inverted-file");
	// open file for reading
	FILE* fp = fopen(path.c_str(), "r");
	if(!fp) {
		cerr << "inverted_file_build(): cannot open file at: " << path << endl;
		return;
	}
	while(!feof(fp)) {
		// read title line
		int t1, t2, N;
		if(fscanf(fp, "%d%d%d", &t1, &t2, &N) == EOF)
			break;
		if(t2 == -1) {
			// unigram
			// read content lines
			for(int i=0; i<N; i++) {
				int doc_id, count;
				fscanf(fp, "%d%d", &doc_id, &count);
				UNI_POSTINGS[t1][doc_id] = count;
				// increment doc len
				DOC_LEN[doc_id] = DOC_LEN[doc_id] + count;
			}
			UNI_DF[t1] = N;
		} else {
			// bigram
			// read content lines
			for(int i=0; i<N; i++) {
				int doc_id, count;
				fscanf(fp, "%d%d", &doc_id, &count);
				BI_POSTINGS[make_pair(t1, t2)][doc_id] = count;
			}
			BI_DF[make_pair(t1, t2)] = N;
		}
	}
	// fill in AVG_DOC_LEN
	AVG_DOC_LEN = 0;
	for(map<int, int>::iterator it = DOC_LEN.begin(); it != DOC_LEN.end(); ++it)
		AVG_DOC_LEN += it->second;
	AVG_DOC_LEN /= DOC_COUNT;
}

void query_build() {
	// open file for reading
	FILE* fp = fopen(QUERY_FILE_NAME.c_str(), "r");
	if(!fp) {
		cerr << "query_build(): cannot open file at: " << QUERY_FILE_NAME << endl;
		return;
	}
	while(!feof(fp)) {
		// read the number of queries
		if(fscanf(fp, "%d", &QUERY_COUNT) == EOF)
			break;
		// read each query
		for(int q_id = 0; q_id < QUERY_COUNT; q_id++) {
			int uni_count, bi_count;
			if(fscanf(fp, "%d%d", &uni_count, &bi_count) == EOF)
				break;
			// reading unigrams of this query
			for(int i=0; i<uni_count; i++) {
				int t_id;
				fscanf(fp, "%d", &t_id);
				Q_UNI_POSTINGS[t_id][q_id] = Q_UNI_POSTINGS[t_id][q_id] + 1;
			}
			// reading bigrams of this query
			for(int i=0; i<bi_count; i++) {
				int t_id1, t_id2;
				fscanf(fp, "%d%d", &t_id1, &t_id2);
				Q_BI_POSTINGS[make_pair(t_id1, t_id2)][q_id] = Q_BI_POSTINGS[make_pair(t_id1, t_id2)][q_id] + 1;
			}
		}
		// fill in Q_UNI_DF and Q_BI_DF (by iterating postings lists)
		for(map<int, map<int, int> >::iterator it = Q_UNI_POSTINGS.begin(); it != Q_UNI_POSTINGS.end(); ++it) {
			int df = 0;
			for(int q_id = 0; q_id < QUERY_COUNT; q_id++) {
				if(Q_UNI_POSTINGS[it->first][q_id] > 0)
					df += 1;
			}
			Q_UNI_DF[it->first] = df;
		}
		for(map<pair<int, int>, map<int, int> >:: iterator it = Q_BI_POSTINGS.begin(); it != Q_BI_POSTINGS.end(); ++it) {
			int df = 0;
			for(int q_id = 0; q_id < QUERY_COUNT; q_id++) {
				if(Q_BI_POSTINGS[it->first][q_id] > 0)
					df += 1;
			}
			Q_BI_DF[it->first] = df;
		}
	}
}

double compute_similarity(int q_id ,int doc_id) {
	// scores
	double total_score = 0.0; // a combination of uni_score and bi_score
	double uni_score = 0.0; // score calculated from unigrams
	double bi_score = 0.0; // score calculated from bigrams

	// OKAPI normalization denominator
	double okapi_norm_denominator = (1.0 - OKAPI_PARAM_B + OKAPI_PARAM_B * (double)DOC_LEN[doc_id] / (double)AVG_DOC_LEN);

	// loop through Q_UNI_POSTINGS
	// map<int, map<int, int> > Q_UNI_POSTINGS;
	// for(map<int, map<int, int> >::iterator it = Q_UNI_POSTINGS.begin(); it != Q_UNI_POSTINGS.end(); ++it) {
	// 	if(Q_UNI_POSTINGS[it->first][q_id] == 0 || UNI_POSTINGS[it->first][doc_id] == 0 || Q_UNI_DF[it->first] == 0 || UNI_DF[it->first] == 0)
	// 		continue;
	// 	// w1 = (TF in q) * (IDF in q), with OKAPI normalization on TF
	// 	double w1 = ((double)(Q_UNI_POSTINGS[it->first][q_id]) / okapi_norm_denominator) * log2((double)QUERY_COUNT / (double)Q_UNI_DF[it->first]);
	// 	// w2 = (TF in d) * (IDF in d), with OKAPI normalization on TF
	// 	double w2 = ((double)(UNI_POSTINGS[it->first][doc_id]) / okapi_norm_denominator) * log2((double)DOC_COUNT / (double)UNI_DF[it->first]);
	// 	uni_score += (w1 * w2);
	// }

	// loop through Q_BI_POSTINGS
	// map<pair<int, int>, map<int, int> > Q_BI_POSTINGS;
	for(map<pair<int, int>, map<int, int> >:: iterator it = Q_BI_POSTINGS.begin(); it != Q_BI_POSTINGS.end(); ++it) {
		if(Q_BI_POSTINGS[it->first][q_id] == 0 || BI_POSTINGS[it->first][doc_id] == 0 || Q_BI_DF[it->first] == 0 || BI_DF[it->first] == 0)
			continue;
		// w1 = (TF in q) * (IDF in q), with OKAPI normalization on TF
		double w1 = ((double)Q_BI_POSTINGS[it->first][q_id] / okapi_norm_denominator) * log2((double)QUERY_COUNT / (double)Q_BI_DF[it->first]);
		// w2 = (TF in d) * (IDF in d), with OKAPI normalization on TF
		double w2 = ((double)(BI_POSTINGS[it->first][doc_id]) / okapi_norm_denominator) * log2((double)DOC_COUNT / (double)BI_DF[it->first]);
		bi_score += (w1 * w2);
	}

	total_score = (0.0) * uni_score + (1.0 - 0.0) * bi_score;
	return total_score;
}

double compute_similarity_rocchio(int q_id ,int doc_id, map<int, int> uni_map, map<pair<int, int>, int> bi_map) {
	// scores
	double total_score = 0.0; // a combination of uni_score and bi_score
	double uni_score = 0.0; // score calculated from unigrams
	double bi_score = 0.0; // score calculated from bigrams

	// OKAPI normalization denominator
	double okapi_norm_denominator = (1.0 - OKAPI_PARAM_B + OKAPI_PARAM_B * (double)DOC_LEN[doc_id] / (double)AVG_DOC_LEN);

	// loop through Q_UNI_POSTINGS
	// map<int, map<int, int> > Q_UNI_POSTINGS;
	// for(map<int, map<int, int> >::iterator it = Q_UNI_POSTINGS.begin(); it != Q_UNI_POSTINGS.end(); ++it) {
	// 	if(Q_UNI_POSTINGS[it->first][q_id] == 0 || UNI_POSTINGS[it->first][doc_id] == 0 || Q_UNI_DF[it->first] == 0 || UNI_DF[it->first] == 0)
	// 		continue;
	// 	// w1 = (TF in q) * (IDF in q), with OKAPI normalization on TF
	// 	double w1 = ((double)(Q_UNI_POSTINGS[it->first][q_id]) / okapi_norm_denominator) * log2((double)QUERY_COUNT / (double)Q_UNI_DF[it->first]);
	// 	// w2 = (TF in d) * (IDF in d), with OKAPI normalization on TF
	// 	double w2 = ((double)(UNI_POSTINGS[it->first][doc_id]) / okapi_norm_denominator) * log2((double)DOC_COUNT / (double)UNI_DF[it->first]);
	// 	if(uni_map[it->first] > 0)
	// 		uni_score += 2.0 * (w1 * w2);
	// 	else
	// 		uni_score += (w1 * w2);
	// }

	// loop through Q_BI_POSTINGS
	// map<pair<int, int>, map<int, int> > Q_BI_POSTINGS;
	for(map<pair<int, int>, map<int, int> >:: iterator it = Q_BI_POSTINGS.begin(); it != Q_BI_POSTINGS.end(); ++it) {
		if(Q_BI_POSTINGS[it->first][q_id] == 0 || BI_POSTINGS[it->first][doc_id] == 0 || Q_BI_DF[it->first] == 0 || BI_DF[it->first] == 0)
			continue;
		// w1 = (TF in q) * (IDF in q), with OKAPI normalization on TF
		double w1 = ((double)Q_BI_POSTINGS[it->first][q_id] / okapi_norm_denominator) * log2((double)QUERY_COUNT / (double)Q_BI_DF[it->first]);
		// w2 = (TF in d) * (IDF in d), with OKAPI normalization on TF
		double w2 = ((double)(BI_POSTINGS[it->first][doc_id]) / okapi_norm_denominator) * log2((double)DOC_COUNT / (double)BI_DF[it->first]);
		if(bi_map[it->first] > 0)
			bi_score += 2.0 * (w1 * w2);
		else
			bi_score += (w1 * w2);
	}

	total_score = (0.0) * uni_score + (1.0 - 0.0) * bi_score;
	return total_score;
}

bool compare_score_pair(const pair<int, double>&i, const pair<int, double>&j) {
	// descending order
	return i.second > j.second;
}


