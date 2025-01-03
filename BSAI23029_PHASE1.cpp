#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
using namespace std;

class Node {
public:
    string name;
    bool isDirectory;
    string content;
    Node* firstChild;
    Node* nextSibling;
    Node* parent;

    time_t createdAt;
    time_t modifiedAt;

    Node(string name, bool isDirectory)
        : name(name), isDirectory(isDirectory), content(""), firstChild(nullptr), nextSibling(nullptr), parent(nullptr) {
        createdAt = time(nullptr);
        modifiedAt = time(nullptr);
    }
};

class FileSystemException : public exception {
    string message;
public:
    FileSystemException(string msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class FileSystem {
private:
    Node* root;
    Node* current;

    Node* findChild(Node* parent, const string& name) {
        Node* child = parent->firstChild;
        while (child) {
            if (child->name == name) return child;
            child = child->nextSibling;
        }
        return nullptr;
    }

    void printPath(Node* node) {
        if (!node->parent) {
            cout << "/";
            return;
        }
        printPath(node->parent);
        cout << node->name << "/";
    }

    void serializeNode(Node* node, ofstream& out) {
        if (!node) return;
        out << node->name << "|" << node->isDirectory << "|" << node->content << "|" << node->createdAt << "|" << node->modifiedAt << endl;
        serializeNode(node->firstChild, out);
        serializeNode(node->nextSibling, out);
    }

    Node* deserializeNode(ifstream& in, Node* parent) {
        string line;
        if (!getline(in, line) || line.empty()) return nullptr;

        stringstream ss(line);
        string name, isDirStr, content;
        time_t createdAt, modifiedAt;
        getline(ss, name, '|');
        getline(ss, isDirStr, '|');
        getline(ss, content, '|');
        ss >> createdAt >> modifiedAt;

        Node* node = new Node(name, isDirStr == "1");
        node->content = content;
        node->createdAt = createdAt;
        node->modifiedAt = modifiedAt;
        node->parent = parent;
        node->firstChild = deserializeNode(in, node);
        node->nextSibling = deserializeNode(in, parent);

        return node;
    }

    void executeCommand(const string& command) {
        stringstream ss(command);
        string cmd;
        ss >> cmd;

        if (cmd == "mkdir") {
            string name;
            ss >> name;
            mkdir(name);
        }
        else if (cmd == "touch") {
            string name, content;
            ss >> name;
            getline(ss, content);
            touch(name, content);
        }
        else if (cmd == "cd") {
            string name;
            ss >> name;
            cd(name);
        }
        else if (cmd == "pwd") {
            pwd();
        }
        else if (cmd == "ls") {
            ls();
        }
        else if (cmd == "cat") {
            string name;
            ss >> name;
            cat(name);
        }
        else if (cmd == "exit") {
            exit(0);
        }
        else {
            cout << "Unknown command: " << cmd << endl;
        }
    }

public:
    FileSystem() {
        root = new Node("/", true);
        current = root;
    }

    ~FileSystem() { deleteTree(root); }

    void deleteTree(Node* node) {
        if (!node) return;
        deleteTree(node->firstChild);
        deleteTree(node->nextSibling);
        delete node;
    }

    void mkdir(string name) {
        if (findChild(current, name)) {
            cout << "Directory already exists." << endl;
            return;
        }
        Node* newDir = new Node(name, true);
        newDir->parent = current;
        newDir->nextSibling = current->firstChild;
        current->firstChild = newDir;
    }

    void touch(string name, string content = "") {
        if (findChild(current, name)) {
            cout << "File already exists." << endl;
            return;
        }
        Node* newFile = new Node(name, false);
        newFile->content = content;
        newFile->parent = current;
        newFile->nextSibling = current->firstChild;
        current->firstChild = newFile;
    }

    void cd(string name) {
        if (name == "..") {
            if (current->parent) current = current->parent;
            return;
        }
        Node* target = findChild(current, name);
        if (!target || !target->isDirectory) {
            cout << "Directory not found." << endl;
            return;
        }
        current = target;
    }

    void pwd() {
        printPath(current);
        cout << endl;
    }

    void ls() {
        Node* child = current->firstChild;
        while (child) {
            cout << child->name;
            if (child->isDirectory) cout << "/";
            cout << " ";
            child = child->nextSibling;
        }
        cout << endl;
    }

    void cat(string name) {
        Node* file = findChild(current, name);
        if (!file || file->isDirectory) {
            cout << "File not found." << endl;
            return;
        }
        cout << file->content << endl;
    }

    void saveToFile(string filename) {
        ofstream out(filename);
        if (!out) {
            cout << "Error: Could not save to file." << endl;
            return;
        }
        serializeNode(root, out);
        out.close();
    }

    void loadFromFile(string filename) {
        ifstream in(filename);
        if (!in) {
            cout << "Error: Could not load from file." << endl;
            return;
        }
        deleteTree(root);
        root = deserializeNode(in, nullptr);
        current = root;
        in.close();
    }

    void startCLI() {
        while (true) {
            cout << "Current directory: ";
            printPath(current);
            cout << "> ";
            string command;
            getline(cin, command);
            executeCommand(command);
        }
    }
};

int main() {
    FileSystem fs;
    fs.mkdir("home");
    fs.cd("home");
    fs.mkdir("user");
    fs.cd("user");
    fs.touch("notes.txt", "Hello World!");
    fs.ls(); // Should show: notes.txt
    fs.cat("notes.txt"); // Should print: Hello World!
    fs.pwd(); // Should print: /home/user/
    return 0;
}
