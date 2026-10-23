// ===== SECTION 0: includes, Position, EditorError, helpers =====
#include <iostream>    // cout, cin
#include <fstream>     // reading and writing files
#include <string>      // string
#include <stdexcept>   // runtime_error (used by EditorError)
#include <cstdlib>     // exit
using namespace std;

// A spot in the document. Inside the code BOTH numbers count from 0.
// (The user types from 1; only the menu converts.)
struct Position {
    int line;   // which line (0 = first line)
    int col;    // which letter on that line (0 = before the first letter)
};

// Every problem in the editor is reported by throwing one of these.
// Example:  throw EditorError("Line number out of range");
// The menu catches it and shows the message with e.what().
class EditorError : public runtime_error {
public:
    EditorError(const string& message) : runtime_error(message) {}
};

// Is spot a strictly before spot b?
bool isBefore(Position a, Position b) {
    if (a.line < b.line) return true;
    if (a.line > b.line) return false;
    return a.col < b.col;           // same line: compare the letter numbers
}

// Are the two spots exactly the same?
bool isSame(Position a, Position b) {
    return a.line == b.line && a.col == b.col;
}

// A "word letter" is a letter, a digit or an underscore.
bool isWordChar(char c) {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '_') return true;
    return false;
}

// Turns a capital letter into a small one. Everything else stays the same.
char toLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c - 'A' + 'a');
    }
    return c;
}

// Where does the text END if we insert it at spot 'start'?
//   no line break in text -> same line, further right
//   with line breaks      -> that many lines lower; the column is the
//                            length of the text after the last line break
Position endOf(Position start, const string& text) {
    int breaks = 0;        // how many '\n' are in the text
    int lastBreak = -1;    // position of the last '\n' (-1 = none)
    for (int i = 0; i < (int)text.size(); i++) {
        if (text[i] == '\n') {
            breaks++;
            lastBreak = i;
        }
    }

    Position result;
    if (breaks == 0) {
        result.line = start.line;
        result.col  = start.col + (int)text.size();
    } else {
        result.line = start.line + breaks;
        result.col  = (int)text.size() - lastBreak - 1;
    }
    return result;
}

