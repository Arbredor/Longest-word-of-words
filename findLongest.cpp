
// Code submission
// Date:  11/11/2013

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iterator>

// Compilation caveats:
//   Requires C++11 extensions, for use of 'auto' type specifier to simplify iterator declaration.

// Notes on the problem:
// (1) The input file may have extra control characters, whitespace, empty lines, and mixed case.
// (2) Assume that mixed case words that would match if they were all lower case should match.
// (3) If there are multiple words made of words of the longest size, then the program will
//     return the first two words to match of that size.
// (4) The given input file is already sorted, except for extra blank lines.  If the input file
//     were not already sorted, then we could sort it first, adding an average O(n log n) operation.
//     (Implemented as a command line option.)
//
// Steps to find longest words made of other words:
// (1) Read words in separate lines, clean the words, and add the words to a (hashed) set.
//     Analysis:  The operation is amortized O(n).
//     (1b) Optional, if we want to prioritize longest words first:
//          Add pointers to strings in vectors corresponding to string length; hash vectors by string length.
//          NOTE:  We need to have a sorted list of keys anyway since a fast unordered set will
//                 return its hashed keys in an unknown order.  If we have multiple matches of
//                 the longest lengths, the first/second word results could be indeterminate.
//                 We might as well have a small hash to vectors of string pointers, keyed by
//                 string lengths.
//
// (2) If we care about finding a few longest words, start with the longest words.
//     If each vector of strings of a given length is sorted, sorting the vector hash keys
//     by decreasing length means we only have to store the first two matches and do
//     not have to compare sizes for each match.  (Good branch prediction should take
//     over once the first two matches are stored when using longest words first.)
//     Procedure:
//       Start with first character, add characters until we match a word in the set
//        Try a match with the remaining characters, if none, add characters until we match a word in the set
//        Try the test with the remaining characters, ... and so on ...
//       - Call recursively to take advantage of the stack keeping
//         the state of the previously located word boundaries.
//       Note:  Do not match full word.
//       Save 1st and 2nd words found; count all.
//     Analysis:
//       While each word can simply look for matches in the unordered set, each word could
//       have multiple hash lookups, depending on how many substrings it needs to try.
//       Longer words could potentially have more substrings to test before finding a match.
//       If m is the average number of substring lookups per word over n words, then the
//       procedure is approximately O(mn).

// Greedy alternative, starting with large words and shrinking (ultimately implemented below):
// (2)  Start with largest substring less than the whole word, remove characters until we match
//       Try a match with the remaining characters, if none, remove characters until we match
//       Try a match, ... and so on ...

// Time optimization ideas that would require much more space (not implemented):
//   (1) Store the results of completely failed partial words in another set,
//       to avoid unnecessarily checking partial words with no subset matches.
//   (2) Store successful words already found for a partial word in another hash
//       (using pointers).

// A different and possibly better alternative (not implemented):
// Build a trie, suffix tree, or array; mark end of words on a path as it is built.
//  A custom trie would allow us to find the ends of words more easily,
//  testing each node for an "end of word" marker.  The greedy method
//  would work well.  The initial primary substring for a path would end at the
//  last "end of word" marker; the remaining substring could be tested
//  against the trie (or a hashed set) and iterated.  If the remaining substring
//  doesn't work, then the primary substring would end at the preceding
//  "end of word" marker.  If finding the path for a word is engineered to be
//  average O(1), like the hashed set, climbing the path for "end of word"
//  markers seems more efficient than shrinking the primary substring by
//  individual characters, reducing the m term in O(mn), where m is the
//  average number of substring searches per word.
// Tries and suffix trees/arrays, however, are not in the STL and are not
// trivial to build in ~ O(n) time for n objects, unlike the built-in hash
// and (hashed) set objects.
//
// Example:
//  exceptionally:
//   sample words in list are (ally, ex, except, exception, exceptionally)
//   last-1 "end of word" marker at exception - rest is ally, ally is in string set
// We would not have to test "exceptionall"/"y", "exceptional"/"ly", or "exceptiona","lly"
// The average number of lookups per word would fall.

