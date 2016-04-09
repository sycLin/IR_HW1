#!/usr/bin/python
# -*- coding: utf8 -*-
import sys
import os
import io # for reading utf8
from collections import defaultdict

###############
# global data #
###############

VOCAB = defaultdict(int) # a reversed version of vocabulary (character => term_id)

####################
# helper functions #
####################

# helps parsing an XML file
def parseXML(f_name):
	# import required module
	# no need to worry about importing the module twice
	# Python has done the optimization for us
	import xml.etree.ElementTree as ET
	tree = ET.parse(f_name)
	return tree.getroot()

def removeSpecialCharacters(s):
	special = "\n\r\t"
	for c in special:
		s = s.replace(c, "")
	return s

def removeChinesePunctuations(s):
	ch_punc = [u"，", u"、", u"。", u"！", u"：", u"？"]
	for c in ch_punc:
		s = s.replace(c, "")
	return s

def getUnigramFromString(s):
	unigram = []
	for ch in s:
		unigram.append(ch)
	return unigram

def getBigramFromString(s):
	bigram = []
	for i in range(len(s) - 1):
		bigram.append(s[i] + s[i+1])
	return bigram

def getTermIdSequence(s):
	ret = []
	for ch in s:
		ret.append(getTermId(ch))
	return ret

# term_id = 0 means not-found (not-in-vocabulary)
def getTermId(term):
	global VOCAB
	return VOCAB[term]

def getUnigramFromTermIdSequence(term_list):
	ret = []
	for term_id in term_list:
		if term_id > 0:
			ret.append(term_id)
	return ret

def getBigramFromTermIdSequence(term_list):
	ret = []
	for i in range(len(term_list) - 1):
		if term_list[i] > 0 and term_list[i + 1] > 0:
			ret.append((term_list[i], term_list[i+1]))
	return ret

def build_vocabulary():
	global VOCAB
	counter = 0
	with open(os.path.join(sys.argv[1], "vocab.all")) as f:
		for l in f:
			term = l.strip()
			if type(term) != type(u"whatever"):
				term = unicode(term, "utf-8")
			VOCAB[term] = counter
			counter += 1

def print_usage():
	print >> sys.stderr, "Usage:"
	print >> sys.stderr, "\t%s %s %s %s %s" % (sys.argv[0], "<model_dir>", "<infile>", "<outfile>", "<outfile2>")

def main():
	# build the vocabulary (reversed version, chinese character => term_id)
	global VOCAB
	build_vocabulary()

	# parse the query file
	try:
		root = parseXML(sys.argv[2])
	except:
		print >> sys.stderr, "error: cannot parse file as XML at: %s" % (sys.argv[2])
		sys.exit(-1)

	# store all grams.
	# the structure looks like:
	#    grams = [topic1, topic2, ..., topicN]
	#    topicK = [uni, bi]
	#    uni = [ug1, ug2, ug3, ...] with each ug an integer (i.e., term_id)
	#    bi = [bg1, bg2, bg3, ...] with each bg a tuple of 2 term_id (e.g., bg1 = (bgp1, bgp2))
	grams = []
	topic_numbers = []
	for e in root.findall("./*"):
		if e.tag == "topic":
			uni = [] # to store all unigram of this topic
			bi = [] # to store all bigram of this topic
			for ee in e.findall("./*"):
				if ee.tag == "number":
					# store the numbers
					topic_numbers.append(ee.text.strip()[-3:])
				elif ee.tag == "title":
					# usually without chinese punctuation, just in case
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					seq = getTermIdSequence(ee.text)
					uni = uni + getUnigramFromTermIdSequence(seq)
					bi = bi + getBigramFromTermIdSequence(seq)
				elif ee.tag == "question":
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					seq = getTermIdSequence(ee.text)
					uni = uni + getUnigramFromTermIdSequence(seq)
					bi = bi + getBigramFromTermIdSequence(seq)
				elif ee.tag == "narrative":
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					seq = getTermIdSequence(ee.text)
					uni = uni + getUnigramFromTermIdSequence(seq)
					bi = bi + getBigramFromTermIdSequence(seq)
				elif ee.tag == "concepts":
					concepts = removeSpecialCharacters(ee.text).split(u"、")
					for con in concepts:
						con = removeSpecialCharacters(removeChinesePunctuations(con))
						seq = getTermIdSequence(con)
						uni = uni + getUnigramFromTermIdSequence(seq)
						bi = bi + getBigramFromTermIdSequence(seq)
			# let uni and bi be unique
			# uni = list(set(uni))
			# bi = list(set(bi))
			grams.append([uni, bi])

	# write all information to file
	with open(sys.argv[3], "w") as f:
		# first line: the number of topics
		f.write("%d\n" % (len(grams)))
		for uni, bi in grams:
			f.write("%d %d\n" % (len(uni), len(bi)))
			for unigram in uni:
				f.write("%d\n" % (unigram))
			for bigram in bi:
				f.write("%d %d\n" % (bigram[0], bigram[1]))
		f.close()
	# write topic numbers to outfile2
	with open(sys.argv[4], "w") as f:
		for tn in topic_numbers:
			f.write("%s\n" % (tn))
		f.close()

if __name__ == "__main__":

	# check arguments
	if len(sys.argv) != 5:
		print_usage()
		sys.exit(-1)
	if not os.path.isdir(sys.argv[1]):
		print >> sys.stderr, "error: directory not exists at: %s" % (sys.argv[1])
		print_usage()
		sys.exit(-1)
	if not os.path.exists(sys.argv[2]):
		print >> sys.stderr, "error: file not exists at: %s" % (sys.argv[2])
		print_usage()
		sys.exit(-1)
	if os.path.exists(sys.argv[3]):
		print >> sys.stderr, "error: file already exists at: %s" % (sys.argv[3])
		print_usage()
		sys.exit(-1)
	if os.path.exists(sys.argv[4]):
		print >> sys.stderr, "error: file already exists at: %s" % (sys.argv[4])
		print_usage()
		sys.exit(-1)

	# call main()
	main()