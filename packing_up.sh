#!/bin/bash

if [ -d R04922170 ];then
	echo "removing directory: R04922170/ ..."
	rm -rf R04922170
fi

if [ -f R04922170.zip ];then
	echo "removing zip file: R04922170.zip ..."
	rm -rf R04922170.zip
fi

echo "creating directory R04922170/ ..."
mkdir R04922170
echo "copying necessary files into R04922170/ ..."
cp Makefile REPORT.pdf compile.sh execute.sh finalize.py parse_query.py vsm.cpp wrap.cpp R04922170
echo "zipping up the directory ..."
zip -r R04922170.zip R04922170/
echo "Done"