// -----------------------------------------------------------------

// Global options for debug and pre-sorting of words, if input file is not sorted.
// Sort will add an average O(n log n) operation.
static bool kDoDebug = false;
static bool kDoPreSort = false;

// Typedefs to simplify multiple usage of these template types.
typedef std::unordered_map<size_t, std::vector<std::string *> > FLLengthMap;
typedef std::unordered_set<std::string> FLStringSet;
typedef std::__hash_const_iterator<const std::__hash_node<std::string, void *> *> FLSetIterator;

// -----------------------------------------------------------------

// Function declarations - typically placed in <file>.h, but here for simplicity and reference.
// Declaration order matches the definition order.
void toLowerString(std::string &s);
void trimEndOfLine(std::string &s);
void trimLeadingWhitespace(std::string &s);
void trimTrailingWhitespace(std::string &s);
void cleanWord(std::string &s);
bool wordIsMadeOfOtherWords(std::string &word, FLStringSet *stringSet);
bool sizeHashSortFunction(std::string *a, std::string *b);
void extractAndSortKeysFromSizeHash(FLLengthMap *sizeHash, std::vector<int> &keyVector);
int findLongestWordsOfWords(FLStringSet *stringSet, FLLengthMap *sizeHash, std::string &firstWord, std::string &secondWord);
bool hashStringFile(std::string &fileName, FLLengthMap **sizeHash, FLStringSet **stringSet);
void printUsage(bool doExit, char* argv[]);
void parseArguments(int argc, char* argv[], std::string &fileName);
int main(int argc, char* argv[]);

// -----------------------------------------------------------------

// toLowerString()
// Requires:  std::string reference
// Returns:   None
// Transforms passed string reference from upper/mixed case to lower case.
// Empty strings are ignored.
void toLowerString(std::string &s) {
  if(s.empty()) return;
  std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(std::tolower));
}

// trimEndOfLine()
// Requires:  std::string reference
// Returns:   None
// Trims end of line control characters from the passed string reference.
// Empty strings are ignored.
void trimEndOfLine(std::string &s) {
  char last;
  while(!s.empty() && ((strncmp(&(last = s.back()), "\n", 1) == 0) ||
		       (strncmp(&last, "\r", 1) == 0))) {
    //    if(kDoDebug) printf("Removed control character.\n");
    s.pop_back();
  }
}

// trimLeadingWhitespace()
// Requires:  std::string reference
// Returns:   None
// Trims leading spaces from the passed string reference, by looking
// for the last leading space and executing a single replacement over
// all leading spaces.  Empty strings are ignored.
void trimLeadingWhitespace(std::string &s) {
  size_t slen;
  if((slen = s.length()) > 0) {
    int current = 0;          // start at first and work forward
    int end = -1;
    while((current < slen) && (strncmp(&s[current], " ", 1) == 0)) {
      end = current;
      current++;
    }
    if(end >= 0) {
      s.replace(0, (end + 1), "");
      //      if(kDoDebug) printf("Removed leading whitespace.\n");
    }
  }
}

// trimTrailingWhitespace()
// Requires:  std::string reference
// Returns:   None
// Trims trailing spaces from the passed string reference, by looking
// for the first leading space and executing a single replacement over
// all trailing spaces.  Empty strings are ignored.
void trimTrailingWhitespace(std::string &s) {
  size_t slen;
  if((slen = s.length()) > 0) {
    int current = slen - 1;    // start at last and work back
    int end = -1;
    while((current >= 0) && (strncmp(&s[current], " ", 1)) == 0) {
      end = current;
      current--;
    }
    if(end >= 0) {
      s.replace(end, (slen - 1), "");
      //      if(kDoDebug) printf("Removed trailing whitespace.\n");
    }
  }
}

