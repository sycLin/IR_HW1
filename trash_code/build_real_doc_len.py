#!/usr/bin/python
# -*- coding: utf8 -*-
import sys
import os
import parse_query as PQ

####################
# global variables #
####################

MODEL_DIR = ""
NTCIR_DIR = ""


####################
# helper functions #
####################

def get_doc_len(path):
	try:
		root = PQ.parseXML(path)
	except:
		print >> sys.stderr, "cannot parse XML file at: %s" % (path)
		sys.exit(-1)
	doc_len, title_len, text_len = 0, 0, 0
	for e in root.findall("./*"):
		if e.tag == "doc":
			for ee in e.findall("./*"):
				if ee.tag == "title":
					if ee.text:
						title_len += len(PQ.removeSpecialCharacters(PQ.removeChinesePunctuations(ee.text)))
				if ee.tag == "text":
					for eee in ee.findall("./*"):
						if eee.tag == "p":
							text_len += len(PQ.removeSpecialCharacters(PQ.removeChinesePunctuations(eee.text)))
	doc_len = title_len + text_len
	return title_len, text_len, doc_len

def main():
	global MODEL_DIR, NTCIR_DIR
	# 1) read the file list
	files = []
	with open(os.path.join(MODEL_DIR, "file-list")) as f:
		for l in f:
			line = l.strip()
			files.append(line)
		f.close()
	# read all the file and calculate the length
	lengths = []
	for fn in files:
		# construct path
		if NTCIR_DIR.split("/")[-1] == "":
			if NTCIR_DIR.split("/")[-2] == fn.split("/")[0]:
				path = os.path.join("".join(NTCIR_DIR.split("/")[:-2]), fn)
			else:
				path = os.path.join(NTCIR_DIR, fn)
		elif NTCIR_DIR.split("/")[-1] == fn.split("/")[0]:
			path = os.path.join("".join(NTCIR_DIR.split("/")[:-1]), fn)
		else:
			path = os.path.join(NTCIR_DIR, fn)
		# parse the XML file
		title_len, text_len, doc_len = get_doc_len(path)
		print "(%d, %d) => %d" % (title_len, text_len, doc_len)

	
def print_usage():
	print >> sys.stderr, "Usage:"
	print >> sys.stderr, "\t" + sys.argv[0] + " <model_dir> <NTCIR_dir>"

if __name__ == "__main__":
	global MODEL_DIR, NTCIR_DIR
	# check arguments
	if len(sys.argv) != 3:
		print_usage()
		sys.exit(-1)
	if not os.path.isdir(sys.argv[1]):
		print >> sys.stderr, "not a directory: %s" % (sys.argv[1])
		print_usage()
		sys.exit(-1)
	if not os.path.isdir(sys.argv[2]):
		print >> sys.stderr, "not a directory: %s" % (sys.argv[2])
		print_usage()
		sys.exit(-1)
	MODEL_DIR = sys.argv[1]
	NTCIR_DIR = sys.argv[2]
	# call main
	main()





