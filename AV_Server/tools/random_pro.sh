#!/bin/bash
# 该脚本产生 appid 和 appsecret 值
# appid为当前时间的毫秒值,字符长度:13
# appsecret 值字符长度为:20
#
# appid 取值为 cur_time_ms 字段值
# appsecret取值为 20_types_tm_and_random_md5_val 字段值 
#

random_len=20
random=$(cat /dev/urandom |tr -cd [:alnum:] |fold -w${random_len} |head -n 1)
echo "${random_len}_bytes_random: "$random
timeStamp=$(date +%s)
currentTimeStamp=$((timeStamp*1000+`date "+%N"`/1000000))
echo "cur_time_ms: "$currentTimeStamp
tm_random="$currentTimeStamp$random"
echo "tm_and_random: "$tm_random
md5len=20
tm_random_md5=$(echo "$tm_random" | md5sum | fold -w${md5len} |head -n 1)
echo "20_types_tm_and_random_md5_val: "$tm_random_md5
echo -e "------ allocate domain: ----------"
echo "---> home.xl.cn <-----"
echo "---> sns.xl.cn <-----"
