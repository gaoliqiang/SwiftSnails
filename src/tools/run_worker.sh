#!/bin/bash
set -x

cat > ./data.txt


# SwiftSnails librarys
export LD_LIBRARY_PATH="/home/chunwei/local/lib64:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="/home/chunwei/SwiftSnails/third/local/gtest/lib/.libs/:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="/home/chunwei/SwiftSnails/third/local/zeromq/lib:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="/home/chunwei/SwiftSnails/third/local/glog/lib:$LD_LIBRARY_PATH"

./swift_worker -config worker.conf -data ./data.txt
