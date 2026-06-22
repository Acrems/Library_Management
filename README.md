
# Library Management System (C++)

A terminal based Library Management System built entirely in C++.

I created this project as a self  learning exercise to learn Object-Oriented Programming (OOP) concepts in C++. By building this system from scratch, I gained practical experience in designing class hierarchies, managing memory, and implementing persistent data storage using standard file I/O.

## Learning Journey: Mastering OOP

This project served as a sandbox for applying theoretical OOP concepts to a real-world scenario. Here is how the core pillars of OOP are implemented in this codebase:

### Encapsulation

Data and the functions that manipulate them are bundled into coherent classes. For example, the `Account` class securely handles sensitive operations like fine calculation and due date tracking, hiding the complex `time_t` logic from the rest of the application.

### Inheritance

To avoid code duplication, I created an abstract base class called `User`. The `Student`, `Faculty`, and `Librarian` classes inherit from `User`, gaining standard attributes (ID, name, password) while defining their own specific behaviors.

### Polymorphism

The system uses virtual functions (like `checkCanBorrow()`, `borrowBook()`, and `returnBook()`). When the core program calls `user->borrowBook()`, C++ automatically determines at runtime whether to apply the strict 3-book limit for a `Student`, the 5-book limit for `Faculty`, or block the action entirely if the user is a `Librarian`.

### Abstraction

The `Library` class provides a clean, high-level interface to the main menu, completely hiding the messy details of how strings are parsed and serialized into `.csv` files behind the scenes.

## Class Architecture

### 1. Book

A simple data model representing a book in the library. It stores attributes like `genre`, `title`, `author`, `publisher`, `isbn`, `year`, and status (`Available`/`Reserved`/`Borrowed`).

### 2. Account

The engine behind user borrowing limits and fines.

* Maintains a history of `BorrowRecord` (a tuple tracking ISBN, borrowed date, due date, status, and fines).
* Handles date math to automatically calculate overdue days and monetary fines based on the user's role.

### 3. User (Abstract Base Class)

The blueprint for anyone interacting with the system. It holds basic demographic data, credentials, and an instance of an `Account`.

### 4. Derived User Classes

#### Student

* Can borrow up to 3 books for 10 days.
* Incurs a ₹10 fine per overdue day.
* Cannot borrow new books if fines are unpaid.

#### Faculty

* Can borrow up to 5 books for 30 days.
* Does not incur standard monetary fines.
* Cannot borrow new books if any held book is more than 60 days overdue.

#### Librarian

An administrative role.

* Cannot borrow books.
* Has exclusive access to a special menu to add/remove/update books in the catalog and manage user accounts.

### 5. Library

The central state manager.

* Holds the vectors of all `Book` and `User` pointers.
* Acts as the bridge to the file system.
* Responsible for serializing system state to `books.csv` and `users.csv`.
* Deserializes data back into objects when the program boots up.

## How to Run the Project

### Prerequisites

You will need a C++ compiler (like GCC/G++) installed on your machine.

### Compilation

Open your terminal or command prompt, navigate to the folder containing the code, and compile it using:

```bash
g++ library_management.cpp -o library
```

### Execution

#### Windows

```bash
library.exe
```

#### Mac/Linux

```bash
./library
```

## First Run & Data Persistence

When you run the program for the very first time, it will automatically detect that no database files exist and will seed the system with default books and users.

It creates two files in the same directory:

* `books.csv`
* `users.csv`

Any changes you make (borrowing a book, signing up, paying a fine) are instantly saved to these CSV files. If you close the program and reopen it, all your data will be restored perfectly.

## Default Login Credentials

If you just want to test the system out of the box, use these generated accounts:

| Role      | Username | Password |
| --------- | -------- | -------- |
| Student   | vaibhav  | 10001    |
| Faculty   | virat    | 20001    |
| Librarian | ms       | 30001    |
