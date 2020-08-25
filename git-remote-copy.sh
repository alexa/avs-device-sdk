#!/bin/sh

if [ $# -ne 4 ] ; then
  echo 'usage: git-remote-copy.sh LOCAL_PATH FROM_COMMIT HOST REMOTE_PATH'
  exit 2
fi

local="$1"
commit=$2
host=$3
remote="$4"

git -C "$local" diff --name-only $commit | grep -v -e git-remote-copy.sh -e .gitignore | \
  tar cf /tmp/remote_copy.tar -C "$local" -T -
  
scp /tmp/remote_copy.tar $host:/tmp >/dev/null
ssh $host tar xvf /tmp/remote_copy.tar -C "$remote"
