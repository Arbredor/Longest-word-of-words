
Code Challenge Submission README
Summary:  Find the longest word made entirely of other full words in a large list

  The problem statement follows at the end of this README file.  The findLongest.cpp file contains all the code necessary to solve the problem and all notes describing the algorithm.  Compiling the C++ file requires C++11 extensions, due to the use of the ‘auto’ type specifier to simplify a complicated iterator declaration.

  Note:  The solution is fast, but additional caching of intermediate results would improve the algorithm.  (The idea for the enhancement came after submitting the solution for evaluation and the extra code for it has not been added.)

———————————————————

Problem statement:
Kindly use C, C++ or C# to code this.

Write a program that reads a file containing a sorted list of words (one word per line, no spaces, all lower case), then identifies the 
	•	1st longest word in the file that can be constructed by concatenating copies of shorter words also found in the file. 
	•	The program should then go on to report the 2nd longest word found 
	•	Total count of how many of the words in the list can be constructed of other words in the list.

Please reply to this email with your solution as source code along with 
the 1st and the 2nd longest words that the program finds and the count of words that can be constructed as an output in the body of the email and any comments you have on the approach you took.

For example, if the file contained:

       cat
       cats
       catsdogcats
       catxdogcatsrat
       dog
       dogcatsdog
       hippopotamuses
       rat
       ratcatdogcat

The answer would be 'ratcatdogcat' - at 12 letters, it is the longest word made up of other words in the list.  The program should then go on to report how many of the words in the list can be constructed of other words in the list.

Please send your solution in source code form, written in C or C++.   This is not just a puzzle or classroom assignment; it is your opportunity to demonstrate your engineering judgment in a way that you cannot do in a personal interview.  Performance matters: the program should return results quickly even for very large lists (100,000+ items).  

Please find attached a file, “words for problem.txt”, containing a word list, with 173k rows, for testing purposes.



