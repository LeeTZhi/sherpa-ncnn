#!/bin/bash

# $1 src folder, $2 dest folder $3 private key, $4 public key

#get current dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SIGNER=$DIR/../build/bin/sign_sn_tool

if [ ! -f $SIGNER ]; then
    echo "signer tool not found"
    exit 1
fi

#input params number 
if [ $# -ne 4 ]; then
    echo "Usage: $0 src_folder dest_folder private_key public_key"
    exit 1
fi

SRC_FOLDER=$1
DEST_FOLDER=$2
PRIVATE_KEY=$3
PUBLIC_KEY=$4

#if dest folder not exist, create it
if [ ! -d $DEST_FOLDER ]; then
    mkdir -p $DEST_FOLDER
fi

total_files=$(ls $SRC_FOLDER/*.txt | wc -l)
# for loop to sign all files in src folder
for file in $SRC_FOLDER/*.txt
do
    filename=$(basename $file)
    echo "Sign $filename"
    $SIGNER $file  $PRIVATE_KEY  $PUBLIC_KEY $DEST_FOLDER/$filename 
done

echo "Sign $total_files files done"