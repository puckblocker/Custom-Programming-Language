#!/usr/bin/env bash
date
echo "Compiling intro_sjh529.cpp..."
g++ intro_sjh529.cpp -o intro_sjh529
echo "Running program first time..."
./intro_sjh529
echo "Running program second time, saving output..."
./intro_sjh529 > my.out
echo "Comparing with expected..."
diff -y expected.out my.out
echo "Current user is: $(whoami)"