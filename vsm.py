#!/usr/bin/python
import sys
import os
import math
from collections import defaultdict


###################################
# global variables for settings   #
# (will be specified by sys.argv) #
###################################

MODEL_DIR = ""
OUTPUT_FILE_NAME = ""
QUERY_FILE_NAME = ""
NTCIR_DIR = ""

########################################
# global variables for data            #
# (will be constructed by the program) #
########################################

DOC_COUNT = 0
DOC_LIST = []
VOCABULARY = []
UNI_POSTINGS_LISTS = defaultdict(dict) # dict of dict (unigram)
BI_POSTINGS_LISTS = defaultdict(dict) # dict of dict (bigram)
UNI_DOCUMENT_FREQUENCY = defaultdict(int) # dict of int
BI_DOCUMENT_FREQUENCY = defaultdict(int) # dict of int
DOC_LENGTHS = defaultdict(float) # dict of float

#########################
# helper functions area #
#########################

"""
the main function to call
"""
def main():
	set_up_from_argv()
	build_vocabulary()
	build_postings_and_document_frequency() # usually takes about 1.5 minutes
	pass

"""
to output a hint for usage of this program
"""
def print_usage():
	print >> sys.stderr, "Usage:"
	print >> sys.stderr, "\t" + __file__ + " <output_path> <query_path> <model_dir_path> <NTCIR_dir_path>"

"""
to get global settings from sys.argv
"""
def set_up_from_argv():
	global OUTPUT_FILE_NAME, QUERY_FILE_NAME, MODEL_DIR, NTCIR_DIR
	# argv[1]: output file name
	# argv[2]: query file name
	# argv[3]: model dir
	# argv[4]: NTCIR dir
	if len(sys.argv) < 5:
		print >> sys.stderr, "wrong argument count..."
		print_usage()
		sys.exit(-1)
	if not os.path.exists(sys.argv[1]):
		OUTPUT_FILE_NAME = sys.argv[1]
	else:
		print >> sys.stderr, "not a valid output file name: %s" % sys.argv[1]
		print_usage()
		sys.exit(-1)
	if os.path.exists(sys.argv[2]):
		QUERY_FILE_NAME = sys.argv[2]
	else:
		print >> sys.stderr, "not a valid query file name: %s" % sys.argv[2]
		print_usage()
		sys.exit(-1)
	if os.path.isdir(sys.argv[3]):
		MODEL_DIR = sys.argv[3]
	else:
		print >> sys.stderr, "not a valid model dir: %s" % sys.argv[3]
		print_usage()
		sys.exit(-1)
	if os.path.isdir(sys.argv[4]):
		NTCIR_DIR = sys.argv[4]
	else:
		print >> sys.stderr, "not a valid NTCIR dir: %s" % sys.argv[4]
		print_usage()
		sys.exit(-1)

"""
to build the vocabulary from the file vocab.all
"""
def build_vocabulary():
	global MODEL_DIR, VOCABULARY
	with open(os.path.join(MODEL_DIR, "vocab.all")) as f:
		# no need to skip the first line with: next(f)
		# because the id can match the list index directly
		for raw_line in f:
			term = raw_line.strip()
			VOCABULARY.append(term)

"""
to build the postings lists from the file inverted-file
"""
def build_postings_and_document_frequency():
	global MODEL_DIR, UNI_POSTINGS_LISTS, BI_POSTINGS_LISTS, VOCABULARY, UNI_DOCUMENT_FREQUENCY, BI_DOCUMENT_FREQUENCY
	with open(os.path.join(MODEL_DIR, "inverted-file")) as f:
		section_counter = 0 # to indicate how many more lines to read for this section
		current_term = "" # to store the current term
		gram_indicator = 0 # to indicaate unigram (1) / bigram (2)
		for l in f:
			if section_counter == 0:
				# it is the "vocab line"
				t1, t2, n = l.split()
				t1, t2, n = int(t1), int(t2), int(n)
				if t2 == -1:
					# it's a unigram
					gram_indicator = 1
					current_term = VOCABULARY[t1]
					UNI_DOCUMENT_FREQUENCY[current_term] = n
				else:
					# it's a bigram
					gram_indicator = 2
					current_term = VOCABULARY[t1] + " " + VOCABULARY[t2] # note: a space is put between the bigram
					BI_DOCUMENT_FREQUENCY[current_term] = n
				section_counter = n
			else:
				# it is the "document line"
				doc_id, count = l.split()
				doc_id, count = int(doc_id), int(count)
				# put into the postings list
				if gram_indicator == 1:
					UNI_POSTINGS_LISTS[current_term][doc_id] = count
				elif gram_indicator == 2:
					BI_POSTINGS_LISTS[current_term][doc_id] = count
				else:
					print >> sys.stderr, "error: unknown gram_indicator = %d" % (gram_indicator)
					sys.exit(-1)
				section_counter -= 1


