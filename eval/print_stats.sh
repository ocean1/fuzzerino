#!/bin/bash

for dir in datasets/*; do
    echo
    echo "=== $dir"
    echo "    total files:" `ls $dir | wc -l`
    echo "    avg size:" `ls $dir -l | gawk '{sum += $5; n++;} END {print sum/(n-1);}'` "bytes"
    echo "    largest:" `find $dir -type f | xargs ls -lS 2>/dev/null | head -n 1 | awk '{print $5}'` "bytes"
    echo "    smallest:" `find $dir -type f | xargs ls -lrS 2>/dev/null | head -n 1 | awk '{print $5}'` "bytes"
done