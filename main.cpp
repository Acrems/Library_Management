#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <map>
#include <iomanip>
#include <limits>

using namespace std;

// Prompts until the user enters a valid integer, clearing bad input each attempt.
int getValidIntegerInput(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Please enter an integer.\n";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
    }
}

// Converts a time_t timestamp to "dd/mm/yyyy HH:MM:SS" format.
string formatTimestamp(time_t timestamp) {
    char buffer[20];
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeinfo);
    return string(buffer);
}

// Parses a "dd/mm/yyyy HH:MM:SS" string back into a time_t timestamp.
time_t parseTimestamp(const string& datetime) {
    struct tm timeinfo = {};
    sscanf(datetime.c_str(), "%d/%d/%d %d:%d:%d",
           &timeinfo.tm_mday, &timeinfo.tm_mon, &timeinfo.tm_year,
           &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
    timeinfo.tm_mon  -= 1;    // tm_mon is 0-based (January = 0)
    timeinfo.tm_year -= 1900; // tm_year counts years since 1900
    return mktime(&timeinfo);
}

// ─────────────────────────────────────────────
// Book
// ─────────────────────────────────────────────

class Book {
public:
    string genre, title, author, publisher, isbn;
    int    year;
    string status;

    Book(string g, string t, string a, string p, int y, string i, string s = "Available")
        : genre(g), title(t), author(a), publisher(p), year(y), isbn(i), status(s) {}

    string toCSV() const {
        return genre + "," + title + "," + author + "," + publisher + "," +
               to_string(year) + "," + isbn + "," + status + "\n";
    }
};

// ─────────────────────────────────────────────
// Account
// ─────────────────────────────────────────────

class Account {
private:
    string userType;

    //   [0] isbn        [1] title       [2] borrowedDate
    //   [3] returnedDate ("Not Returned" until returned)
    //   [4] dueDate     [5] status ("Not Returned" | "Returned" | "Overdue")
    //   [6] fine in rupees (students only; 0 while not overdue)
    vector<tuple<string, string, string, string, string, string, int>> borrowedBooks;

    int fines;

public:
    Account(string type) : userType(type), fines(0) {}

    void borrowBook(const string& isbn, const string& title) {
        int borrowDays = (userType == "Student") ? 10 : 30;

        time_t    now   = time(0);
        struct tm* dueTm = localtime(&now);
        dueTm->tm_mday += borrowDays;
        dueTm->tm_hour  = 23; // Due at end-of-day
        dueTm->tm_min   = 59;
        dueTm->tm_sec   = 59;
        time_t dueTime = mktime(dueTm);

        string borrowedDate = formatTimestamp(now);
        string dueDate      = formatTimestamp(dueTime);

        borrowedBooks.emplace_back(isbn, title, borrowedDate, "Not Returned", dueDate, "Not Returned", 0);
    }

    // Returns the number of calendar days by which date1 is ahead of date2.
    // A positive result means date1 is later; negative means date1 is earlier.
    int calculateDifferenceInDays(const string& date1, const string& date2) {
        struct tm tm1 = {}, tm2 = {};
        sscanf(date1.c_str(), "%d/%d/%d", &tm1.tm_mday, &tm1.tm_mon, &tm1.tm_year);
        sscanf(date2.c_str(), "%d/%d/%d", &tm2.tm_mday, &tm2.tm_mon, &tm2.tm_year);

        tm1.tm_mon -= 1; tm1.tm_year -= 1900;
        tm2.tm_mon -= 1; tm2.tm_year -= 1900;

        double diff = difftime(mktime(&tm1), mktime(&tm2));
        return static_cast<int>(diff / 86400);
    }

    // Marks the book as returned and calculates any applicable fine.
    // Returns true if a matching unreturned copy was found; false otherwise.
    bool returnBook(const string& isbn, const string& callerUserType) {
        time_t now = time(0);
        string returnedDate = formatTimestamp(now);
        string currentDate  = formatTimestamp(now).substr(0, 10); // "dd/mm/yyyy"

        for (auto& book : borrowedBooks) {
            if (get<0>(book) == isbn && get<5>(book) != "Returned") {
                get<3>(book) = returnedDate;
                get<5>(book) = "Returned";

                string dueDate    = get<4>(book).substr(0, 10);
                int    overdueDays = calculateDifferenceInDays(currentDate, dueDate);

                cout << "\nBook returned successfully!\n";

                if (callerUserType == "Student") {
                    if (overdueDays > 0) {
                        int fine = overdueDays * 10; // ₹10 per overdue day
                        get<6>(book) = fine;
                        fines += fine;
                        cout << "Overdue period: " << overdueDays << " days\n";
                        cout << "Fine incurred: Rs " << fine << "\n";
                        cout << "Please return books on time in the future.\n";
                    } else {
                        cout << "Overdue period: 0 days\nFine incurred: Rs 0\n";
                    }
                } else if (callerUserType == "Faculty") {
                    cout << "Overdue period: " << max(0, overdueDays) << " days\n";
                    if (overdueDays > 0) cout << "Please return books on time in the future.\n";
                }
                return true;
            }
        }
        return false; // ISBN not found or already returned
    }

    // Returns true only if the user currently holds an unreturned copy of isbn.
    bool hasBook(const string& isbn) const {
        for (const auto& book : borrowedBooks) {
            if (get<0>(book) == isbn && get<5>(book) != "Returned") {
                return true;
            }
        }
        return false;
    }

    int  getFine() const { 
        return fines; 
    }
    void setFine(int amount){ 
        fines = amount; 
    }

    // Serialises all borrow records to a pipe/semicolon-delimited string for CSV storage.
    string getBorrowedBooks() const {
        string result;
        for (const auto& b : borrowedBooks) {
            result += get<0>(b) + "|" + get<1>(b) + "|" + get<2>(b) + "|" +
                      get<3>(b) + "|" + get<4>(b) + "|" + get<5>(b) + "|" +
                      to_string(get<6>(b)) + ";";
        }
        return result;
    }

    vector<tuple<string, string, string, string, string, string, int>> getBorrowedBooksList() const {
        return borrowedBooks;
    }

    // Deserialises borrow records written by getBorrowedBooks() and restores the fine total.
    void loadFromCSV(const string& books, int fineAmount) {
        borrowedBooks.clear();
        stringstream ss(books);
        string entry;
        while (getline(ss, entry, ';')) {
            if (entry.empty()) continue;
            stringstream es(entry);
            string isbn, title, borrowedDate, returnedDate, dueDate, status;
            int fine;
            getline(es,isbn,'|');
            getline(es,title,'|');
            getline(es,borrowedDate, '|');
            getline(es,returnedDate, '|');
            getline(es,dueDate,'|');
            getline(es,status, '|');
            es >> fine;
            borrowedBooks.emplace_back(isbn, title, borrowedDate, returnedDate, dueDate, status, fine);
        }
        fines = fineAmount;
    }

    // Returns only the records where the book has already been returned.
    vector<tuple<string, string, string, string, string, string, int>> getPastBorrowedBooksList() const {
        vector<tuple<string, string, string, string, string, string, int>> past;
        for (const auto& b : borrowedBooks)
            if (get<5>(b) == "Returned") past.push_back(b);
        return past;
    }

    // Counts books that have not yet been returned.
    int getCurrentlyBorrowedCount() const {
        int count = 0;
        for (const auto& b : borrowedBooks)
            if (get<5>(b) != "Returned") ++count;
        return count;
    }

    // Recalculates fines for all unreturned books and updates their status.
    // Should be called each time a Student reaches the main menu so fines stay current.
    void calculateFines() {
        int totalFine = 0;
        string currentDate = formatTimestamp(time(0)).substr(0, 10);

        for (auto& b : borrowedBooks) {
            if (get<5>(b) == "Returned") continue;

            int overdueDays = calculateDifferenceInDays(currentDate, get<4>(b).substr(0, 10));
            if (overdueDays > 0 && userType == "Student") {
                int fine = overdueDays * 10;
                get<6>(b) = fine;
                get<5>(b) = "Overdue";
                totalFine += fine;
            }
        }
        fines = totalFine;
    }

    // Faculty cannot borrow additional books if any copy has been overdue for more than 60 days.
    bool hasOverdueBooksForMoreThan60Days() const {
        string currentDate = formatTimestamp(time(0)).substr(0, 10);
        for (const auto& b : borrowedBooks) {
            if (get<5>(b) == "Not Returned" || get<5>(b) == "Overdue") {
                int days = const_cast<Account*>(this)->calculateDifferenceInDays(
                    currentDate, get<4>(b).substr(0, 10));
                if (days > 60) return true;
            }
        }
        return false;
    }

    // Returns true if any unreturned book is past its due timestamp (to-the-second precision).
    bool hasOverdueBooks() const {
        time_t now = time(0);
        for (const auto& b : borrowedBooks) {
            if (get<5>(b) != "Returned" && now > parseTimestamp(get<4>(b)))
                return true;
        }
        return false;
    }

    // Returns all currently overdue books and updates their status to "Overdue" in-place.
    vector<tuple<string, string, string, string, string, string, int>> getOverdueBooks() {
        vector<tuple<string, string, string, string, string, string, int>> overdue;
        time_t now = time(0);
        for (auto& b : borrowedBooks) {
            if (get<5>(b) != "Returned" && now > parseTimestamp(get<4>(b))) {
                get<5>(b) = "Overdue";
                overdue.push_back(b);
            }
        }
        return overdue;
    }
};

// ─────────────────────────────────────────────
// User (abstract base)
// ─────────────────────────────────────────────

class User {
protected:
    int    user_id;
    string username, password, role;
    string name;
    int    age;
    string gender;
    Account account;

public:
    User(int id, string u, string p, string r,
         const string& borrowedBooks, int fineAmount,
         string n = "", int a = 0, string g = "")
        : user_id(id), username(u), password(p), role(r),
          name(n), age(a), gender(g), account(r)
    {
        account.loadFromCSV(borrowedBooks, fineAmount);
    }

    virtual bool        checkCanBorrow()              = 0;
    virtual void        borrowBook(Book& book)         = 0;
    virtual void        returnBook(Book& book)         = 0;
    virtual string      getRole()              const   = 0;
    virtual void        showDetails()          const   = 0;

    bool authenticate(const string& u, const string& p) const {
        return (u == username && p == password);
    }

    Account& getAccount() { return account; }

    int    getUserID()   const { return user_id; }
    string getUsername() const { return username; }
    string getPassword() const { return password; }
    string getName()     const { return name; }
    int    getAge()      const { return age; }
    string getGender()   const { return gender; }

    void update_user_id(int id) { user_id = id; }

    // Serialises the user record to a single CSV line for persistent storage.
    string toCSV() const {
        return to_string(user_id) + "," + username + "," + password + "," + role + "," +
               name + "," + to_string(age) + "," + gender + "," +
               account.getBorrowedBooks() + "," + to_string(account.getFine()) + "\n";
    }

    virtual void editProfile() {
        string currentPassword, newUsername, newPassword, newName, newGender;

        cout << "\nEnter new username: ";
        cin >> newUsername;

        cout << "Enter your current password: ";
        cin >> currentPassword;

        if (currentPassword != password) {
            cout << "\nIncorrect current password. Profile update cancelled.\n";
            return;
        }
        cout << "Password verified!\n";

        cout << "Enter new password: ";
        cin >> newPassword;

        // Use getline so names with spaces are accepted.
        cout << "Enter new full name: ";
        cin.ignore();
        getline(cin, newName);

        int newAge = getValidIntegerInput("Enter new age: ");

        while (true) {
            cout << "Enter gender (M/F/O): ";
            cin >> newGender;
            if (newGender == "M" || newGender == "F" || newGender == "O") break;
            cout << "Invalid gender! Please enter M, F, or O.\n";
        }

        username = newUsername;
        password = newPassword;
        name     = newName;
        age      = newAge;
        gender   = newGender;

        cout << "\nProfile updated successfully!\n";
    }
};

// ─────────────────────────────────────────────
// Student
// ─────────────────────────────────────────────

class Student : public User {
public:
    Student(int id, string u, string p,
            const string& borrowedBooks, int fineAmount,
            string n = "", int a = 0, string g = "")
        : User(id, u, p, "Student", borrowedBooks, fineAmount, n, a, g) {}

    string getRole() const override { return "Student"; }

    bool checkCanBorrow() override {
        if (account.getFine() > 0) {
            cout << "\nYou have unpaid fines. Please clear them before borrowing.\n";
            return false;
        }
        if (account.getCurrentlyBorrowedCount() >= 3) {
            cout << "Borrow limit reached (3 books max). Return a book before borrowing another.\n";
            return false;
        }
        return true;
    }

    void borrowBook(Book& book) override {
        if (book.status == "Reserved") {
            cout << "\nSorry, this book is RESERVED and cannot be borrowed.\n";
            return;
        }
        if (book.status != "Available") {
            cout << "\nSorry, this book is not available.\n";
            return;
        }
        account.borrowBook(book.isbn, book.title);
        book.status = "Borrowed";
        cout << "Book borrowed successfully!\n";
    }

    void returnBook(Book& book) override {
        if (!account.hasBook(book.isbn)) {
            cout << "You don't currently have this book borrowed.\n";
            return;
        }
        bool returned = account.returnBook(book.isbn, "Student");
        if (!returned) cout << "You already returned this book!\n";
        else           book.status = "Available";
    }

    void showDetails() const override {
        cout << "User ID: "  << user_id  << ", Username: " << username
             << ", Name: "   << name     << "\nAge: "      << age
             << ", Gender: " << gender   << ", Role: Student"
             << ", Fine: "   << account.getFine() << " rupees\n";
    }
};

// ─────────────────────────────────────────────
// Faculty
// ─────────────────────────────────────────────

class Faculty : public User {
public:
    Faculty(int id, string u, string p,
            const string& borrowedBooks, int fineAmount,
            string n = "", int a = 0, string g = "")
        : User(id, u, p, "Faculty", borrowedBooks, fineAmount, n, a, g) {}

    string getRole() const override { return "Faculty"; }

    bool checkCanBorrow() override {
        // Faculty with a copy overdue by more than 60 days cannot borrow new books.
        if (account.hasOverdueBooksForMoreThan60Days()) {
            cout << "\nYou have a book overdue by more than 60 days. Please return it first.\n";
            return false;
        }
        if (account.getCurrentlyBorrowedCount() >= 5) {
            cout << "Borrow limit reached (5 books max). Return a book before borrowing another.\n";
            return false;
        }
        return true;
    }

    void borrowBook(Book& book) override {
        if (book.status == "Reserved") {
            cout << "\nSorry, this book is RESERVED and cannot be borrowed.\n";
            return;
        }
        if (book.status != "Available") {
            cout << "\nSorry, this book is not available.\n";
            return;
        }
        account.borrowBook(book.isbn, book.title);
        book.status = "Borrowed";
        cout << "Book borrowed successfully!\n";
    }

    void returnBook(Book& book) override {
        if (!account.hasBook(book.isbn)) {
            cout << "You don't currently have this book borrowed.\n";
            return;
        }
        bool returned = account.returnBook(book.isbn, "Faculty");
        if (!returned) cout << "You already returned this book!\n";
        else           book.status = "Available";
    }

    void showDetails() const override {
        cout << "User ID: "  << user_id  << ", Username: " << username
             << ", Name: "   << name     << "\nAge: "      << age
             << ", Gender: " << gender   << ", Role: Faculty\n";
    }
};

// ─────────────────────────────────────────────
// Librarian
// ─────────────────────────────────────────────

class Librarian : public User {
public:
    Librarian(int id, string u, string p,
              const string& borrowedBooks, int fineAmount,
              string n = "", int a = 0, string g = "")
        : User(id, u, p, "Librarian", borrowedBooks, fineAmount, n, a, g) {}

    string getRole() const override { return "Librarian"; }

    bool checkCanBorrow() override {
        cout << "Librarians cannot borrow books.\n";
        return false;
    }
    void borrowBook(Book&) override { cout << "Librarians cannot borrow books.\n"; }
    void returnBook(Book&) override { cout << "Librarians cannot return books.\n"; }

    void showDetails() const override {
        cout << "User ID: "  << user_id  << ", Username: " << username
             << ", Name: "   << name     << "\nAge: "      << age
             << ", Gender: " << gender   << ", Role: Librarian\n";
    }

    void addBook(vector<Book>& library) {
        string genre, title, author, publisher, isbn, status;
        int year;
        cin.ignore(); // Discard the newline left by the previous menu choice
        cout << "Enter genre: ";     getline(cin, genre);
        cout << "Enter title: ";     getline(cin, title);
        cout << "Enter author: ";    getline(cin, author);
        cout << "Enter publisher: "; getline(cin, publisher);
        year = getValidIntegerInput("Enter year: ");
        cout << "Enter ISBN: ";   cin >> isbn;
        cout << "Enter status (Available/Reserved): "; cin >> status;

        if (status != "Available" && status != "Reserved") {
            cout << "Invalid status. Defaulting to 'Available'.\n";
            status = "Available";
        }
        library.emplace_back(genre, title, author, publisher, year, isbn, status);
        cout << "\nBook added successfully!\n";
    }

    void removeBook(vector<Book>& library) {
        cout << "Enter book ISBN to remove: ";
        string isbn; cin >> isbn;
        for (auto it = library.begin(); it != library.end(); ++it) {
            if (it->isbn == isbn) {
                library.erase(it);
                cout << "Book removed successfully!\n";
                return;
            }
        }
        cout << "Book not found!\n";
    }

    void updateBook(vector<Book>& library) {
        cout << "Enter book ISBN to update: ";
        string isbn; cin >> isbn;
        for (auto& book : library) {
            if (book.isbn == isbn) {
                cin.ignore(); // Discard the newline before reading string fields
                cout << "Enter new genre: ";     getline(cin, book.genre);
                cout << "Enter new title: ";     getline(cin, book.title);
                cout << "Enter new author: ";    getline(cin, book.author);
                cout << "Enter new publisher: "; getline(cin, book.publisher);
                book.year = getValidIntegerInput("Enter new year: ");
                string status;
                cout << "Enter new status (Available/Reserved): "; cin >> status;
                if (status != "Available" && status != "Reserved") {
                    cout << "Invalid status. Defaulting to 'Available'.\n";
                    book.status = "Available";
                } else {
                    book.status = status;
                }
                cout << "\nBook updated successfully!\n\n";
                return;
            }
        }
        cout << "\nBook not found!\n";
    }

    void addUser(vector<User*>& users) {
        string name, gender, username, password;
        int id = users.size() + 1; // Assign the next sequential user ID
        cin.ignore();
        cout << "Enter full name: "; getline(cin, name);
        int age = getValidIntegerInput("Enter age: ");
        while (true) {
            cout << "Enter gender (M/F/O): "; cin >> gender;
            if (gender == "M" || gender == "F" || gender == "O") break;
            cout << "Invalid gender! Please enter M, F, or O.\n";
        }
        cout << "Enter username: "; cin >> username;
        cout << "Enter password: "; cin >> password;

        int choice;
        while (true) {
            choice = getValidIntegerInput("Choose role (1 Student, 2 Faculty, 3 Librarian): ");
            if (choice >= 1 && choice <= 3) break;
            cout << "Invalid choice! Enter 1, 2, or 3.\n";
        }

        if (choice == 1)      users.push_back(new Student (id, username, password, "", 0, name, age, gender));
        else if (choice == 2) users.push_back(new Faculty (id, username, password, "", 0, name, age, gender));
        else                  users.push_back(new Librarian(id, username, password, "", 0, name, age, gender));

        cout << "\nUser added successfully! Assigned User ID: " << id << "\n";
    }

    void removeUser(vector<User*>& users) {
        cout << "\nEnter user ID to remove: ";
        int id; cin >> id;

        for (auto it = users.begin(); it != users.end(); ++it) {
            if ((*it)->getUserID() == id) {
                delete *it;
                users.erase(it);
                cout << "\nUser removed successfully!\n";
                // Re-number remaining users so IDs stay contiguous from 1.
                for (size_t i = 0; i < users.size(); ++i)
                    users[i]->update_user_id(static_cast<int>(i) + 1);
                return;
            }
        }
        cout << "User not found!\n";
    }
};

// ─────────────────────────────────────────────
// Library
// ─────────────────────────────────────────────

class Library {
public:
    vector<Book>  books;
    vector<User*> users;

    void addBook(Book book)   { books.push_back(book); }
    void addUser(User* user)  { users.push_back(user); }

    // Tries to open the file for appending; if it fails the file is locked by another process.
    bool isFileLocked(const string& filename) {
        ofstream file(filename, ios::app);
        return !file;
    }

    void saveBooksToFile() {
        if (isFileLocked("books.csv")) {
            cerr << "Error: books.csv is open in another program. Close it and try again.\n";
            return;
        }
        ofstream file("books.csv");
        if (!file) { cerr << "Error: Cannot open books.csv for writing.\n"; return; }
        for (const auto& b : books) file << b.toCSV();
    }

    void loadBooksFromFile() {
        ifstream file("books.csv");
        string g, t, a, p, i, s;
        int y;
        while (getline(file, g, ',') && getline(file, t, ',') &&
               getline(file, a, ',') && getline(file, p, ',') &&
               file >> y && file.ignore()   &&
               getline(file, i, ',')        &&
               getline(file, s)) {
            books.emplace_back(g, t, a, p, y, i, s);
        }
    }

    void saveUsersToFile() {
        if (isFileLocked("users.csv")) {
            cerr << "Error: users.csv is open in another program. Close it and try again.\n";
            return;
        }
        ofstream file("users.csv");
        if (!file) { cerr << "Error: Cannot open users.csv for writing.\n"; return; }
        for (auto u : users) file << u->toCSV();
    }

    void loadUsersFromFile() {
        ifstream file("users.csv");
        string line;
        users.clear();
        while (getline(file, line)) {
            stringstream ss(line);
            int id, age, fineAmount;
            string username, password, role, name, gender, borrowedBooks;

            ss >> id; ss.ignore();
            getline(ss, username,     ',');
            getline(ss, password,     ',');
            getline(ss, role,         ',');
            getline(ss, name,         ',');
            ss >> age; ss.ignore();
            getline(ss, gender,       ',');
            getline(ss, borrowedBooks,',');
            ss >> fineAmount;

            if      (role == "Student")   users.push_back(new Student  (id, username, password, borrowedBooks, fineAmount, name, age, gender));
            else if (role == "Faculty")   users.push_back(new Faculty  (id, username, password, borrowedBooks, fineAmount, name, age, gender));
            else if (role == "Librarian") users.push_back(new Librarian(id, username, password, borrowedBooks, fineAmount, name, age, gender));
        }
    }

    int getNextUserID() const { return static_cast<int>(users.size()) + 1; }
};

// ─────────────────────────────────────────────
// Table-printing helpers
// ─────────────────────────────────────────────

void printAllTableHeader() {
    cout << left << setw(15) << "ISBN"
                 << setw(15) << "Genre"
                 << setw(45) << "Title"
                 << setw(25) << "Author"
                 << setw(10) << "Status" << "\n";
    cout << setfill('-') << setw(110) << "" << setfill(' ') << "\n";
}

void printBookTableRow(const Book& b) {
    cout << left << setw(15) << b.isbn
                 << setw(15) << b.genre
                 << setw(45) << b.title
                 << setw(25) << b.author
                 << setw(10) << b.status << "\n";
}

void printCurrentlyBorrowedTableHeader() {
    cout << left << setw(14) << "ISBN"
                 << setw(45) << "Title"
                 << setw(14) << "Status"
                 << setw(21) << "Borrowed Date"
                 << setw(21) << "Due Date"
                 << setw(5)  << "Fine" << "\n";
    cout << setfill('-') << setw(120) << "" << setfill(' ') << "\n";
}

void printTableHeader() {
    cout << left << setw(14) << "ISBN"
                 << setw(45) << "Title"
                 << setw(14) << "Status"
                 << setw(21) << "Borrowed Date"
                 << setw(21) << "Returned Date"
                 << setw(5)  << "Fine" << "\n";
    cout << setfill('-') << setw(120) << "" << setfill(' ') << "\n";
}

void printCurrentlyBorrowedBookTableRow(
    const tuple<string,string,string,string,string,string,int>& b)
{
    cout << left << setw(15) << get<0>(b)
                 << setw(45) << get<1>(b)
                 << setw(14) << get<5>(b)
                 << setw(21) << get<2>(b)
                 << setw(21) << get<4>(b)
                 << setw(5)  << get<6>(b) << "\n";
}

void printBorrowedBookTableRow(
    const tuple<string,string,string,string,string,string,int>& b)
{
    cout << left << setw(15) << get<0>(b)
                 << setw(45) << get<1>(b)
                 << setw(14) << get<5>(b)
                 << setw(21) << get<2>(b)
                 << setw(21) << get<3>(b)
                 << setw(5)  << get<6>(b) << "\n";
}

void printUserTableHeader() {
    cout << left << setw(10) << "User ID"
                 << setw(20) << "Username"
                 << setw(30) << "Name"
                 << setw(10) << "Age"
                 << setw(10) << "Gender"
                 << setw(15) << "Role" << "\n";
    cout << setfill('-') << setw(90) << "" << setfill(' ') << "\n";
}

void printUserTableRow(const User* u) {
    cout << left << setw(10) << u->getUserID()
                 << setw(20) << u->getUsername()
                 << setw(30) << u->getName()
                 << setw(10) << u->getAge()
                 << setw(10) << u->getGender()
                 << setw(15) << u->getRole() << "\n";
}

// ─────────────────────────────────────────────
// Menu logic
// ─────────────────────────────────────────────

void mainMenu(Library& lib, User* currentUser) {
    while (true) {
        cout << "\n=========== Main Menu ===========\n"
             << "\n1. Edit Profile"
             << "\n2. View All Books"
             << "\n3. Borrow Book"
             << "\n4. Return Book"
             << "\n5. View Currently Borrowed Books"
             << "\n6. View Past Borrowing History"
             << "\n7. View Fines"
             << "\n8. View Overdue Books"
             << "\n9. Pay Fines"
             << "\n10. Logout\n";

        // Refresh fine totals and overdue statuses before showing the menu prompt.
        if (currentUser->getRole() == "Student")
            currentUser->getAccount().calculateFines();
        else if (currentUser->getRole() == "Faculty")
            currentUser->getAccount().getOverdueBooks(); // updates status flags in-place

        int choice;
        while (true) {
            choice = getValidIntegerInput("\nChoose an option (1-10): ");
            if (choice >= 1 && choice <= 10) break;
            cout << "Invalid choice! Enter a number between 1 and 10.\n";
        }

        if (choice == 1) {
            currentUser->editProfile();
            lib.saveUsersToFile();

        } else if (choice == 2) {
            cout << "\nLibrary Books:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

        } else if (choice == 3) {
            if (!currentUser->checkCanBorrow()) continue;
            cout << "\nLibrary Books:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

            cout << "\nEnter the ISBN of the book to borrow: ";
            string isbn; cin >> isbn;
            bool found = false;
            for (auto& b : lib.books) {
                if (b.isbn == isbn) { currentUser->borrowBook(b); found = true; break; }
            }
            if (!found) { cout << "\nInvalid ISBN!\n"; continue; }
            lib.saveUsersToFile();
            lib.saveBooksToFile();

        } else if (choice == 4) {
            cout << "\nYour Currently Borrowed Books:\n\n";
            printCurrentlyBorrowedTableHeader();
            auto allBooks = currentUser->getAccount().getBorrowedBooksList();
            bool hasBorrowed = false;
            for (const auto& b : allBooks) {
                if (get<5>(b) != "Returned") { printCurrentlyBorrowedBookTableRow(b); hasBorrowed = true; }
            }
            if (!hasBorrowed) { cout << "\nYou have no books to return.\n"; continue; }

            cout << "\nEnter the ISBN of the book to return: ";
            string isbn; cin >> isbn;
            bool found = false;
            for (auto& b : lib.books) {
                if (b.isbn == isbn) { currentUser->returnBook(b); found = true; break; }
            }
            if (!found) { cout << "\nInvalid ISBN!\n"; continue; }
            lib.saveUsersToFile();
            lib.saveBooksToFile();

        } else if (choice == 5) {
            cout << "\nYour Currently Borrowed Books:\n\n";
            printCurrentlyBorrowedTableHeader();
            bool hasBorrowed = false;
            for (const auto& b : currentUser->getAccount().getBorrowedBooksList()) {
                if (get<5>(b) != "Returned") { printCurrentlyBorrowedBookTableRow(b); hasBorrowed = true; }
            }
            if (!hasBorrowed) cout << "\nYou have no currently borrowed books.\n";

        } else if (choice == 6) {
            cout << "\nYour Past Borrowing History:\n\n";
            printTableHeader();
            auto past = currentUser->getAccount().getPastBorrowedBooksList();
            if (!past.empty()) for (const auto& b : past) printBorrowedBookTableRow(b);
            else               cout << "You have no past borrowing history.\n";

        } else if (choice == 7) {
            if (currentUser->getRole() == "Student")
                cout << "\nTotal Fine: " << currentUser->getAccount().getFine() << " rupees.\n";
            else
                cout << "\nFines are not applicable for faculty members.\n";

        } else if (choice == 8) {
            auto overdue = currentUser->getAccount().getOverdueBooks();
            if (!overdue.empty()) {
                cout << "\nYour Overdue Books:\n";
                printCurrentlyBorrowedTableHeader();
                for (const auto& b : overdue) printCurrentlyBorrowedBookTableRow(b);
            } else {
                cout << "\nNo overdue books.\n";
            }

        } else if (choice == 9) {
            if (currentUser->getRole() == "Student") {
                int fine = currentUser->getAccount().getFine();
                if (fine > 0) {
                    cout << "\nOutstanding fine: " << fine << " rupees.\nSimulating payment...\n";
                    currentUser->getAccount().setFine(0);
                    cout << "Payment successful! Your fines have been cleared.\n";
                } else {
                    cout << "\nYou have no outstanding fines.\n";
                }
            } else {
                cout << "\nNo fines for faculty members.\n";
            }

        } else { // choice == 10
            break;
        }
    }
}

void librarianMenu(Library& lib, Librarian* librarian) {
    while (true) {
        cout << "\n===== Librarian Menu =====\n\n"
             << "1. Edit Profile\n2. View All Books\n3. Add Book\n4. Remove Book\n"
             << "5. Update Book\n6. View All Users\n7. Add User\n8. Remove User\n9. Logout\n";

        int choice;
        while (true) {
            choice = getValidIntegerInput("\nChoose an option (1-9): ");
            if (choice >= 1 && choice <= 9) break;
            cout << "Invalid choice! Enter a number between 1 and 9.\n";
        }

        if (choice == 1) {
            librarian->editProfile();
            lib.saveUsersToFile();

        } else if (choice == 2) {
            cout << "\nLibrary Books:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

        } else if (choice == 3) {
            cout << "\n===== Add Book =====\n";
            librarian->addBook(lib.books);
            lib.saveBooksToFile();
            cout << "\nUpdated Library Book List:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

        } else if (choice == 4) {
            cout << "\n===== Remove Book =====\n\nCurrent Library Books:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);
            cout << "\n";
            librarian->removeBook(lib.books);
            lib.saveBooksToFile();
            cout << "\nUpdated Library Book List:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

        } else if (choice == 5) {
            cout << "\n===== Update Book =====\n\nCurrent Library Books:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);
            cout << "\n";
            librarian->updateBook(lib.books);
            lib.saveBooksToFile();
            cout << "Updated Library Book List:\n\n";
            printAllTableHeader();
            for (const auto& b : lib.books) printBookTableRow(b);

        } else if (choice == 6) {
            cout << "\nLibrary Users:\n\n";
            printUserTableHeader();
            for (const auto& u : lib.users) printUserTableRow(u);

        } else if (choice == 7) {
            cout << "\n===== Add User =====\n\n";
            librarian->addUser(lib.users);
            lib.saveUsersToFile();
            cout << "\nUpdated Users:\n\n";
            printUserTableHeader();
            for (const auto& u : lib.users) printUserTableRow(u);

        } else if (choice == 8) {
            cout << "\n===== Remove User =====\n\nCurrent Users:\n\n";
            printUserTableHeader();
            for (const auto& u : lib.users) printUserTableRow(u);
            librarian->removeUser(lib.users);
            lib.saveUsersToFile();
            cout << "\nUpdated User List:\n\n";
            printUserTableHeader();
            for (const auto& u : lib.users) printUserTableRow(u);

        } else { // choice == 9
            cout << "\nLogging out...\n";
            break;
        }
    }
}

// ─────────────────────────────────────────────
// Auth screens
// ─────────────────────────────────────────────

void printCentered(const string& text, int width = 60) {
    int pad = (static_cast<int>(width) - static_cast<int>(text.length())) / 2;
    if (pad > 0) cout << string(pad, ' ');
    cout << text << "\n";
}

void signup(Library& lib) {
    string username, password, name, gender;

    // cin.ignore() discards the newline left by the previous integer/token read
    // so getline doesn't immediately return an empty string.
    cout << "\nEnter your full name: ";
    cin.ignore();
    getline(cin, name);

    int age = getValidIntegerInput("Enter your age in years: ");

    while (true) {
        cout << "Enter gender (M/F/O): "; cin >> gender;
        if (gender == "M" || gender == "F" || gender == "O") break;
        cout << "Invalid gender! Please enter M, F, or O.\n";
    }

    cout << "Enter username (no spaces): "; cin >> username;
    cout << "Enter password (no spaces): "; cin >> password;

    int choice;
    while (true) {
        choice = getValidIntegerInput("Choose role (1 Student, 2 Faculty, 3 Librarian): ");
        if (choice >= 1 && choice <= 3) break;
        cout << "Invalid choice! Enter 1, 2, or 3.\n";
    }

    int id = lib.getNextUserID();
    if      (choice == 1) lib.addUser(new Student  (id, username, password, "", 0, name, age, gender));
    else if (choice == 2) lib.addUser(new Faculty  (id, username, password, "", 0, name, age, gender));
    else                  lib.addUser(new Librarian(id, username, password, "", 0, name, age, gender));

    lib.saveUsersToFile();
    cout << "\nSignup successful! Your User ID is " << id << ". You can now log in.\n";
}

void login(Library& lib) {
    string username, password;
    cout << "\n======== Login ========\n";
    cout << "Enter username: "; cin >> username;
    cout << "Enter password: "; cin >> password;

    for (auto user : lib.users) {
        if (!user->authenticate(username, password)) continue;

        cout << "\n============================================================\n";
        printCentered("Welcome, " + user->getName() + "!");
        cout << "============================================================\n\n";
        user->showDetails();

        // Notify the user of any overdue books immediately after login.
        auto overdue = user->getAccount().getOverdueBooks();
        if (!overdue.empty()) {
            cout << "\nYou have overdue book(s):\n";
            printTableHeader();
            for (const auto& b : overdue) printBorrowedBookTableRow(b);
        } else {
            cout << "\nNo overdue books.\n";
        }

        cout << "\n";
        if (user->getRole() == "Librarian")
            librarianMenu(lib, dynamic_cast<Librarian*>(user));
        else
            mainMenu(lib, user);
        return;
    }
    cout << "\nInvalid credentials!\n";
}

// ─────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────

void displayWelcomePage() {
    cout << "\n============================================================\n";
    printCentered("WELCOME TO");
    cout << "\n";
    printCentered("LIBRARY MANAGEMENT SYSTEM");
    cout << "\n";
    printCentered("Made by Vaibhav");
    cout << "============================================================\n";
}

int main() {
    Library lib;

    ifstream booksFile("books.csv");
    ifstream usersFile("users.csv");
    bool booksEmpty = booksFile.peek() == ifstream::traits_type::eof();
    bool usersEmpty = usersFile.peek() == ifstream::traits_type::eof();
    booksFile.close();
    usersFile.close();

    if (booksEmpty && usersEmpty) {
        cout << "\n\nInitializing library with default data...\n";

        lib.addBook(Book("Fiction",     "The Great Gatsby",                        "F. Scott Fitzgerald", "Scribner",             1925, "9780743273565", "Available"));
        lib.addBook(Book("Fiction",     "To Kill a Mockingbird",                   "Harper Lee",          "J.B. Lippincott & Co.",1960, "9780061120084", "Available"));
        lib.addBook(Book("Fiction",     "1984",                                    "George Orwell",       "Secker & Warburg",     1949, "9780451524935", "Available"));
        lib.addBook(Book("Non-Fiction", "Sapiens: A Brief History of Humankind",   "Yuval Noah Harari",   "Harper",               2011, "9780062316097", "Available"));
        lib.addBook(Book("Self-Help",   "The Power of Now",                        "Eckhart Tolle",       "New World Library",    1997, "9781577314806", "Available"));
        lib.addBook(Book("Science",     "A Brief History of Time",                 "Stephen Hawking",     "Bantam Books",         1988, "9780553380163", "Available"));
        lib.addBook(Book("Science",     "Cosmos",                                  "Carl Sagan",          "Random House",         1980, "9780345331359", "Available"));
        lib.addBook(Book("Fantasy",     "The Hobbit",                              "J.R.R. Tolkien",      "Allen & Unwin",        1937, "9780547928227", "Reserved"));
        lib.addBook(Book("Fantasy",     "Harry Potter and the Philosopher's Stone","J.K. Rowling",        "Bloomsbury",           1997, "9780747532699", "Available"));
        lib.addBook(Book("Mystery",     "The Da Vinci Code",                       "Dan Brown",           "Doubleday",            2003, "9780307474278", "Available"));

        lib.addUser(new Student  (1, "vaibhav", "10001", "", 0, "Vaibhav Gupta",           20, "M"));
        lib.addUser(new Student  (2, "shanaya",  "10002", "", 0, "Shanaya Srivastava",   22, "F"));
        lib.addUser(new Student  (3, "rahul",  "10003", "", 0, "Rahul Verma",          24, "M"));
        lib.addUser(new Student  (4, "anita",  "10004", "", 0, "Anita Singh",          23, "F"));
        lib.addUser(new Student  (5, "soham",  "10005", "", 0, "Soham Das",            21, "M"));
        lib.addUser(new Faculty  (6, "virat",   "20001", "", 0, "Virat Kohli",           35, "M"));
        lib.addUser(new Faculty  (7, "raj",    "20002", "", 0, "Rajkumar Jain",        40, "M"));
        lib.addUser(new Faculty  (8, "amit",   "20003", "", 0, "Amit Vinayak Shinde",  38, "M"));
        lib.addUser(new Librarian(9, "ms",   "30001", "", 0, "MS Dhoni",       45, "M"));

        lib.saveBooksToFile();
        lib.saveUsersToFile();
        cout << "Default data initialized successfully!\n";
    } else {
        cout << "\n\nLoading data from files...\n";
        lib.loadUsersFromFile();
        lib.loadBooksFromFile();
    }

    displayWelcomePage();
    cout << "\nHello there! What do you want to do today?\n\n";

    while (true) {
        cout << "1. Login\n2. Signup\n3. Exit\n";
        int choice; cin >> choice;
        if      (choice == 1) login(lib);
        else if (choice == 2) signup(lib);
        else if (choice == 3) { cout << "\nGoodbye! Have a nice day.\n"; break; }
        else                  cout << "\nInvalid choice! Please try again.\n";
        cout << "\n==== Initial Menu ====\n";
    }
    return 0;
}