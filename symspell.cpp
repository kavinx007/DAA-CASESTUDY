#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <set>
#include <string>
#include <chrono>
#include <iomanip>
#include <cctype>
#include <thread>

using namespace std;

struct TrieNode {
    unordered_map<char, TrieNode*> children;
    bool isEndOfWord;
    int frequency;  // Store word frequency

    TrieNode() : isEndOfWord(false), frequency(0) {}
};

// Trie Class
class Trie {
public:
    TrieNode* root;
    unordered_map<string, int> wordFrequency; // Store word frequencies separately

    Trie() {
        root = new TrieNode();
    }

    void insert(const string &word, int frequency = 1) {
        TrieNode *node = root;
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = new TrieNode();
            }
            node = node->children[c];
        }
        node->isEndOfWord = true;
        node->frequency = frequency;
        
        // Also store in the frequency map
        wordFrequency[word] = frequency;
    }

    bool search(const string &word) {
        TrieNode* node = root;
        for (char ch : word) {
            if (node->children.find(ch) == node->children.end()) {
                return false;
            }
            node = node->children[ch];
        }
        return node->isEndOfWord;
    }

    int getFrequency(const string &word) {
        // Check if word exists in frequency map
        auto it = wordFrequency.find(word);
        if (it != wordFrequency.end()) {
            return it->second;
        }
        
        // If not in map but in trie, return trie frequency
        TrieNode *node = root;
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                return 0; // Word not found
            }
            node = node->children[c];
        }
        return (node->isEndOfWord) ? node->frequency : 0;
    }

    // Get word count in dictionary
    size_t wordCount() const {
        return wordFrequency.size();
    }

    // Cleanup to prevent memory leaks
    ~Trie() {
        deleteTrie(root);
    }

private:
    void deleteTrie(TrieNode* node) {
        for (auto& pair : node->children) {
            deleteTrie(pair.second);
        }
        delete node;
    }
};

// Progress display class
class ProgressBar {
private:
    int width;
    char completeChar;
    char incompleteChar;
    
public:
    ProgressBar(int w = 50, char complete = '=', char incomplete = ' ') 
        : width(w), completeChar(complete), incompleteChar(incomplete) {}
        
    void display(float progress) {
        int barWidth = width - 10; // Leave space for percentage
        
        cout << "\r[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) cout << completeChar;
            else cout << incompleteChar;
        }
        
        cout << "] " << fixed << setprecision(1) << (progress * 100.0) << "%" << flush;
    }
    
    void complete() {
        display(1.0);
        cout << endl;
    }
};

// String utilities
namespace StringUtils {
    string toLower(const string& s) {
        string result = s;
        transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return tolower(c); });
        return result;
    }
    
    string trim(const string& s) {
        auto start = s.begin();
        while (start != s.end() && isspace(*start)) {
            start++;
        }
        
        auto end = s.end();
        do {
            end--;
        } while (end > start && isspace(*end));
        
        return string(start, end + 1);
    }
    
    bool isAlphaOnly(const string& s) {
        return all_of(s.begin(), s.end(), [](char c) { return isalpha(c); });
    }
}

// Load valid words from a text file
bool loadValidWords(const string &filename, Trie &trie) {
    ifstream file(filename);
    string word;

    if (!file.is_open()) {
        cerr << "Error opening valid words file: " << filename << endl;
        return false;
    }

    cout << "Loading valid words from " << filename << "..." << endl;
    
    // Count lines for progress
    size_t lineCount = 0;
    size_t totalLines = 0;
    
    ifstream countFile(filename);
    string line;
    while (getline(countFile, line)) {
        totalLines++;
    }
    
    ProgressBar progressBar;
    
    // Reset file to beginning
    file.clear();
    file.seekg(0);
    
    // Now load words
    int wordCount = 0;
    while (getline(file, word)) {
        // Trim whitespace
        word = StringUtils::trim(word);
        word = StringUtils::toLower(word);
        
        if (!word.empty() && StringUtils::isAlphaOnly(word)) {
            trie.insert(word, 1); // Default frequency of 1
            wordCount++;
        }
        
        lineCount++;
        if (lineCount % 1000 == 0 || lineCount == totalLines) {
            progressBar.display(static_cast<float>(lineCount) / totalLines);
        }
    }
    
    progressBar.complete();
    cout << "Valid words loaded: " << wordCount << " words processed." << endl;
    return true;
}

