#!/bin/bash
if [ $# -lt 2 ]; then
  echo "Bad usage:"
  echo "Use $0 <user> <server name or ip>"
  exit 1
fi

echo "Generate and copy debug keys... $# for user=$1 server=$2"
if [ ! -f ./debug_key ] ; then
  echo "Generate SSH keys for debuggin...."
  ssh-keygen -t rsa -f debug_key
else
  echo "SSH Key exist!"
fi

echo "Copy to remote debug server..."
ssh-copy-id -i debug_key $1@$2
echo "done!"
exit 0
