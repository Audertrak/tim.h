/* What It Is
 *		A collection of opinionated but extensible types and data
 *structures along with access, scope, and memory utilities.
 *
 * What It Isn't
 *		Necessary. Probably all that good. Purely object oriented.
 *Classes. Methods. Lite
 *
 * Main Concept: Library
 *		A library effectively represents the whole of a single
 *executable's manually mapped/managed memory.
 *
 *		Each library structure contains (where applicable)...
 *			- an array of its children (or pointers to their
 *addresses)
 *			- a pointer to the parent element
 *			- a pointer to the preceding and following elements of
 *the same type
 *			- a pointer to the first and last elements of the same
 *type
 *			- a record of its current size, a record of its current
 *capacity
 *			- its current state:
 *				* borrower
 *				* due date
 *				* restriction
 *
 * Main Concept: Librarian
 *		Librarians are the 'actors' that perform ajax-like operations on
 *the library and its contents.
 *
 *		Librarians have the following functionality
 *			- ‘context aware’ arena-style allocators
 *			- lock
 *			- update
 *			- search
 *			- sort
 *			- resize
 *			- free
 *
 *
 *	Feature Wish List
 *		- An application can have multiple Librarians
 *[co-routines/threads]
 *			- All state manipulation is performed by a Librarian
 *			- Elements must be checked out for any operation that
 *affects state [mutex]
 *				* A Check out event generates a due date
 *[promise]
 *				* The librarian responsible for the check out
 *opens a wait list [future]
 *				* Elements become readable when ‘checked in’
 *				* If there exists a due date and/or wait list,
 *on check in the Librarian provides the requested value
 *			- Librarians' behavior can be spawned both explicitly
 *(in code) and automatically at runtime
 *				* Runtime bounds checking for array
 *accessing/insertion/deletion
 *				* Whenever a librarian accesses an element at
 *runtime, it checks the elements ‘utilization’ (current size/current capacity)
 *					- If the structure is under utilized
 *						* sort elements
 *						* update relationships
 *						* free unused the space
 *					- If the structure is over utilized
 *						* resize the structure
 *						* sort the elements
 *						* update relationships
 *				* (OPTIONAL) Can dynamically create/destroy
 *copies of immutable elements for access by other librarians [multi-threading]
 *				* (EXTREMELY OPTIONAL) Mechanism for using a
 *combination of multiple library features to allow multi-threading (Copy, Check
 *Out, Due Date, Wait List, Check In)
 *		- Atomic elements; a book can be created without being added to
 *a shelf, case, etc.
 *			* The Minimum requirement is to declare a Library, call
 *its allocator, and free it at program end
 *		- Bi-directional relational declaration; a book could be added
 *to a shelf by grammatically stating ‘this book belongs to a shelf’
 *(child→parent) OR ‘this shelf contains a book’ (parent→child)
 *			* The Librarian performs the ‘placement’ and record
 *updates
 *		- Public and Private declarations (global and local scope)
 *			* Blacklists and Whitelists (granular scope)
 *
 *	Hierarchy
 *		Library
 *			- Case
 *				- Shelf
 *					- Book
 *						- Chapter
 *							- Page
 *								- Paragraph
 *									-
 *Sentence -	Phrase -	Word -	Syllable -	Glyph Librarian
 *
 */