// Load frequency data from CSV
bool loadFrequencyData(const string &filename, Trie &trie) {
    ifstream file(filename);
    string line, word;
    int count;

    if (!file.is_open()) {
        cerr << "Error opening frequency file: " << filename << endl;
        return false;
    }

    cout << "Loading frequency data from " << filename << "..." << endl;
    
    // Count lines for progress
    size_t lineCount = 0;
    size_t totalLines = 0;
    
    ifstream countFile(filename);
    string countLine;
    while (getline(countFile, countLine)) {
        totalLines++;
    }
    
    ProgressBar progressBar;
    
    // Reset file to beginning
    file.clear();
    file.seekg(0);
    
    // Skip header if present
    getline(file, line);
    if (line != "word,count") {
        // If no header, process the first line
        stringstream ss(line);
        getline(ss, word, ',');
        word = StringUtils::toLower(word);
        ss >> count;
        if (!word.empty() && trie.search(word)) {
            trie.wordFrequency[word] = count;
        }
    }
    
    // Process the rest of the file
    int wordCount = 0;
    int updatedCount = 0;
    lineCount++;
    
    while (getline(file, line)) {
        stringstream ss(line);
        getline(ss, word, ',');
        word = StringUtils::toLower(word);
        ss >> count;
        
        // Skip empty words
        if (word.empty()) continue;
        
        // Update frequency only if word exists in the valid words dictionary
        if (trie.search(word)) {
            trie.wordFrequency[word] = count;
            updatedCount++;
        }
        
        wordCount++;
        lineCount++;
        
        if (lineCount % 1000 == 0 || lineCount == totalLines) {
            progressBar.display(static_cast<float>(lineCount) / totalLines);
        }
    }
    
    progressBar.complete();
    cout << "Frequency data loaded: " << wordCount << " entries processed, " 
         << updatedCount << " frequencies updated." << endl;
    return true;
}

// Generate edits for SymSpell algorithm
vector<string> generateEdits(const string &word, int maxDistance = 2) {
    vector<string> edits;
    set<string> uniqueEdits; // To avoid duplicates
    
    // Handle empty word case
    if (word.empty()) return edits;
    
    // Deletions (remove one character)
    for (size_t i = 0; i < word.length(); i++) {
        string edit = word.substr(0, i) + word.substr(i + 1);
        if (uniqueEdits.insert(edit).second) {
            edits.push_back(edit);
        }
    }
    
    // Transpositions (swap adjacent characters)
    for (size_t i = 0; i < word.length() - 1; i++) {
        string edit = word;
        swap(edit[i], edit[i + 1]);
        if (uniqueEdits.insert(edit).second) {
            edits.push_back(edit);
        }
    }
    
    // Substitutions (change one character to another)
    for (size_t i = 0; i < word.length(); i++) {
        string base = word;
        for (char c = 'a'; c <= 'z'; c++) {
            if (word[i] != c) {
                base[i] = c;
                if (uniqueEdits.insert(base).second) {
                    edits.push_back(base);
                }
            }
        }
    }
    
    // Insertions (add one character)
    for (size_t i = 0; i <= word.length(); i++) {
        for (char c = 'a'; c <= 'z'; c++) {
            string edit = word.substr(0, i) + c + word.substr(i);
            if (uniqueEdits.insert(edit).second) {
                edits.push_back(edit);
            }
        }
    }
    
    // For edit distance 2, apply edits to the edits (more efficient implementation)
    if (maxDistance > 1) {
        size_t origSize = edits.size();
        for (size_t i = 0; i < origSize; i++) {
            // Only generate deletions for edit distance 2 (faster but still effective)
            for (size_t j = 0; j < edits[i].length(); j++) {
                string edit2 = edits[i].substr(0, j) + edits[i].substr(j + 1);
                if (uniqueEdits.insert(edit2).second) {
                    edits.push_back(edit2);
                }
            }
        }
    }
    
    return edits;
}

// Structure to store correction suggestions with their frequencies
struct Suggestion {
    string word;
    int frequency;
    int editDistance;
    
    Suggestion(const string& w, int f, int d) : word(w), frequency(f), editDistance(d) {}
    
    // For sorting by edit distance first, then by frequency
    bool operator<(const Suggestion& other) const {
        if (editDistance != other.editDistance) {
            return editDistance > other.editDistance; // Lower distance is better
        }
        return frequency < other.frequency; // Higher frequency is better
    }
};

// Function to calculate Levenshtein edit distance
int levenshteinDistance(const string& s1, const string& s2) {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    
    vector<vector<int>> d(len1 + 1, vector<int>(len2 + 1));
    
    for (size_t i = 0; i <= len1; i++) {
        d[i][0] = i;
    }
    
    for (size_t j = 0; j <= len2; j++) {
        d[0][j] = j;
    }
    
    for (size_t i = 1; i <= len1; i++) {
        for (size_t j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            
            d[i][j] = min({
                d[i - 1][j] + 1,      // deletion
                d[i][j - 1] + 1,      // insertion
                d[i - 1][j - 1] + cost // substitution
            });
            
            // Transposition
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1]) {
                d[i][j] = min(d[i][j], d[i - 2][j - 2] + cost);
            }
        }
    }
    
    return d[len1][len2];
}

