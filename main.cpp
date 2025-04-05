// main.cpp
#include <iostream>
#include<cstring>
#include<vector>
#include <fstream>
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Security
string encryptText(const string& text, int shift = 3) {
    string result = text;
    for (char& c : result) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            c = (c - base + shift) % 26 + base;
        }
    }
    return result;
}

string decryptText(const string& text, int shift = 3) {
    return encryptText(text, 26 - shift); // reverse the shift
}

// Sanitize filename: replaces space with _
string sanitizeFileName(const string& name) {
    string result = name;
    std::replace(result.begin(), result.end(), ' ', '_');  // replace space with underscore
    return result;
}

void createNote() {
    clearScreen();
    string title, content;

    cout << "Enter note title (or type 2 to CANCEL): ";
    cin.ignore();
    getline(cin, title);
    if (title == "2") return;

    string fileName = sanitizeFileName(title) + ".txt";
    string filePath = "notes/" + fileName;

    ofstream file(filePath);
    if (!file) {
        cout << "Error creating file.\n";
        return;
    }

    int isPrivate;
    cout << "Make this note private?\n";
    cout << "1. Yes\n2. No\n3. EXIT\nChoose: ";
    cin >> isPrivate;
    cin.ignore();

    if (isPrivate == 3) {
        file.close();
        fs::remove(filePath); // cleanup
        return;
    }

    string password;
    if (isPrivate == 1) {
        cout << "Enter password for this note: ";
        getline(cin, password);
        file << "PRIVATE:" << encryptText(password) << "\n";
    } else if (isPrivate == 2) {
        file << "PUBLIC\n";
    } else {
        cout << "Invalid input. Note creation cancelled.\n";
        file.close();
        fs::remove(filePath);
        return;
    }

    cout << "Enter note content (end with ~ on a new line or type 2 to cancel):\n";
    string line, fullText;
    while (true) {
        getline(cin, line);
        if (line == "2") {
            file.close();
            fs::remove(filePath);
            return;
        }
        if (line == "~") break;
        fullText += line + "\n";
    }

    string encrypted = encryptText(fullText);
    file << encrypted;
    file.close();

    cout << "Note saved successfully.\n";
}

void listNotes() {
    clearScreen();
    cout << "\nSaved Notes:\n";
    for (const auto& entry : fs::directory_iterator("notes")) {
        cout << "- " << entry.path().filename().string() << "\n";
    }
}

void readNote() {
    clearScreen();
    vector<string> files;
    int count = 1;

    // List all notes
    for (const auto& entry : fs::directory_iterator("notes")) {
        string filename = entry.path().filename().string();
        files.push_back(filename);
        cout << count++ << ". " << filename << "\n";
    }

    if (files.empty()) {
        cout << "No notes found.\n";
        return;
    }

    int choice;
    cout << "Enter number to read note: ";
    cin >> choice;

    if (choice < 1 || choice > files.size()) {
        cout << "Invalid number!\n";
        return;
    }

    string filepath = "notes/" + files[choice - 1];
    ifstream file(filepath);
    if (!file) {
        cout << "Could not open file.\n";
        return;
    }

    string header;
    getline(file, header); // read first line (header)
    
    string content, line;
    while (getline(file, line)) {
        content += line + "\n";
    }
    file.close();

    if (header.rfind("PRIVATE:", 0) == 0) {
        string encryptedPassword = header.substr(8);
        string enteredPassword;
        cout << "This note is private. Enter password: ";
        cin.ignore();
        getline(cin, enteredPassword);
        
        if (encryptText(enteredPassword) == encryptedPassword) {
            cout << "\n--- Note Content ---\n";
            cout << decryptText(content);
        } else {
            cout << "Incorrect password. Access denied.\n";
        }
    } else if (header == "PUBLIC") {
        cout << "\n--- Note Content ---\n";
        cout << decryptText(content);
    } else {
        cout << "Invalid note format.\n";
    }
}

void deleteNote() {
    clearScreen();
    vector<string> files;
    int count = 1;

    for (const auto& entry : fs::directory_iterator("notes")) {
        string filename = entry.path().filename().string();
        files.push_back(filename);
        cout << count++ << ". " << filename << "\n";
    }

    if (files.empty()) {
        cout << "No notes to delete.\n";
        return;
    }

    int choice;
    cout << "Enter number to delete note: ";
    cin >> choice;

    if (choice < 1 || choice > files.size()) {
        cout << "Invalid choice!\n";
        return;
    }

    string filepath = "notes/" + files[choice - 1];
    ifstream file(filepath);
    if (!file) {
        cout << "Could not open file.\n";
        return;
    }

    string firstLine;
    getline(file, firstLine);
    file.close();

    if (firstLine.rfind("PRIVATE:", 0) == 0) {
        string encryptedPassword = firstLine.substr(8);
        string inputPassword;
        cout << "This is a private note. Enter password to delete: ";
        cin.ignore();
        getline(cin, inputPassword);

        if (encryptText(inputPassword) != encryptedPassword) {
            cout << "Incorrect password. Deletion aborted.\n";
            return;
        }
    }

    if (fs::remove(filepath)) {
        cout << "Note deleted successfully.\n";
    } else {
        cout << "Failed to delete note.\n";
    }
}

void searchNotes() {
    clearScreen();
    string keyword;
    cout << "Enter keyword to search: ";
    cin.ignore();
    getline(cin, keyword);

    bool found = false;
    for (const auto& entry : fs::directory_iterator("notes")) {
        string filename = entry.path().filename().string();
        ifstream file(entry.path());
        if (!file) continue;

        string header;
        getline(file, header); // First line contains PRIVATE or PUBLIC

        // Skip private notes
        if (header.rfind("PRIVATE:", 0) == 0) {
            file.close();
            continue;
        }

        string line, content;
        while (getline(file, line)) {
            content += line + "\n";
        }
        file.close();

        // Decrypt content
        string decrypted = decryptText(content);

        // Check for match
        if (filename.find(keyword) != string::npos) {
            cout << "[Title Match] " << filename << "\n";
            found = true;
        } else if (decrypted.find(keyword) != string::npos) {
            cout << "[Content Match] " << filename << "\n";
            found = true;
        }
    }

    if (!found) {
        cout << "No matching public notes found.\n";
    }
}

int main() {
    fs::create_directory("notes");
    clearScreen();
    int choice;
    do {
        cout << "\n--- Notes App ---\n";
        cout << "1. Create Note\n2. List Notes\n3. Read Note\n4. Delete Note\n5. Search Notes\n6. Exit\nChoose: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();              // clear error flags
            cin.ignore(10000, '\n');  // discard invalid input
            cout << "Invalid input! Please enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1: createNote(); break;
            case 2: listNotes(); break;
            case 3: readNote(); break;
            case 4: deleteNote(); break;
            case 5: searchNotes(); break;
            case 6: cout << "Goodbye!\n"; break;
            default: cout << "Invalid option.\n";
        }
        

    } while (choice != 6);

    return 0;
}

