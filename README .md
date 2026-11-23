# Student Record Management System

A menu-driven console application for managing student records, built with **C++ and Data Structures**.

## Features

- Add, update and delete student records
- Search by roll number, exact name or partial name
- Sort by roll number, CGPA, name or course
- Top-N students by CGPA
- Statistics: average, highest and lowest CGPA, tree height
- Undo the last add, delete or update
- Admission request queue (first in, first out)
- Save and load records using CSV files

## Data Structures Used

| Data Structure | Used For |
|---|---|
| AVL Tree | Primary index by roll number (O(log n) insert, search, delete) |
| Hash Table (chaining) | Fast lookup by student name |
| Stack | Undo feature |
| Queue | Pending admission requests |
| Merge Sort | Sorting records by different fields |

## Project Files

- `student_record_system.cpp` - complete C++ source code

## How to Run

```bash
g++ -std=c++17 student_record_system.cpp -o srms
./srms        # on Windows: .\srms.exe
```

Records are saved to `students.csv` in the same folder.

## Technologies

C++17

## Author

Aradhya Raj, B.Tech
