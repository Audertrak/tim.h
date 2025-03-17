## What It Is

A hierarchical collection of opinionated but extensible types and data structures with access, scope, and memory utilities. 

Each structure contains (where applicable) an array of its children (or pointers to their addresses), a pointer to the parent element, a pointer to the preceding and following elements of the same type, a pointer to the first and last elements of the same type, a record of its current size, a record of its current capacity, its current state: borrower, due date, restriction

The Librarian has a ‘context aware’ arena-style allocator, lock, update, search, sort, resize, and free function for each element type.



## What It Isn't

Necessary. Probably all that good. Purely object oriented. Classes. Methods. Lite



Feature Wish List

An application can have multiple Librarians [co-routines/threads]All state manipulation is performed by a LibrarianElements must be checked out for any operation that affects state [mutex]A Check out event generates a due date [promise]

The librarian responsible for the check out opens a wait list [future]

Elements become readable when ‘checked in’If there exists a due date and/or wait list, on check in the Librarian provides the requested value

Librarians' behavior can be spawned both explicitly (in code) and automatically at runtimeRuntime bounds checking for array accessing/insertion/deletion

Whenever a librarian accesses an element at runtime, it checks the elements ‘utilization’ (current size/current capacity)If the structure is under utilized; sort elements, update relationships, and free the unused the space

If the structure is over utilized; resize the structure, sort the elements, update relationships

(OPTIONAL) Can dynamically create/destroy copies of immutable elements for access by other librarians [multi-threading](EXTREMELY OPTIONAL) Mechanism for using a combination of multiple library features to allow multi-threading (Copy, Check Out, Due Date, Wait List, Check In)

Atomic elements; a book can be created without being added to a shelf, case, etc.The Minimum requirement is to declare a Library, call its allocator, and free it at program end

Bi-directional relational declaration; a book could be added to a shelf by grammatically stating ‘this book belongs to a shelf’ (child→parent) OR ‘this shelf contains a book’ (parent→child)The Librarian performs the ‘placement’ and record updates

Public and Private declarations (global and local scope)Blacklists and Whitelists (granular scope)



Definitions

Library 

A library effectively represents the whole of a single executable's manually mapped/managed memory; contains a top level allocator, a ‘Section’ allocator, an array of pointers to each section, the current total size, the current total capacity, a ‘Section’ free function, a resize function, a sort function, a search function, and a top level free)

Section (effectively representsCaseShelfBookChapterPageParagraphSentencePhraseWordSyllableGlyph

Librarian

The ‘librarian’ interface is responsible for inserting, retrieving, sorting, and deleting data. 