vector<string> correctSpelling(Trie &trie, const string &inputWord, int maxResults = 5) {
    // If the word exists in dictionary, return it as is
    if (trie.search(inputWord)) {
        return {inputWord};
    }
    
    // Priority queue to store suggestions sorted by edit distance and frequency
    priority_queue<Suggestion> suggestions;
    
    // Generate edits
    vector<string> edits = generateEdits(inputWord, 2);
    
    // Check each edit in the trie
    for (const string& edit : edits) {
        if (trie.search(edit)) {
            int frequency = trie.getFrequency(edit);
            int distance = levenshteinDistance(inputWord, edit);
            suggestions.push(Suggestion(edit, frequency, distance));
        }
    }
    
    // Extract top suggestions
    vector<string> results;
    while (!suggestions.empty() && results.size() < maxResults) {
        results.push_back(suggestions.top().word);
        suggestions.pop();
    }
    
    // If no suggestions found
    if (results.empty()) {
        results.push_back("No suggestion");
    }
    
    return results;
}

// Function to clear console screen (cross-platform)
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Function to display animated banner
void displayBanner() {
    clearScreen();
    cout << "\n";
    cout << " ┌───────────────────────────────────┐\n";
    cout << " │                                   │\n";
    cout << " │        SYMSPELL CHECKER           │\n";
    cout << " │                                   │\n";
    cout << " └───────────────────────────────────┘\n\n";
}

// Function to display help information
void displayHelp() {
    cout << "\nCommands:\n";
    cout << "  <word>      - Check spelling of a word\n";
    cout << "  !quit       - Exit the program\n";
    cout << "  !help       - Display this help message\n";
    cout << "  !clear      - Clear the screen\n";
    cout << "  !stats      - Show dictionary statistics\n\n";
}

// Function to display dictionary statistics
void displayStats(const Trie &trie) {
    cout << "\nDictionary Statistics:\n";
    cout << "Total words in dictionary: " << trie.wordCount() << "\n";
    cout << "Memory usage (approximate): " << (trie.wordCount() * (sizeof(string) + sizeof(int)) / 1024) << " KB\n\n";
}

// Main function
int main(int argc, char* argv[]) {
    // Initialize dictionary
    Trie dictionary;
    string validWordsFile = "words_alpha.txt";
    string frequencyFile = "english_word_frequency.csv";
    
    // Check if dictionary files are provided
    if (argc > 1) {
        validWordsFile = argv[1];
    }
    if (argc > 2) {
        frequencyFile = argv[2];
    }
    
    // Load dictionary
    if (!loadValidWords(validWordsFile, dictionary)) {
        cerr << "Failed to load valid words. Exiting." << endl;
        return 1;
    }
    
    // Load frequency data if available
    if (!loadFrequencyData(frequencyFile, dictionary)) {
        cout << "No frequency data loaded. Using default frequencies." << endl;
    }
    
    // Display welcome message
    displayBanner();
    cout << "Welcome to SymSpell Checker!" << endl;
    cout << "Dictionary loaded with " << dictionary.wordCount() << " words." << endl;
    displayHelp();
    
    // Main interaction loop
    string input;
    while (true) {
        cout << "\nEnter a word to check (or !quit to exit): ";
        getline(cin, input);
        
        // Convert to lowercase
        input = StringUtils::toLower(input);
        input = StringUtils::trim(input);
        
        // Check for commands
        if (input == "!quit" || input == "!exit") {
            cout << "Exiting SymSpell Checker. Goodbye!" << endl;
            break;
        } else if (input == "!help") {
            displayHelp();
        } else if (input == "!clear") {
            displayBanner();
        } else if (input == "!stats") {
            displayStats(dictionary);
        } else if (!input.empty()) {
            // Check spelling
            auto start = chrono::high_resolution_clock::now();
            
            vector<string> suggestions = correctSpelling(dictionary, input);
            
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
            
            // Display results
            if (suggestions.size() == 1 && suggestions[0] == input) {
                cout << "✓ \"" << input << "\" is spelled correctly." << endl;
            } else if (suggestions[0] == "No suggestion") {
                cout << "✗ \"" << input << "\" not found. No suggestions available." << endl;
            } else {
                cout << "✗ \"" << input << "\" not found. Did you mean:" << endl;
                for (size_t i = 0; i < suggestions.size(); i++) {
                    cout << "  " << (i + 1) << ". " << suggestions[i] 
                         << " (freq: " << dictionary.getFrequency(suggestions[i]) << ")" << endl;
                }
            }
            
            cout << "Time taken: " << duration.count() / 1000.0 << " ms" << endl;
        }
    }
    
    return 0;
}