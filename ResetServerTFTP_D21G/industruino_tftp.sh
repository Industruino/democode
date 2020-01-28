#!/bin/bash
# Industruino D21G TFTP script
# this script takes 2 arguments: 
# 1. the IP address of the Industruino
# 2. the file.bin with the Industruino sketch, processed with the prepareFile utility
# this script does:
# > reset the Industruino by URL
# > uploads the .bin file by TFTP
# password assumed is 'password' and reset port '8080'

if [ $# != 2 ] 
then 
	echo "ERROR: script needs 2 arguments: Industruino IP address and file.bin"
	exit 1
fi

echo "+++++++++++++++++++++++++++++++++++++"
echo "Doing RESET of Industruino over http:"
echo "+++++++++++++++++++++++++++++++++++++"
echo "http://$1:8080/password/reset"
curl "http://$1:8080/password/reset"

echo ""
echo "++++++++++++++++++++++++++"
echo "Doing TFTP to Industruino:"
echo "++++++++++++++++++++++++++"
tftp $1 << !
mode binary
trace
verbose
put $2
quit
!

echo "++++++++++++++++++++++++++"

exit 0



