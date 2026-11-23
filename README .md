# Student Record Management System

A student record management system built with **C++ and Data Structures**, with a C++ console backend and a browser-based frontend.

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

- `student_record_system.cpp` - C++ console backend
- `student_record_frontend.html` - browser frontend (same logic, written in JavaScript)

## How to Run

### Backend (C++)

```bash
g++ -std=c++17 student_record_system.cpp -o srms
./srms        # on Windows: .\srms.exe
```

Records are saved to `students.csv` in the same folder.

### Frontend

Open `student_record_frontend.html` in any web browser. No installation needed. Data is stored in the browser, and you can import or export CSV files compatible with the C++ backend.

## Technologies

C++17, HTML, CSS, JavaScript

## Author

ARADHYA RAJ, B.Tech