"""
to compute the lengths for every document
TODO: only unigram lengths finished, bigram not yet!!
"""
def compute_document_lengths():
	# first lets count the document (and store in DOC_COUNT global)
	global DOC_LIST, DOC_COUNT, MODEL_DIR
	with open(os.path.join(MODEL_DIR, "file-list")) as f:
		for line in f:
			DOC_LIST.append(line.strip())
	DOC_COUNT = len(DOC_LIST)
	# next, compute the length for every doc
	global VOCABULARY, DOC_LENGTHS
	for doc_id in range(len(DOC_LIST)):
		length = 0
		for term_id in range(len(VOCABULARY)):
			if term_id == 0:
				# term_id 0 does not exist.
				# that was actually the encoding line.
				continue
			length += (compute_importance(VOCABULARY[term_id], doc_id, 1) ** 2)
		DOC_LENGTHS[doc_id] = math.sqrt(length)

"""
to calculate the importance of a term in a certain document
(we use TF-IDF as importance here)
term: the term to compute importance
doc_id: the id of the document to compute importance
gram_indicator: to specify unigram (1) / bigram (2)
"""
def compute_importance(term, doc_id, gram_indicator):
	global UNI_POSTINGS_LISTS, BI_POSTINGS_LISTS
	if gram_indicator == 1:
		# unigram
		if doc_id in UNI_POSTINGS_LISTS[term]:
			return UNI_POSTINGS_LISTS[term][doc_id] * get_idf(term, gram_indicator)
		else:
			return 0.0
	elif gram_indicator == 2:
		# bigram
		if doc_id in BI_POSTINGS_LISTS[term]:
			return BI_POSTINGS_LISTS[term][doc_id] * get_idf(term, gram_indicator)
		else:
			return 0.0
	else:
		print >> sys.stderr, "error: unknown gram_indicator in compute_importance(): %d" % (gram_indicator)
		sys.exit(-1)

"""
to compute the inverse-document-frequency from document frequency
term: the term to get IDF
gram_indicator: to specify unigram (1) / bigram (2)
"""
def get_idf(term, gram_indicator):
	global UNI_DOCUMENT_FREQUENCY, BI_DOCUMENT_FREQUENCY, DOC_COUNT
	if gram_indicator == 1:
		if term in UNI_DOCUMENT_FREQUENCY:
			return math.log(DOC_COUNT / UNI_DOCUMENT_FREQUENCY[term], 2) # base = 2 here, but it doesn't matter.
		else:
			return 0.0
	elif gram_indicator == 2:
		if term in BI_DOCUMENT_FREQUENCY:
			return math.log(DOC_COUNT / BI_DOCUMENT_FREQUENCY[term], 2) # base = 2 here, but it doesn't matter.
		else:
			return 0.0
	else:
		print >> sys.stderr, "error: unknown gram_indicator in get_idf(): %d" % (gram_indicator)
		sys.exit(-1)

"""
to compute the cosine similarity for two vectors
terms: the query terms that have been well extracted
doc_id: the id of the document to compute similarity
gram_indicator: to specify unigram (1) / bigram (2)
"""
def compute_cos_similarity(terms, doc_id, gram_indicator):
	if gram_indicator == 1:
		global VOCABULARY, DOC_LENGTHS
		similarity = 0.0
		for term in terms:
			if term in VOCABULARY:
				similarity += get_idf(term, gram_indicator) * compute_importance(term, doc_id, gram_indicator)
		return (similarity / DOC_LENGTHS[doc_id])
	elif gram_indicator == 2:
		print >> sys.stderr, "warning: compute_cos_similarity() for bigram: not yet implemented."
	else:
		print >> sys.stderr, "error: unknown gram_indicator in compute_cos_similarity(): %d" % (gram_indicator)
		sys.exit(-1)


if __name__ == "__main__":
	main()