// cleanWord()
// Requires:  std::string reference
// Returns:   None
// Trims end of line characters, leading spaces, and trailing spaces
// from the passed string reference.  After that, it transforms the
// word to lower case for consistent matching.
void cleanWord(std::string &s) {
  // Small chance of getting empty string, so don't check here every time.
  trimEndOfLine(s);
  trimLeadingWhitespace(s);
  trimTrailingWhitespace(s);
  toLowerString(s);
}


// wordIsMadeOfOtherWords()
// Requires:  std::string reference, FLStringSet *
// Returns:   bool
//
// Recursive function looks for matches of the passed string reference and
// its substrings against the keys in the passed FLStringSet (pointer ref).
//
// (1) Start at beginning of word with a primary substring size - 1.
//     A greedy matching algorithm shrinking to a match yields more efficient
//     results than growing a small substring to a match (when using the given
//     input file and the debug statements).
//     The initial remaining substring is (size - (size - 1)), or 1.
//     NOTE:  Never having a primary substring of the full size means not
//            having to worry about matching the full word with itself.
// (2) While the substring length is greater than 0 and a match has not been
//     found, loop over the substring testing.
//     (2a)  Create and test primary substring
//     (2b)  If primary substring matches:
//              Create and test secondary substring
//              If secondary substring matches: return -true- result
//              If secondary substring doesn't match:
//                  Return result of calling function again with
//                   secondary substring
//     (2c)  If primary substring doesn't match:
//              Shrink primary substring, grow remaining substring,
//               and return to head of loop (2)
//
// NOTE:  The check for substring match could be done at the head of the function,
//        except:
//        (1) It then requires a check for call level (is it the full word?)
//        (2) Checking before calling the function on the remaining substring
//            avoids another call stack push/pop.
//
// The extra debugging statements were useful in determining to use the greedy approach.
// Call level information is not required given the construction of primary substring,
// but it could be interesting to analyze the average and max call depths.
// bool wordIsMadeOfOtherWords(std::string &word, FLStringSet *stringSet, int level) {
bool wordIsMadeOfOtherWords(std::string &word, FLStringSet *stringSet) {
  // Original base case check for empty is no longer necessary - removed.

  int sublen = word.length() - 1;  // greedy initial primary substring, never full word
  int remlen = 1;
  while(sublen > 0) {
    std::string partWord = word.substr(0, sublen);   // primary substring

    if(kDoDebug) printf("Testing partial word %s\n", partWord.c_str());
    if(stringSet->count(partWord) > 0) {
      if(kDoDebug) printf("Match found with partial word %s, start %d, length %d\n", partWord.c_str(), 0, sublen);

      std::string remainingString = word.substr(sublen, remlen);  // secondary substring
      if(stringSet->count(remainingString) > 0) {
	if(kDoDebug) printf("Match found with remaining string %s\n", remainingString.c_str());
	return true;
      }

      if(kDoDebug) printf("Calling recursive function with remaining string %s\n", remainingString.c_str());
      //      if(wordIsMadeOfOtherWords(remainingString, stringSet, level + 1))
      if(wordIsMadeOfOtherWords(remainingString, stringSet))
      return true;
    }
    sublen--;  // decrease primary substring
    remlen++;  // increase remaining substring
  }

  if(kDoDebug) printf("Word %s is not made of other words in the set\n", word.c_str());
  return false;
}


// sizeHashSortFunction()
// Requires:  std::string *, std::string *
// Returns:   bool
// The function dereferences two string pointers and compares the second to the first.
// Used for std::sort() with iterators for a vector of std::string *.
// std::sort() expects a boolean result of (a < b), not the integer results
// of (<0, 0, or >0) supplied by string.compare() or strcmp().
bool sizeHashSortFunction(std::string *a, std::string *b) {
  //  return (a->compare(*b) < 0);
  return (strcmp(a->c_str(), b->c_str()) < 0);  // faster than object compare
}