// Find the n-th (1-based) WHOLE-WORD copy of 'word' inside 'text'.
// Returns where it starts (0-based), or -1 if there is no n-th copy.
// "Whole word" = the letters just before and just after are NOT word letters,
// so "the" is not found inside "other".
int findNthWord(const string& text, const string& word, int n) {
    int len = (int)text.size();
    int m = (int)word.size();
    int seen = 0;
    for (int i = 0; i + m <= len; i++) {
        int j = 0;
        while (j < m && text[i + j] == word[j]) {
            j++;
        }
        if (j == m) {
            bool leftOk  = (i == 0)       || !isWordChar(text[i - 1]);
            bool rightOk = (i + m == len) || !isWordChar(text[i + m]);
            if (leftOk && rightOk) {
                seen++;
                if (seen == n) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// ===== SECTION 1: Line list (M1) =====


// STEP 1: one card = one line of the document
struct Line {
    string text;   // the words on this line
    Line* prev;    // the card above (nullptr if this is the first card)
    Line* next;    // the card below (nullptr if this is the last card)
};

class LineList {
private:
    Line* head;    // the first card
    Line* tail;    // the last card
    int count;     // how many cards there are

public:
    // STEP 2: start empty, and clean up when the program ends
    LineList() {
        head = nullptr;
        tail = nullptr;
        count = 0;
    }
    ~LineList() {
        clear();
    }
    LineList(const LineList&) = delete;              // copying is not allowed
    LineList& operator=(const LineList&) = delete;   // (it would cause double-delete bugs)

    int size() const { return count; }
    Line* first() const { return head; }
    Line* last() const { return tail; }

    // STEP 3: add a new card at the END
    void append(const string& text) {
        Line* card = new Line;
        card->text = text;
        card->prev = tail;          // the old last card is above the new one
        card->next = nullptr;       // nothing is below the new last card
        if (tail == nullptr) {
            head = card;            // the list was empty: it is also the first card
        } else {
            tail->next = card;      // the old last card now points down to the new one
        }
        tail = card;
        count++;
    }

    // STEP 4: find card number i (0 = first card)
    Line* getNode(int i) const {
        if (i < 0 || i >= count) {
            throw EditorError("Line number out of range");
        }
        Line* card;
        if (i < count / 2) {                     // closer to the top: walk down from head
            card = head;
            for (int k = 0; k < i; k++) {
                card = card->next;
            }
        } else {                                 // closer to the bottom: walk up from tail
            card = tail;
            for (int k = count - 1; k > i; k--) {
                card = card->prev;
            }
        }
        return card;
    }

    // STEP 5: add a new card right BELOW the card 'card'
    Line* insertAfter(Line* card, const string& text) {
        if (card == nullptr) {
            throw EditorError("Cannot insert after nothing");
        }
        Line* fresh = new Line;
        fresh->text = text;
        fresh->prev = card;              // the new card sits below 'card'
        fresh->next = card->next;        // and above whatever was below 'card'
        if (card->next != nullptr) {
            card->next->prev = fresh;    // the card below now points back up to the new one
        } else {
            tail = fresh;                // 'card' was the last card, so the new one is last
        }
        card->next = fresh;
        count++;
        return fresh;
    }

    // STEP 6: take a card out and delete it
    void removeNode(Line* card) {
        if (card == nullptr) {
            throw EditorError("Cannot remove nothing");
        }
        if (card->prev != nullptr) {
            card->prev->next = card->next;   // the card above skips over this one
        } else {
            head = card->next;               // this was the first card
        }
        if (card->next != nullptr) {
            card->next->prev = card->prev;   // the card below skips over this one
        } else {
            tail = card->prev;               // this was the last card
        }
        delete card;
        count--;
    }

    // STEP 7: delete every card
    void clear() {
        Line* card = head;
        while (card != nullptr) {
            Line* below = card->next;        // remember the next card BEFORE deleting this one
            delete card;
            card = below;
        }
        head = nullptr;
        tail = nullptr;
        count = 0;
    }

    // STEP 8: self-check. Call it in tests; it throws if any link is broken.
    void checkIntegrity() const {
        int n = 0;
        Line* before = nullptr;
        for (Line* card = head; card != nullptr; card = card->next) {
            if (card->prev != before) throw EditorError("prev link is broken");
            before = card;
            n++;
            if (n > count) throw EditorError("count is wrong (or the list loops)");
        }
        if (before != tail) throw EditorError("tail is wrong");
        if (n != count) throw EditorError("count is wrong");
    }
};


// ===== SECTION 2: Stack (+ the Operation note it will hold) =====

// A pile of plates. You only add or remove at the TOP.
// "template <typename T>" means: T is a placeholder for any type,
// so  Stack<int>  is a pile of ints and  Stack<Operation>  is a pile of undo notes.
template <typename T>
class Stack {
private:
    struct StackNode {
        T data;              // the item stored on this plate
        StackNode* below;    // the plate underneath (nullptr if this is the bottom plate)
    };
    StackNode* topNode;      // the plate on top (nullptr if the pile is empty)
    int count;               // how many plates

public:
    Stack() {
        topNode = nullptr;
        count = 0;
    }
    ~Stack() {
        clear();
    }
    Stack(const Stack&) = delete;              // copying is not allowed
    Stack& operator=(const Stack&) = delete;

    bool isEmpty() const { return topNode == nullptr; }
    int size() const { return count; }

    // put an item on top
    void push(const T& item) {
        StackNode* plate = new StackNode;
        plate->data = item;
        plate->below = topNode;     // the new plate sits on the old top plate
        topNode = plate;            // and becomes the new top
        count++;
    }

    // take the top item off and give it back
    T pop() {
        if (topNode == nullptr) {
            throw EditorError("Stack is empty");
        }
        StackNode* plate = topNode;
        T item = plate->data;       // copy the item out BEFORE deleting the plate
        topNode = plate->below;
        delete plate;
        count--;
        return item;
    }

    // look at the top item without removing it
    T& top() {
        if (topNode == nullptr) {
            throw EditorError("Stack is empty");
        }
        return topNode->data;
    }

    // throw away every plate
    void clear() {
        while (topNode != nullptr) {
            StackNode* plate = topNode;
            topNode = plate->below;
            delete plate;
        }
        count = 0;
    }
};

// One note on the undo pile: what happened to the document.
enum class OpType { Insert, Delete };   // was text ADDED or REMOVED?

struct Operation {
    OpType type;       // Insert or Delete
    Position start;    // where it happened (0-based)
    string text;       // the text that was added / removed (may contain '\n')
    int group;         // notes with the same number are undone together
};


// ===== SECTION 3: DynArray (a row of boxes that grows) + Match =====

// A row of numbered boxes (0, 1, 2, ...). When it gets full it makes a
// row TWICE as big, copies everything across, and carries on.
// Used for lists whose size is not known in advance (like search results).
template <typename T>
class DynArray {
private:
    T* items;        // the row of boxes
    int count;       // how many boxes are used
    int capacity;    // how many boxes exist

public:
    DynArray() {
        items = nullptr;
        count = 0;
        capacity = 0;
    }
    ~DynArray() {
        delete[] items;
    }
    DynArray(const DynArray&) = delete;              // copying is not allowed
    DynArray& operator=(const DynArray&) = delete;

    int size() const { return count; }

    // add one item at the end
    void add(const T& item) {
        T copy = item;                               // copy FIRST (item might live inside this array)
        if (count == capacity) {                     // the row is full: make a bigger one
            int newCapacity;
            if (capacity == 0) {
                newCapacity = 4;
            } else {
                newCapacity = capacity * 2;
            }
            T* bigger = new T[newCapacity];
            for (int i = 0; i < count; i++) {
                bigger[i] = items[i];                // copy the old items across
            }
            delete[] items;                          // free the old row
            items = bigger;
            capacity = newCapacity;
        }
        items[count] = copy;
        count++;
    }

    // get the item in box i (0 = first box)
    T& get(int i) {
        if (i < 0 || i >= count) {
            throw EditorError("Index out of range");
        }
        return items[i];
    }

    // same thing, for arrays that are marked const (read only)
    const T& get(int i) const {
        if (i < 0 || i >= count) {
            throw EditorError("Index out of range");
        }
        return items[i];
    }

    // forget all items (the boxes stay, ready to be reused)
    void clear() {
        count = 0;
    }
};

// One search result: where a match was found (0-based).
struct Match {
    int line;
    int col;
};

// ===== SECTION 4: Document class (declaration only: every function name listed) =====

class Document {
private:
    LineList lines;               // the document: one card per line (never empty: at least 1 line)
    Stack<Operation> undoStack;   // notes for UNDO
    Stack<Operation> redoStack;   // notes for REDO
    string filename;              // "" = not saved yet
    bool dirty;                   // true = there are unsaved changes
    int nextGroupId;              // next group number to hand out
    int activeGroup;              // -1 = no group is open
    string clipboard; 

    void record(const Operation& note);        // put a note on the undo pile
    void checkPosition(Position p) const;      // throws EditorError if p is not a real spot
      Position locateWord(int line, const string& word, int which) const;

public:
    Document();

    // ---- Step 2: the two basic actions + display (Section 5) ----
    void   insertAt(Position pos, const string& text, bool saveNote = true);
    string deleteRange(Position a, Position b, bool saveNote = true);   // b is NOT included
    string getText(Position a, Position b) const;                       // read only
    string render() const;                                              // whole document, with line numbers
    string renderLines(int firstLine, int lastLine) const;              // some lines, with line numbers
    int    lineCount() const;
    int    charCount() const;
    bool   isEmpty() const;
    bool   isDirty() const { return dirty; }

    // ---- later steps (declared now, written later in Section 6) ----
    // files
    void newDocument();
    void openFile(const string& path);
    void saveFile();
    void saveFileAs(const string& path);
    bool hasFilename() const { return !filename.empty(); }
    // insert
    void insertText(Position pos, const string& text);
    void insertNewLine(Position pos);
    void insertBlankLineAfter(int line);
    void insertBlankLineBefore(int line);
    // delete
    void deleteChar(Position pos);
    void deleteWord(Position pos);
        int  countWordOnLine(int line, const string& word) const;
    void deleteWordOnLine(int line, const string& word, int which);
    void deleteCharInWord(int line, const string& word, char ch, int which);
    void deleteLines(int firstLine, int lastLine);
    // undo / redo
    void beginGroup();
    void endGroup();
    void undo();
    void redo();
    // search / replace
    void findAll(const string& pattern, bool ignoreCase, bool wholeWord, DynArray<Match>& found) const;
    void replaceOne(const string& pattern, const string& replacement, int k, bool ignoreCase, bool wholeWord);
    int  replaceAll(const string& pattern, const string& replacement, bool ignoreCase, bool wholeWord);
    // cut / copy / paste
    void copyRange(Position a, Position b);
    void copyLines(int firstLine, int lastLine);
    void cutRange(Position a, Position b);
    void cutLines(int firstLine, int lastLine);
    void pasteAt(Position pos);
    void pasteLinesBefore(int line);
};

// ===== SECTION 5: the two basic actions + display =====

// ---------- start: the document always has at least one line ----------
Document::Document() {
    filename = "";
    dirty = false;
    nextGroupId = 0;
    activeGroup = -1;
    lines.append("");             // one empty line = an "empty document"
}

// ---------- helper: put a note on the undo pile ----------
void Document::record(const Operation& note) {
    Operation copy = note;
    if (activeGroup != -1) {
        copy.group = activeGroup;          // inside a group: share its number
    } else {
        copy.group = nextGroupId;          // alone: take a fresh number
        nextGroupId++;
    }
    undoStack.push(copy);
    redoStack.clear();                     // a new edit makes the old redo notes useless
}

// ---------- helper: is p a real spot? ----------
void Document::checkPosition(Position p) const {
    if (p.line < 0 || p.line >= lines.size()) {
        throw EditorError("Line number out of range");
    }
    Line* card = lines.getNode(p.line);
    if (p.col < 0 || p.col > (int)card->text.size()) {
        throw EditorError("Column out of range");
    }
}

// ---------- BASIC ACTION 1: add text at a spot ----------
void Document::insertAt(Position pos, const string& text, bool saveNote) {
    // 1. check everything BEFORE changing anything
    if (text.empty()) {
        throw EditorError("Nothing to insert");
    }
    checkPosition(pos);

    // 2. split the line at the spot into a left part and a right part
    Line* card = lines.getNode(pos.line);
    string left = card->text.substr(0, pos.col);
    string right = card->text.substr(pos.col);

    // 3. cut the new text into pieces at every line break
    DynArray<string> pieces;
    string part = "";
    for (int i = 0; i < (int)text.size(); i++) {
        if (text[i] == '\n') {
            pieces.add(part);
            part = "";
        } else {
            part += text[i];
        }
    }
    pieces.add(part);                      // the last piece (there is always at least one)

    // 4. put the pieces into the document
    if (pieces.size() == 1) {
        card->text = left + pieces.get(0) + right;          // no line break: one line grows
    } else {
        card->text = left + pieces.get(0);                   // first piece joins the left part
        Line* cursor = card;
        for (int i = 1; i < pieces.size() - 1; i++) {        // middle pieces become new lines
            cursor = lines.insertAfter(cursor, pieces.get(i));
        }
        lines.insertAfter(cursor, pieces.get(pieces.size() - 1) + right);  // last piece + right part
    }

    // 5. remember it for undo
    if (saveNote) {
        Operation note;
        note.type = OpType::Insert;
        note.start = pos;
        note.text = text;
        note.group = 0;                    // record() gives the real group number
        record(note);
    }
    dirty = true;
}

// ---------- read the text between two spots (changes nothing) ----------
string Document::getText(Position a, Position b) const {
    checkPosition(a);
    checkPosition(b);
    if (!isBefore(a, b)) {
        throw EditorError("Start must come before end");
    }

    Line* cardA = lines.getNode(a.line);
    if (a.line == b.line) {
        return cardA->text.substr(a.col, b.col - a.col);     // all on one line
    }

    string result = cardA->text.substr(a.col);               // the rest of A's line
    Line* card = cardA->next;
    for (int i = a.line + 1; i < b.line; i++) {              // every line in between
        result += "\n" + card->text;
        card = card->next;
    }
    result += "\n" + card->text.substr(0, b.col);            // the start of B's line
    return result;
}

// ---------- BASIC ACTION 2: remove text between two spots ----------
string Document::deleteRange(Position a, Position b, bool saveNote) {
    // 1. check everything and copy the text out (getText checks the spots for us)
    string removed = getText(a, b);

    // 2. remove it
    Line* cardA = lines.getNode(a.line);
    if (a.line == b.line) {
        cardA->text = cardA->text.substr(0, a.col) + cardA->text.substr(b.col);
    } else {
        Line* cardB = lines.getNode(b.line);
        // A's line = what is before a  +  what is after b (on B's line)
        cardA->text = cardA->text.substr(0, a.col) + cardB->text.substr(b.col);
        // every line below A, up to and including B, is now unnecessary
        int howMany = b.line - a.line;
        for (int i = 0; i < howMany; i++) {
            lines.removeNode(cardA->next);
        }
    }

    // 3. remember it for undo
    if (saveNote) {
        Operation note;
        note.type = OpType::Delete;
        note.start = a;
        note.text = removed;
        note.group = 0;
        record(note);
    }
    dirty = true;
    return removed;
}

// ---------- display ----------
int Document::lineCount() const {
    return lines.size();
}

int Document::charCount() const {
    int total = 0;
    for (Line* card = lines.first(); card != nullptr; card = card->next) {
        total += (int)card->text.size();       // line breaks are not counted
    }
    return total;
}

bool Document::isEmpty() const {
    return lines.size() == 1 && lines.first()->text.empty();
}

// some lines, each with its line number (numbers are added only here, never stored)
string Document::renderLines(int firstLine, int lastLine) const {
    if (firstLine < 0 || lastLine >= lines.size() || firstLine > lastLine) {
        throw EditorError("Line range out of range");
    }
    int width = (int)to_string(lines.size()).size();      // so the numbers line up
    string out = "";
    Line* card = lines.getNode(firstLine);
    for (int i = firstLine; i <= lastLine; i++) {
        string number = to_string(i + 1);                 // the user counts from 1
        while ((int)number.size() < width) {
            number = " " + number;
        }
        out += number + " | " + card->text + "\n";
        card = card->next;
    }
    return out;
}

string Document::render() const {
    if (isEmpty()) {
        return "(empty document)\n";
    }
    return renderLines(0, lines.size() - 1);
}

// ===== SECTION 6: files, insert, delete, search, replace, undo, clipboard =====
// Paste this whole block into main.cpp, right after Section 5.
// Rule: each person edits ONLY the sub-section that carries their name.
// Every stub throws EditorError("TODO") so the file always compiles.


// ---------- 6A. FILES + (see 6D for undo) ----------  OWNER: MEMBER A
 
void Document::newDocument() {
    lines.clear();
    lines.append("");            // document must always have at least 1 line
    undoStack.clear();
    redoStack.clear();
    filename = "";
    dirty = false;
    nextGroupId = 0;
    activeGroup = -1;
}
 
void Document::openFile(const string& path) {
    ifstream in(path);
    if (!in) {
        throw EditorError("Cannot open file: " + path);
    }
 
    // Read the whole file first: if it fails, the current document stays untouched
    DynArray<string> loaded;
    string row;
    while (getline(in, row)) {
        if (!row.empty() && row[row.size() - 1] == '\r') {
            row.erase(row.size() - 1);       // remove '\r' from Windows files
        }
        loaded.add(row);
    }
    if (in.bad()) {
        throw EditorError("Error while reading file: " + path);
    }
 
    lines.clear();
    for (int i = 0; i < loaded.size(); i++) {
        lines.append(loaded.get(i));
    }
    if (lines.size() == 0) {
        lines.append("");                    // empty file -> one empty line
    }
 
    undoStack.clear();
    redoStack.clear();
    filename = path;
    dirty = false;
    nextGroupId = 0;
    activeGroup = -1;
}
 
void Document::saveFile() {
    if (filename.empty()) {
        throw EditorError("No file name yet. Use Save As");
    }
    saveFileAs(filename);
}
 
void Document::saveFileAs(const string& path) {
    if (path.empty()) {
        throw EditorError("File name is empty");
    }
 
    ofstream out(path);
    if (!out) {
        throw EditorError("Cannot create file: " + path);
    }
 
    // Every line is written followed by '\n' (openFile reads it back the same way)
    for (Line* card = lines.first(); card != nullptr; card = card->next) {
        out << card->text << '\n';
    }
 
    out.close();
    if (out.fail()) {
        throw EditorError("Error while writing file: " + path);
    }
 
    filename = path;
    dirty = false;
}
 
 
// ---------- 6B. INSERT ----------  OWNER: STARTER (done)

// Add text at a spot. insertAt already checks the spot, rejects empty text
// and splits at every '\n', so this is just the public entry point.
void Document::insertText(Position pos, const string& text) {
    insertAt(pos, text);
}

// Press Enter at a spot: the line splits in two.
// A line break is just the character '\n', which insertAt already understands.
void Document::insertNewLine(Position pos) {
    insertAt(pos, "\n");
}

// Add an empty line right BELOW line number 'line' (0-based).
// Trick: put a '\n' at the END of that line. The text after it (nothing) becomes the new line.
void Document::insertBlankLineAfter(int line) {
    Line* card = lines.getNode(line);                 // throws if 'line' is out of range
    Position endOfLine;
    endOfLine.line = line;
    endOfLine.col  = (int)card->text.size();
    insertAt(endOfLine, "\n");
}

// Add an empty line right ABOVE line number 'line' (0-based).
// Trick: put a '\n' at the START of that line. Everything on it moves down.
void Document::insertBlankLineBefore(int line) {
    lines.getNode(line);                              // only used to check that 'line' exists
    Position startOfLine;
    startOfLine.line = line;
    startOfLine.col  = 0;
    insertAt(startOfLine, "\n");
}


// ---------- 6C. DELETE ----------
// deleteChar and deleteWord: OWNER: MEMBER B
// Delete ONE character at pos.
// If pos is at the END of a line (col == line length), the thing to delete is
// the line break: from (line, len) to (line+1, 0), so the next line joins this one.
// Throws EditorError if there is nothing to delete.
void Document::deleteChar(Position pos) {
    checkPosition(pos);                       // the line and column must exist

    Line* card = lines.getNode(pos.line);
    int len = (int)card->text.size();

    Position after;                           // the spot just AFTER what we delete
    if (pos.col < len) {
        after.line = pos.line;                // a letter: delete up to the next column
        after.col  = pos.col + 1;
    } else {
        if (pos.line == lines.size() - 1) {   // end of the LAST line: no line break there
            throw EditorError("Nothing to delete");
        }
        after.line = pos.line + 1;            // the line break: (line, len) -> (line+1, 0)
        after.col  = 0;
    }
    deleteRange(pos, after);                  // removes it AND puts one note on the undo pile
}

// Delete the WORD at pos (plus one neighbouring space, so no double space is left).
// pos may be anywhere inside the word, or just after its last letter.
// Throws EditorError if there is no word at pos.
void Document::deleteWord(Position pos) {
    checkPosition(pos);

    Line* card = lines.getNode(pos.line);
    const string& text = card->text;
    int len = (int)text.size();

    // 1. find one letter that belongs to the word
    int hit;
    if (pos.col < len && isWordChar(text[pos.col])) {
        hit = pos.col;                        // pos is on a word letter
    } else if (pos.col > 0 && isWordChar(text[pos.col - 1])) {
        hit = pos.col - 1;                    // pos is just after a word
    } else {
        throw EditorError("No word at this position");
    }

    // 2. walk left and right to find where the word starts and ends
    int start = hit;
    while (start > 0 && isWordChar(text[start - 1])) {
        start--;
    }
    int end = hit + 1;                        // end is just AFTER the last letter
    while (end < len && isWordChar(text[end])) {
        end++;
    }

    // 3. also remove one space: after the word if there is one, else before it
    if (end < len && text[end] == ' ') {
        end++;
    } else if (start > 0 && text[start - 1] == ' ') {
        start--;
    }

    // 4. delete the range (one undoable step)
    Position a;
    a.line = pos.line;
    a.col  = start;
    Position b;
    b.line = pos.line;
    b.col  = end;
    deleteRange(a, b);
}

// How many times does 'word' appear (as a whole word) on line number 'line' (0-based)?
// The menu calls this first: if the answer is 2 or more, it asks the user "which one?".
int Document::countWordOnLine(int line, const string& word) const {
    if (word.empty()) {
        throw EditorError("The word is empty");
    }
    for (int i = 0; i < (int)word.size(); i++) {
        if (!isWordChar(word[i])) {
            throw EditorError("Type ONE word (letters, digits or underscore)");
        }
    }
    Line* card = lines.getNode(line);            // throws "Line number out of range"
    int total = 0;
    while (findNthWord(card->text, word, total + 1) != -1) {
        total++;
    }
    return total;
}

// Where does copy number 'which' (1 = leftmost) of 'word' start on line 'line'?
// Throws if the word is not there or 'which' is too big.
Position Document::locateWord(int line, const string& word, int which) const {
    int total = countWordOnLine(line, word);     // also checks line and word
    if (total == 0) {
        throw EditorError("Word not found on that line");
    }
    if (which < 1 || which > total) {
        throw EditorError("That occurrence does not exist");
    }
    Position p;
    p.line = line;
    p.col  = findNthWord(lines.getNode(line)->text, word, which);
    return p;
}

// Delete copy number 'which' of 'word' on line 'line'.
void Document::deleteWordOnLine(int line, const string& word, int which) {
    Position p = locateWord(line, word, which);
    deleteWord(p);                               // does the real work + the undo note
}

// Delete the first 'ch' inside copy number 'which' of 'word' on line 'line'.
void Document::deleteCharInWord(int line, const string& word, char ch, int which) {
    Position p = locateWord(line, word, which);
    size_t k = word.find(ch);                    // where is ch inside the word?
    if (k == string::npos) {
        throw EditorError("That character is not in the word");
    }
    p.col = p.col + (int)k;                      // the spot of that letter on the line
    deleteChar(p);                               // does the real work + the undo note
}
// deleteLines: OWNER: STARTER (done). Kept by the starter because cutLines needs it.
// Removes whole lines firstLine..lastLine (both included, 0-based) as ONE undoable step.
// The document must never have 0 lines, so there are three cases:
void Document::deleteLines(int firstLine, int lastLine) {
    if (firstLine < 0 || lastLine >= lines.size() || firstLine > lastLine) {
        throw EditorError("Line range out of range");
    }

    Position a;
    Position b;

    if (lastLine < lines.size() - 1) {
        // CASE 1: a line exists below the range.
        // Delete from the start of firstLine to the start of the line below.
        // That removes the lines AND their line breaks.
        a.line = firstLine;    a.col = 0;
        b.line = lastLine + 1; b.col = 0;
    } else if (firstLine > 0) {
        // CASE 2: the range reaches the last line, but a line exists above it.
        // Delete from the END of the line above to the end of the last line.
        // That removes the line break above the range, and the lines themselves.
        a.line = firstLine - 1;
        a.col  = (int)lines.getNode(firstLine - 1)->text.size();
        b.line = lastLine;
        b.col  = (int)lines.getNode(lastLine)->text.size();
    } else {
        // CASE 3: the range is the WHOLE document.
        // Empty it but keep one empty line.
        if (isEmpty()) {
            throw EditorError("Nothing to delete");   // a == b would make getText fail
        }
        a.line = 0;            a.col = 0;
        b.line = lastLine;     b.col = (int)lines.getNode(lastLine)->text.size();
    }

    deleteRange(a, b);         // one note on the undo pile
}


// ---------- 6D. UNDO / REDO ----------  OWNER: MEMBER A
// Undo of Insert = delete that text. Undo of Delete = insert that text back.
// Notes with the same 'group' number are undone / redone together.
// saveNote = false everywhere, so undo/redo do not create new notes.
 
void Document::beginGroup() {
    if (activeGroup != -1) {
        throw EditorError("A group is already open");
    }
    activeGroup = nextGroupId;
    nextGroupId++;
}
 
void Document::endGroup() {
    if (activeGroup == -1) {
        throw EditorError("No group is open");
    }
    activeGroup = -1;
}
 
void Document::undo() {
    if (activeGroup != -1) {
        throw EditorError("Close the group before undo");
    }
    if (undoStack.isEmpty()) {
        throw EditorError("Nothing to undo");
    }
 
    int g = undoStack.top().group;
 
    // Newest note first = correct order for undoing
    while (!undoStack.isEmpty() && undoStack.top().group == g) {
        Operation op = undoStack.pop();
 
        if (op.type == OpType::Insert) {
            deleteRange(op.start, endOf(op.start, op.text), false);
        } else {
            insertAt(op.start, op.text, false);
        }
 
        redoStack.push(op);
    }
}
 
void Document::redo() {
    if (activeGroup != -1) {
        throw EditorError("Close the group before redo");
    }
    if (redoStack.isEmpty()) {
        throw EditorError("Nothing to redo");
    }
 
    int g = redoStack.top().group;
 
    // Oldest note is on top, so notes are replayed in the original order
    while (!redoStack.isEmpty() && redoStack.top().group == g) {
        Operation op = redoStack.pop();
 
        if (op.type == OpType::Insert) {
            insertAt(op.start, op.text, false);
        } else {
            deleteRange(op.start, endOf(op.start, op.text), false);
        }
 
        undoStack.push(op);      // NOT record(): it would clear the rest of redoStack
    }
}
 

// ---------- 6E. SEARCH / REPLACE ----------  OWNER: MEMBER B
// Find every copy of 'pattern' in the whole document and add each hit to 'found'.
// Works like the Find box of a real editor:
//   - every line is scanned ONCE (KMP), so it stays fast on long documents
//   - hits never overlap: after a hit, the next search starts AFTER it ("aa" in "aaaa" = 2 hits)
//   - hits come in reading order (top to bottom, left to right)
// Line and column in each Match count from 0 (the menu adds 1 when it prints).
// Throws EditorError if the pattern is empty.
// Time: O(m) once for the pattern + O(N) for the scan (N = all letters in the document).
void Document::findAll(const string& pattern, bool ignoreCase, bool wholeWord,
                       DynArray<Match>& found) const {
    if (pattern.empty()) {
        throw EditorError("Search pattern is empty");
    }
    int m = (int)pattern.size();

    // 1. make the pattern small letters ONCE (the text is never copied or changed)
    string p = pattern;
    if (ignoreCase) {
        for (int i = 0; i < m; i++) {
            p[i] = toLowerChar(p[i]);
        }
    }

    // 2. build the KMP table ONCE for the whole search:
    //    lps[i] = how many letters at the END of p[0..i] are also the START of p
    //    (it tells us how far to fall back after a mismatch, so we never re-read the text)
    DynArray<int> lps;
    lps.add(0);
    int len = 0;
    for (int i = 1; i < m; i++) {
        while (len > 0 && p[i] != p[len]) {
            len = lps.get(len - 1);
        }
        if (p[i] == p[len]) {
            len++;
        }
        lps.add(len);
    }

    // 3. scan every line once
    int lineNo = 0;                                        // 0 = first line
    for (Line* card = lines.first(); card != nullptr; card = card->next) {
        const string& text = card->text;
        int n = (int)text.size();

        if (n >= m) {                                      // a shorter line cannot contain it
            int j = 0;                                     // how many letters of p match right now
            for (int i = 0; i < n; i++) {
                char c = text[i];
                if (ignoreCase) {
                    c = toLowerChar(c);
                }
                while (j > 0 && c != p[j]) {
                    j = lps.get(j - 1);                    // mismatch: fall back, do NOT go back in the text
                }
                if (c == p[j]) {
                    j++;
                }

                if (j == m) {                              // all m letters matched, ending at i
                    int start = i - m + 1;
                    bool ok = true;
                    if (wholeWord) {                       // letters just before/after must not be word letters
                        bool leftOk  = (start == 0) || !isWordChar(text[start - 1]);
                        bool rightOk = (i + 1 == n) || !isWordChar(text[i + 1]);
                        ok = leftOk && rightOk;
                    }
                    if (ok) {
                        Match hit;
                        hit.line = lineNo;
                        hit.col  = start;
                        found.add(hit);
                        j = 0;                             // next hit must start AFTER this one
                    } else {
                        j = lps.get(m - 1);                // rejected: keep looking, overlap allowed
                    }
                }
            }
        }
        lineNo++;
    }
}
void Document::replaceOne(const string& pattern, const string& replacement, int k,
                          bool ignoreCase, bool wholeWord) {
    (void)pattern; (void)replacement; (void)k; (void)ignoreCase; (void)wholeWord;
    throw EditorError("TODO");
}
int Document::replaceAll(const string& pattern, const string& replacement,
                         bool ignoreCase, bool wholeWord) {
    (void)pattern; (void)replacement; (void)ignoreCase; (void)wholeWord;
    throw EditorError("TODO");
}


// ---------- 6F. CUT / COPY / PASTE ----------  OWNER: STARTER (done)
// The copied text lives in the private string 'clipboard' (added in Section 4).
// Copy never changes the document, so it never touches 'dirty' or the undo pile.

// Copy the text from a up to (not including) b.
// getText checks both spots and that a comes before b, so bad input throws before clipboard changes.
void Document::copyRange(Position a, Position b) {
    clipboard = getText(a, b);
}

// Copy whole lines firstLine..lastLine (0-based). Lines are joined with '\n',
// with NO '\n' after the last one.
void Document::copyLines(int firstLine, int lastLine) {
    if (firstLine < 0 || lastLine >= lines.size() || firstLine > lastLine) {
        throw EditorError("Line range out of range");
    }
    string result = "";
    Line* card = lines.getNode(firstLine);
    for (int i = firstLine; i <= lastLine; i++) {
        if (i > firstLine) {
            result += "\n";
        }
        result += card->text;
        card = card->next;
    }
    clipboard = result;
}

// Cut = delete, then keep what was deleted.
// deleteRange returns the removed text and throws BEFORE changing anything if the spots are bad.
void Document::cutRange(Position a, Position b) {
    clipboard = deleteRange(a, b);
}

// Cut whole lines = copyLines, then deleteLines.
// If deleteLines fails (for example an empty document), put the old clipboard back.
void Document::cutLines(int firstLine, int lastLine) {
    string oldClipboard = clipboard;
    copyLines(firstLine, lastLine);
    try {
        deleteLines(firstLine, lastLine);
    } catch (EditorError&) {
        clipboard = oldClipboard;
        throw;
    }
}

// Insert the clipboard text at a spot.
void Document::pasteAt(Position pos) {
    if (clipboard.empty()) {
        throw EditorError("Clipboard is empty");
    }
    insertAt(pos, clipboard);
}

// Insert the clipboard as whole new line(s) ABOVE line number 'line'.
// Text + '\n' put at column 0 pushes the old line down.
void Document::pasteLinesBefore(int line) {
    if (clipboard.empty()) {
        throw EditorError("Clipboard is empty");
    }
    Position startOfLine;
    startOfLine.line = line;
    startOfLine.col  = 0;
    insertAt(startOfLine, clipboard + "\n");
}

// ===== SECTION 7: menu and main() =====  (OWNER: MEMBER C)

// ---------- 7A. menu helpers ----------

// Build a Position from 0-based numbers.
Position makePos(int line, int col) {
    Position p;
    p.line = line;
    p.col = col;
    return p;
}

// Ask for text. Quits cleanly if the input stream is closed.
string readString(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) {
        cout << "\nInput closed. Goodbye.\n";
        exit(0);
    }
    return s;
}

// Ask for a whole number. Keeps asking until the user types a valid one.
int readInt(const string& prompt) {
    while (true) {
        string s = readString(prompt);
        try {
            size_t used = 0;
            int value = stoi(s, &used);
            if (used == s.size()) {
                return value;
            }
        } catch (...) {
            // fall through to the message below
        }
        cout << "Please type a whole number.\n";
    }
}

// Ask a yes/no question.
bool askYesNo(const string& prompt) {
    while (true) {
        string s = readString(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        cout << "Please type y or n.\n";
    }
}

// The user types from 1, the code counts from 0: convert HERE and only here.
int readLineNumber(const string& prompt) {
    return readInt(prompt) - 1;
}

Position readPosition(const string& what) {
    cout << what << "\n";
    int line = readLineNumber("  Line number: ");
    int col  = readInt("  Column number (1 = before the first letter): ") - 1;
    return makePos(line, col);
}

void printMenu() {
    cout << "\n===== TEXT EDITOR =====\n"
         << " 1. New document           2. Open file\n"
         << " 3. Save                   4. Save as\n"
         << " 5. Insert text            6. Press Enter (split line)\n"
         << " 7. Blank line after       8. Blank line before\n"
         << " 9. Delete character      10. Delete word\n"
         << "11. Delete lines          12. Undo\n"
         << "13. Redo                  14. Find\n"
         << "15. Replace one           16. Replace all\n"
         << "17. Copy range            18. Copy lines\n"
         << "19. Cut range             20. Cut lines\n"
         << "21. Paste at spot         22. Paste lines before\n"
         << "23. Show document         24. Run tests\n"
         << " 0. Exit\n";
}

// Ask whether to save before leaving. Returns false if the user cancels.
bool confirmLeaveUnsaved(Document& doc) {
    if (!doc.isDirty()) return true;
    if (askYesNo("You have unsaved changes. Save first?")) {
        if (doc.hasFilename()) {
            doc.saveFile();
        } else {
            doc.saveFileAs(readString("File name: "));
        }
    }
    return true;
}

// ---------- 7B. tests ----------

int testsPassed = 0;
int testsFailed = 0;

void check(const string& name, bool ok) {
    if (ok) { testsPassed++; cout << "  [PASS] " << name << "\n"; }
    else    { testsFailed++; cout << "  [FAIL] " << name << "\n"; }
}

// Used for "this should throw EditorError" tests: pass the result of a try/catch.
void checkThrows(const string& name, bool threw) {
    check(name + " (should throw)", threw);
}

void runTests() {
    testsPassed = 0;
    testsFailed = 0;

    cout << "\n-- Section 0 helpers --\n";
    check("isBefore same line", isBefore(makePos(0, 1), makePos(0, 2)));
    check("isBefore different line", isBefore(makePos(0, 9), makePos(1, 0)));
    check("isBefore equal is false", !isBefore(makePos(1, 1), makePos(1, 1)));
    check("isSame", isSame(makePos(2, 3), makePos(2, 3)));
    check("isWordChar letter/digit/_", isWordChar('a') && isWordChar('Z') && isWordChar('5') && isWordChar('_'));
    check("isWordChar space is false", !isWordChar(' '));
    check("toLowerChar", toLowerChar('A') == 'a' && toLowerChar('b') == 'b' && toLowerChar('1') == '1');
    Position e1 = endOf(makePos(0, 3), "abc");
    check("endOf no line break", e1.line == 0 && e1.col == 6);
    Position e2 = endOf(makePos(0, 3), "ab\ncde");
    check("endOf with line break", e2.line == 1 && e2.col == 3);

    cout << "\n-- LineList --\n";
    {
        LineList list;
        list.append("a");
        list.append("c");
        list.insertAfter(list.getNode(0), "b");
        check("size after inserts", list.size() == 3);
        check("order a,b,c", list.getNode(0)->text == "a" && list.getNode(1)->text == "b" && list.getNode(2)->text == "c");
        list.checkIntegrity();
        list.removeNode(list.getNode(1));
        check("size after remove", list.size() == 2);
        list.checkIntegrity();
        bool threw = false;
        try { list.getNode(5); } catch (EditorError&) { threw = true; }
        checkThrows("getNode out of range", threw);
    }

    cout << "\n-- Stack --\n";
    {
        Stack<int> s;
        s.push(1);
        s.push(2);
        check("top is last pushed", s.top() == 2);
        check("pop gives last pushed", s.pop() == 2);
        check("size after pop", s.size() == 1);
        s.pop();
        check("empty after popping all", s.isEmpty());
        bool threw = false;
        try { s.pop(); } catch (EditorError&) { threw = true; }
        checkThrows("pop on empty stack", threw);
    }

    cout << "\n-- DynArray --\n";
    {
        DynArray<int> arr;
        for (int i = 0; i < 20; i++) arr.add(i * 2);   // forces several growths
        check("size after 20 adds", arr.size() == 20);
        check("values kept after growing", arr.get(0) == 0 && arr.get(19) == 38);
        bool threw = false;
        try { arr.get(20); } catch (EditorError&) { threw = true; }
        checkThrows("get out of range", threw);
    }

    cout << "\n-- Document: insert --\n";
    {
        Document d;
        check("new document is empty", d.isEmpty() && d.lineCount() == 1);
        check("new document is not dirty", !d.isDirty());
        d.insertText(makePos(0, 0), "hello world");
        check("insert text", d.getText(makePos(0, 0), makePos(0, 11)) == "hello world");
        check("charCount", d.charCount() == 11);
        check("dirty after insert", d.isDirty());
        d.insertText(makePos(0, 5), "\nabc");
        check("insert with line break adds a line", d.lineCount() == 2);
        check("text across lines", d.getText(makePos(0, 0), makePos(1, 3)) == "hello\nabc");
        d.insertNewLine(makePos(0, 2));
        check("insertNewLine splits the line", d.lineCount() == 3);
        check("render of first line", d.renderLines(0, 0) == "1 | he\n");
        d.insertBlankLineAfter(0);
        check("blank line after", d.lineCount() == 4 && d.renderLines(1, 1) == "2 | \n");
        d.insertBlankLineBefore(0);
        check("blank line before", d.lineCount() == 5);
    }

    cout << "\n-- Document: delete --\n";
    {
        Document d;
        d.insertText(makePos(0, 0), "hello world");
        string removed = d.deleteRange(makePos(0, 5), makePos(0, 11));
        check("deleteRange returns removed text", removed == " world");
        check("text after deleteRange", d.getText(makePos(0, 0), makePos(0, 5)) == "hello");

        Document m;
        m.insertText(makePos(0, 0), "one\ntwo\nthree");
        m.deleteLines(1, 1);
        check("deleteLines middle", m.lineCount() == 2 && m.renderLines(1, 1) == "2 | three\n");
        m.deleteLines(1, 1);
        check("deleteLines last line", m.lineCount() == 1);
        m.deleteLines(0, 0);
        check("deleteLines whole document keeps 1 line", m.lineCount() == 1 && m.isEmpty());
        bool threw = false;
        try { m.deleteLines(0, 0); } catch (EditorError&) { threw = true; }
        checkThrows("deleteLines on empty document", threw);
    }

    cout << "\n-- Document: clipboard --\n";
    {
        Document d;
        d.insertText(makePos(0, 0), "abc");
        d.copyRange(makePos(0, 0), makePos(0, 2));
        d.pasteAt(makePos(0, 3));
        check("copy then paste", d.getText(makePos(0, 0), makePos(0, 5)) == "abcab");

        Document c;
        c.insertText(makePos(0, 0), "one\ntwo");
        c.cutLines(0, 0);
        check("cutLines removes the line", c.lineCount() == 1 && c.renderLines(0, 0) == "1 | two\n");
        c.pasteLinesBefore(0);
        check("pasteLinesBefore puts it back", c.lineCount() == 2 && c.renderLines(0, 0) == "1 | one\n");

        Document n;
        bool threw = false;
        try { n.pasteAt(makePos(0, 0)); } catch (EditorError&) { threw = true; }
        checkThrows("paste with empty clipboard", threw);
    }

    cout << "\n-- Document: error handling --\n";
    {
        Document d;
        d.insertText(makePos(0, 0), "abc");
        bool t1 = false, t2 = false, t3 = false, t4 = false;
        try { d.insertText(makePos(0, 0), ""); } catch (EditorError&) { t1 = true; }
        checkThrows("insert empty text", t1);
        try { d.insertText(makePos(9, 0), "x"); } catch (EditorError&) { t2 = true; }
        checkThrows("insert on a line that does not exist", t2);
        try { d.insertText(makePos(0, 99), "x"); } catch (EditorError&) { t3 = true; }
        checkThrows("insert at a column that does not exist", t3);
        try { d.getText(makePos(0, 2), makePos(0, 1)); } catch (EditorError&) { t4 = true; }
        checkThrows("getText with start after end", t4);
        check("document unchanged after failed edits", d.getText(makePos(0, 0), makePos(0, 3)) == "abc");
    }

    // TODO (after Members A and B finish): add tests for
    // undo/redo, openFile/saveFile, deleteChar, deleteWord, findAll, replaceOne, replaceAll.

    cout << "\nResult: " << testsPassed << " passed, " << testsFailed << " failed.\n";
}

// ---------- 7C. the menu ----------

void runMenu() {
    Document doc;
    while (true) {
        printMenu();
        int choice = readInt("Choose an option: ");
        try {
            switch (choice) {
            case 0:
                if (confirmLeaveUnsaved(doc)) {
                    cout << "Goodbye.\n";
                    return;
                }
                break;
            case 1:
                if (confirmLeaveUnsaved(doc)) {
                    doc.newDocument();
                    cout << "New document created.\n";
                }
                break;
            case 2: {
                if (confirmLeaveUnsaved(doc)) {
                    doc.openFile(readString("File name: "));
                    cout << "File opened.\n";
                }
                break;
            }
            case 3:
                if (doc.hasFilename()) {
                    doc.saveFile();
                } else {
                    doc.saveFileAs(readString("File name: "));
                }
                cout << "Saved.\n";
                break;
            case 4:
                doc.saveFileAs(readString("File name: "));
                cout << "Saved.\n";
                break;
            case 5: {
                Position p = readPosition("Where to insert?");
                doc.insertText(p, readString("Text: "));
                break;
            }
            case 6:
                doc.insertNewLine(readPosition("Where to split the line?"));
                break;
            case 7:
                doc.insertBlankLineAfter(readLineNumber("Insert a blank line after line number: "));
                break;
            case 8:
                doc.insertBlankLineBefore(readLineNumber("Insert a blank line before line number: "));
                break;
            case 9:
                doc.deleteChar(readPosition("Delete the character at:"));
                break;
            case 10:
                doc.deleteWord(readPosition("Delete the word at:"));
                break;
            case 11: {
                int first = readLineNumber("First line to delete: ");
                int last  = readLineNumber("Last line to delete: ");
                doc.deleteLines(first, last);
                break;
            }
            case 12:
                doc.undo();
                break;
            case 13:
                doc.redo();
                break;
            case 14: {
                string pattern = readString("Search for: ");
                bool ic = askYesNo("Ignore upper/lower case?");
                bool ww = askYesNo("Whole words only?");
                DynArray<Match> found;
                doc.findAll(pattern, ic, ww, found);
                cout << found.size() << " match(es).\n";
                for (int i = 0; i < found.size(); i++) {
                    cout << "  #" << (i + 1) << ": line " << (found.get(i).line + 1)
                         << ", column " << (found.get(i).col + 1) << "\n";
                }
                break;
            }
            case 15: {
                string pattern = readString("Search for: ");
                string repl = readString("Replace with: ");
                int k = readInt("Which match number (1 = first)? ") - 1;
                bool ic = askYesNo("Ignore upper/lower case?");
                bool ww = askYesNo("Whole words only?");
                doc.replaceOne(pattern, repl, k, ic, ww);
                break;
            }
            case 16: {
                string pattern = readString("Search for: ");
                string repl = readString("Replace with: ");
                bool ic = askYesNo("Ignore upper/lower case?");
                bool ww = askYesNo("Whole words only?");
                int n = doc.replaceAll(pattern, repl, ic, ww);
                cout << n << " replacement(s) made.\n";
                break;
            }
            case 17: {
                Position a = readPosition("Copy FROM:");
                Position b = readPosition("Copy UP TO (not included):");
                doc.copyRange(a, b);
                break;
            }
            case 18: {
                int first = readLineNumber("First line to copy: ");
                int last  = readLineNumber("Last line to copy: ");
                doc.copyLines(first, last);
                break;
            }
            case 19: {
                Position a = readPosition("Cut FROM:");
                Position b = readPosition("Cut UP TO (not included):");
                doc.cutRange(a, b);
                break;
            }
            case 20: {
                int first = readLineNumber("First line to cut: ");
                int last  = readLineNumber("Last line to cut: ");
                doc.cutLines(first, last);
                break;
            }
            case 21:
                doc.pasteAt(readPosition("Paste at:"));
                break;
            case 22:
                doc.pasteLinesBefore(readLineNumber("Paste above line number: "));
                break;
            case 23:
                cout << "\n" << doc.render();
                break;
            case 24:
                runTests();
                break;
            default:
                cout << "Unknown option. Choose a number from the menu.\n";
            }
        } catch (EditorError& e) {
            cout << "Error: " << e.what() << "\n";    // every editor problem ends up here
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--test") {
        runTests();
        return testsFailed == 0 ? 0 : 1;
    }
    runMenu();
    return 0;

// ---------------------------------------------------------------
// Helper: read an integer safely
// ---------------------------------------------------------------
int readInt(const string &prompt)
{
    string input;
    while (true)
    {
        cout << prompt;
        getline(cin, input);
        try
        {
            return stoi(input);
        }
        catch (...)
        {
            cout << "Please type a number.\n";
        }
    }
}

static void menuDeleteWord(Document& doc) {
    try {
        int line = readInt("Line number: ") - 1;          // the user counts from 1
        string word;
        cout << "Word to delete: ";
        getline(cin, word);

        int which = 1;
        int total = doc.countWordOnLine(line, word);      // throws if the line or word is bad
        if (total == 0) {
            throw EditorError("Word not found on that line");
        }
        if (total > 1) {
            cout << "\"" << word << "\" appears " << total << " times on this line.\n";
            which = readInt("Delete which one? (1 = leftmost): ");
        }
        doc.deleteWordOnLine(line, word, which);
        cout << "Word deleted.\n";
    } catch (EditorError& e) {
        cout << "Error: " << e.what() << ".\n";
    }
}

static void menuDeleteChar(Document& doc) {
    try {
        int line = readInt("Line number: ") - 1;
        string word;
        cout << "Word on that line: ";
        getline(cin, word);
        string c;
        cout << "Character to remove: ";
        getline(cin, c);
        if (c.size() != 1) {
            throw EditorError("Type exactly one character");
        }

        int which = 1;
        int total = doc.countWordOnLine(line, word);
        if (total == 0) {
            throw EditorError("Word not found on that line");
        }
        if (total > 1) {
            cout << "\"" << word << "\" appears " << total << " times on this line.\n";
            which = readInt("Which one? (1 = leftmost): ");
        }
        doc.deleteCharInWord(line, word, c[0], which);
        cout << "Character removed.\n";
    } catch (EditorError& e) {
        cout << "Error: " << e.what() << ".\n";
    }
}