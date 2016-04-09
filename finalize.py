#!/usr/bin/python
# -*- coding: utf8 -*-
import sys
import os

def main():
	topic_numbers = read_topic_numbers()
	ranks = read_rank_file()
	if len(topic_numbers) != len(ranks):
		print >> sys.stderr, "QAQ len (topics, ranks) = (%d, %d)" % (len(topic_numbers), len(ranks))
		sys.exit(-1)
	# write result to output file
	with open(sys.argv[3], "w") as f:
		for i in range(len(topic_numbers)):
			for fn in ranks[i]:
				# processed fn first
				true_fn = fn.split("/")[-1].lower()
				f.write("%s %s\n" % (topic_numbers[i], true_fn))
		f.close()


def read_topic_numbers():
	ret = []
	with open(sys.argv[1]) as f:
		for l in f:
			if l.strip() != "":
				ret.append(l.strip())
	return ret

def read_rank_file():
	ret = []
	tmp = []
	with open(sys.argv[2]) as f:
		for l in f:
			if l.strip() == "********":
				ret.append(tmp)
				tmp = []
			else:
				tmp.append(l.strip())
	return ret

def print_usage():
	print >> sys.stderr, "Usage:"
	print >> sys.stderr, "\t" + sys.argv[0] + " <TopicNumFile> <RankFile> <OutputFile>"

if __name__ == "__main__":
	# check arguments
	if len(sys.argv) != 4:
		print_usage()
		sys.exit(-1)
	main()