// extractAndSortKeysFromSizeHash()
// Requires:  FLLengthMap *, std::vector<int> reference (should be empty)
// Returns:   None
// The function pulls the (key, value) pairs from the string size hash, where the
// keys are integer string lengths and the values are vectors of std::string *.
// It adds the integer string lengths (keys) to the empty provided vector reference.
// -- It optionally sorts the vectors of std::string * if requested by the command
//    line option.
// It finally sorts the vector of integer key lengths.
void extractAndSortKeysFromSizeHash(FLLengthMap *sizeHash, std::vector<int> &keyVector) {
  for(auto sizeHashIter = sizeHash->begin(); sizeHashIter != sizeHash->end(); ++sizeHashIter) {
    keyVector.push_back(sizeHashIter->first);
    if(kDoPreSort) std::sort((sizeHashIter->second).begin(), (sizeHashIter->second).end(),
    			     sizeHashSortFunction);
  }
  std::sort(keyVector.begin(), keyVector.end());
}

// findLongestWordsOfWords()
// Requires:  FLStringSet *, FLLengthMap *, std::string reference, std::string reference
// Returns:   int
// The function takes pointers to populated FLStringSet and FLLengthMap objects, and references
// to string objects for the requested first and second longest words made of other words.
// (1) It extracts and sorts the string length keys (number should be <= longest words)
//     from the FLLengthMap, and initializes a loop counter to start with the largest key.
// (2) While the key index is in range:
//     (2a) Get a pointer to the vector of std::string * at the key index
//     (2b) Get the size of the vector and initialize a loop counter (word index)
//     (2c) While the word index is in range:
//          (2d) Get the std::string * at the word index and check if it is made of other words
//               If match:  increment count of found words of other words, and potentially
//                          store word in the provided first or second word references,
//                          if either has not already been found.
// The function returns the final count of words made of other words in the string set.
int findLongestWordsOfWords(FLStringSet *stringSet, FLLengthMap *sizeHash, std::string &firstWord, std::string &secondWord) {
  firstWord = secondWord = "";
  bool firstFound = false, secondFound = false;
  int countFound = 0;

  std::vector<int> keyList;
  extractAndSortKeysFromSizeHash(sizeHash, keyList);

  int keyListIndex, listLength, wordListIndex;
  for(keyListIndex = keyList.size() - 1; keyListIndex >= 0; keyListIndex--) {
    std::vector<std::string *> *wordList = &(*sizeHash)[keyList[keyListIndex]];
    listLength = wordList->size();

    for(wordListIndex = 0; wordListIndex < listLength; wordListIndex++) {
      std::string *word = (*wordList)[wordListIndex];
      if(kDoDebug) printf("Trying word %s\n", word->c_str());
      //      if(wordIsMadeOfOtherWords(*word, stringSet, 0)) {     // no longer need call level
      if(wordIsMadeOfOtherWords(*word, stringSet)) {
	if(kDoDebug) printf("Word %s is made of other words.\n", word->c_str());
	countFound++;
	if(!secondFound) {
	  if(!firstFound) {
	    firstWord = *word;
	    firstFound = true;
	  } else {
	    secondWord = *word;
	    secondFound = true;
	  }
	}
      }
    }
  }
  return countFound;
}

// hashStringFile()
// Requires:  std::string reference, FLLengthMap **, FLStringSet **
// Returns:   bool
// The function takes a string reference to a file name and attempts to open
// the file.  Failure causes an immediate exit.
// On success, it initializes the provided double pointers with a new empty
// string set and length map (hash).  For each line in the file, it "cleans"
// the word in the line and then measures its length.  It adds a non-empty
// word to the string set and adds the pointer to the inserted word into
// a vector in the length map, keyed by the length of the word.
// A set insertion failure will terminate the function early and return false.
bool hashStringFile(std::string &fileName, FLLengthMap **sizeHash, FLStringSet **stringSet) {
  std::ifstream fileStream(fileName);
  if(!fileStream.good()) {
    printf("ERROR:  Couldn't open file %s for input.\n", fileName.c_str());
    exit(1);
  }
  *stringSet = new FLStringSet;
  *sizeHash = new FLLengthMap;
  size_t lineLen;

  for(std::string line; std::getline(fileStream, line);) {
    cleanWord(line);
    lineLen = line.length();
    if(lineLen > 0) {
      //      if(kDoDebug) printf("Adding word %s of length %lu\n", line.c_str(), lineLen);
      std::pair<FLSetIterator, bool> insertResult = (*stringSet)->insert(line);
      if(insertResult.second) {
	// NOTE:  STL hash creates new vectors automatically when using [] operator with an unknown key.
      	(**sizeHash)[lineLen].push_back((std::string *)&(*(insertResult.first)));
      } else {
      	printf("ERROR:  A string was not inserted into the string set.\n");
	fileStream.close();
	return false;
      }
    }
  }
  fileStream.close();
  return true;
}

