#!/bin/bash

GREEN=$'\e[0;32m'
RED=$'\e[0;31m'
NC=$'\e[0m'


rm -f autograder_out.txt;
make clean;
make test1;
./autograder >> autograder_out.txt;
if [ $? -ne 0 ]
then
    echo "${RED}Test Crashed${NC}";
    rm -f autograder_out.txt;
    make clean;
    exit
fi
make clean;
make test2;
./autograder >> autograder_out.txt;
if [ $? -ne 0 ]
then
    echo "${RED}Test Crashed${NC}";
    make clean;
    rm -f autograder_out.txt;
    exit
fi
make clean;

echo "${GREEN}Test Completed, output: autograder_out.txt${NC}";