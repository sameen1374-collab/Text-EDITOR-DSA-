/ ---------------------------------------------------------------
// // Helper: read an integer safely
// // ---------------------------------------------------------------
// int readInt(const string &prompt)
// {
//     string input;
//     while (true)
//     {
//         cout << prompt;
//         getline(cin, input);
//         try
//         {
//             return stoi(input);
//         }
//         catch (...)
//         {
//             cout << "Please type a number.\n";
//         }
//     }
// }

// static void menuDeleteWord(Document& doc) {
//     try {
//         int line = readInt("Line number: ") - 1;          // the user counts from 1
//         string word;
//         cout << "Word to delete: ";
//         getline(cin, word);

//         int which = 1;
//         int total = doc.countWordOnLine(line, word);      // throws if the line or word is bad
//         if (total == 0) {
//             throw EditorError("Word not found on that line");
//         }
//         if (total > 1) {
//             cout << "\"" << word << "\" appears " << total << " times on this line.\n";
//             which = readInt("Delete which one? (1 = leftmost): ");
//         }
//         doc.deleteWordOnLine(line, word, which);
//         cout << "Word deleted.\n";
//     } catch (EditorError& e) {
//         cout << "Error: " << e.what() << ".\n";
//     }
// }

// static void menuDeleteChar(Document& doc) {
//     try {
//         int line = readInt("Line number: ") - 1;
//         string word;
//         cout << "Word on that line: ";
//         getline(cin, word);
//         string c;
//         cout << "Character to remove: ";
//         getline(cin, c);
//         if (c.size() != 1) {
//             throw EditorError("Type exactly one character");
//         }

//         int which = 1;
//         int total = doc.countWordOnLine(line, word);
//         if (total == 0) {
//             throw EditorError("Word not found on that line");
//         }
//         if (total > 1) {
//             cout << "\"" << word << "\" appears " << total << " times on this line.\n";
//             which = readInt("Which one? (1 = leftmost): ");
//         }
//         doc.deleteCharInWord(line, word, c[0], which);
//         cout << "Character removed.\n";
//     } catch (EditorError& e) {
//         cout << "Error: " << e.what() << ".\n";
//     }
// }