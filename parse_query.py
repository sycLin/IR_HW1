#!/usr/bin/python
# -*- coding: utf8 -*-
import sys
import os
import io # for reading utf8

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

def print_usage():
	print >> sys.stderr, "Usage:"
	print >> sys.stderr, "\t%s %s %s" % (sys.argv[0], "<infile>", "<outfile>")

def main():
	# parse the query file
	try:
		root = parseXML(sys.argv[1])
	except:
		print >> sys.stderr, "error: cannot parse file as XML at: %s" % (sys.argv[1])
		sys.exit(-1)
	for e in root.findall("./*"):
		if e.tag == "topic":
			print "found a topic tag:"
			for ee in e.findall("./*"):
				if ee.tag == "number":
					# no use
					pass
				elif ee.tag == "title":
					# usually without chinese punctuation, just in case
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					print "[%s]: %s" % (ee.tag, ee.text)
				elif ee.tag == "question":
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					print "[%s]: %s" % (ee.tag, ee.text)
					uni = getUnigramFromString(ee.text)
					bi = getBigramFromString(ee.text)
					print "\t\tuni"
					for u in uni:
						print "\t\t\t%s" % (u)
					print "\t\tbi"
					for b in bi:
						print "\t\t\t%s" % (b)
				elif ee.tag == "narrative":
					ee.text = removeSpecialCharacters(removeChinesePunctuations(ee.text))
					print "[%s]: %s" % (ee.tag, ee.text)
				elif ee.tag == "concepts":
					concepts = removeSpecialCharacters(ee.text).split(u"、")
					print "[%s]:" % (ee.tag)
					for con in concepts:
						con = removeSpecialCharacters(removeChinesePunctuations(con))
						print con
		break

if __name__ == "__main__":

	# check arguments
	if len(sys.argv) != 3:
		print_usage()
		sys.exit(-1)
	if not os.path.exists(sys.argv[1]):
		print >> sys.stderr, "error: file not exists at: %s" % (sys.argv[1])
		print_usage()
		sys.exit(-1)
	if os.path.exists(sys.argv[2]):
		print >> sys.stderr, "error: file already exists at: %s" % (sys.argv[2])
		print_usage()
		sys.exit(-1)

	# call main()
	main()