// printUsage()
// Requires:  bool, char*
// Returns:   None
// The function will print out the usage of the program, using argv[0] as the
// name of the program.  If requested, the function will terminate the program.
void printUsage(bool doExit, char* argv[]) {
  printf("Usage: %s [-[d|h|s]] <word_input_text_file>\n", argv[0]);
  printf("  The script must be called with a word input text file.\n");
  printf("  Optional arguments can be combined, can appear before or after the input file, and include:\n");
  printf("    -d:  enable printing of algorithm info for debug and analysis\n");
  printf("    -s:  sort input file before processing\n");
  printf("    -h:  print this help information and exit\n\n");
  if(doExit)
    exit(1);
}

// parseArguments()
// Requires:  int, char*, std::string reference
// Returns:   None
// The function checks all the (argc) arguments in argv[].
// For any argument with a leading dash, it collects all the
// following characters, checks them for validity, and handles them.
// Invalid options cause failure.
// On successful parsing of a word without a leading dash, it will store
// the word in the provided string reference.  Multiple blind words cause failure.
void parseArguments(int argc, char* argv[], std::string &fileName) {
  if(argc < 2)
    printUsage(true, argv);

  int carg, cargIndex, argLength;
  for(carg = 1; carg < argc; carg++) {
    std::string argstr = argv[carg];
    if(strncmp(&argstr[0], "-", 1) == 0) {
      if((argLength = argstr.length()) == 1)
	printUsage(true, argv);
      for(cargIndex = 1; cargIndex < argLength; cargIndex++) {
	if(strncmp(&argstr[cargIndex], "h", 1) == 0)
	  printUsage(true, argv);
	else if(strncmp(&argstr[cargIndex], "d", 1) == 0)
	  kDoDebug = true;
	else if(strncmp(&argstr[cargIndex], "s", 1) == 0)
	  kDoPreSort = true;
	else {
	  printf("ERROR:  %c is not a valid option.\n", argstr[cargIndex]);
	  printUsage(true, argv);
	}
      }
    } else {
      if(fileName.length() > 0) {
	printf("ERROR:  Please pass the script a maximum of one input text file name.\n");
	printUsage(true, argv);
      }
      fileName = argstr;
    }
  }
  if(fileName.length() == 0)
    printUsage(true, argv);
}

// main()
// Requires:  int, char*
// Returns:   int
// Top level function holds the file name string and pointers to the size hash and
// string set.  It parses the arguments and attempts to create the size hash and
// string set from the words in the provided file.  If successful, it retrieves
// a count of the words made from other words in the file, and saves the first
// and second longest words it finds.  It prints out the results and then cleans
// up the manually allocated objects (via "new" in hashStringFile()).
int main(int argc, char* argv[]) {
  std::string fileName = "";
  FLLengthMap *sizeHash;
  FLStringSet *stringSet;

  parseArguments(argc, argv, fileName);
  if(hashStringFile(fileName, &sizeHash, &stringSet)) {
    std::string firstWord;
    std::string secondWord;
    int count = findLongestWordsOfWords(stringSet, sizeHash, firstWord, secondWord);
    printf("First word found is %s, second word found is %s, total count found is %d.\n",
	   firstWord.c_str(), secondWord.c_str(), count);
  }

  delete stringSet;
  delete sizeHash;
}


