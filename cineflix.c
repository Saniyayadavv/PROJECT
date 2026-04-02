/* CineFlex - Movie Ticket Booking System (C version) - SAFE VERSION */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 100
#define MAX_TIME_LEN 20

/* Data Structures */
typedef struct Seat {
    int row, col;
    int isBooked;
    char bookedBy[MAX_NAME_LEN];
    struct Seat* next;
} Seat;

typedef struct Row {
    int rowNum;
    Seat* seats;
    struct Row* next;
} Row;

typedef struct QueueNode {
    char userName[MAX_NAME_LEN];
    struct QueueNode* next;
} QueueNode;

typedef struct Show {
    char time[MAX_TIME_LEN];
    double price;
    Row* seatLayout;
    int totalSeats;
    int bookedCount;
    QueueNode* waitingListFront;
    QueueNode* waitingListRear;
    struct Show* next;
} Show;

typedef struct Movie {
    int id;
    char name[MAX_NAME_LEN];
    char genre[MAX_NAME_LEN];
    Show* shows;
    struct Movie* next;
} Movie;

typedef struct Action {
    int type;
    int movieId;
    char showTime[MAX_TIME_LEN];
    int row, col;
    char userName[MAX_NAME_LEN];
    struct Action* next;
} Action;

typedef struct Stack {
    Action* top;
} Stack;

/* Function prototypes */
Seat* createSeat(int row, int col);
Row* createRow(int rowNum, int cols);
Show* createShow(const char* time, double price, int rows, int cols);
Movie* createMovie(int id, const char* name, const char* genre);
void freeMovie(Movie* m);
void freeShow(Show* s);
void freeRow(Row* r);
void freeSeat(Seat* s);
void queuePush(Show* show, const char* userName);
int queuePop(Show* show, char* outUserName);
int queueIsEmpty(Show* show);
void queueClear(Show* show);
void stackInit(Stack* s);
void stackPush(Stack* s, Action act);
int stackPop(Stack* s, Action* outAct);
int stackIsEmpty(Stack* s);
void stackClear(Stack* s);
void addMovie(Movie** head, Stack* undo, Stack* redo);
void deleteMovie(Movie** head, Stack* undo, Stack* redo);
void addShow(Movie* head, Stack* undo, Stack* redo);
void updateSortedMovies(Movie* head, Movie*** sorted, int* count);
Movie* binarySearchMovie(Movie** sorted, int count, const char* name);
int compareMovieNames(const void* a, const void* b);
void listMovies(Movie* head, Movie** sorted, int sortedCount);
void searchMovie(Movie** sorted, int sortedCount);
void bookTicket(Movie* head, Movie** sorted, int sortedCount, Stack* undo, Stack* redo);
void cancelTicket(Movie* head, Movie** sorted, int sortedCount, Stack* undo, Stack* redo);
void undo(Stack* undo, Stack* redo, Movie* head);
void redo(Stack* undo, Stack* redo, Movie* head);
void viewSeatMap(Movie* head, Movie** sorted, int sortedCount);
void showReports(Movie* head);
Show* findShowByTime(Movie* movie, const char* time);
Movie* findMovieById(Movie* head, int id);
void clearInputBuffer(void);

/* Implementations */
Seat* createSeat(int row, int col) {
    Seat* s = (Seat*)malloc(sizeof(Seat));
    if (!s) return NULL;
    s->row = row; s->col = col; s->isBooked = 0;
    s->bookedBy[0] = '\0'; s->next = NULL;
    return s;
}

Row* createRow(int rowNum, int cols) {
    Row* r = (Row*)malloc(sizeof(Row));
    if (!r) return NULL;
    r->rowNum = rowNum; r->next = NULL; r->seats = NULL;
    Seat* tail = NULL;
    for (int c = 1; c <= cols; ++c) {
        Seat* newSeat = createSeat(rowNum, c);
        if (!newSeat) { freeRow(r); return NULL; }
        if (!r->seats) r->seats = newSeat;
        else tail->next = newSeat;
        tail = newSeat;
    }
    return r;
}

Show* createShow(const char* time, double price, int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        printf("Error: rows and columns must be positive.\n");
        return NULL;
    }
    Show* s = (Show*)malloc(sizeof(Show));
    if (!s) return NULL;
    strcpy(s->time, time);
    s->price = price;
    s->totalSeats = rows * cols;
    s->bookedCount = 0;
    s->next = NULL;
    s->seatLayout = NULL;
    s->waitingListFront = s->waitingListRear = NULL;

    Row* tail = NULL;
    for (int r = 1; r <= rows; ++r) {
        Row* newRow = createRow(r, cols);
        if (!newRow) { freeShow(s); return NULL; }
        if (!s->seatLayout) s->seatLayout = newRow;
        else tail->next = newRow;
        tail = newRow;
    }
    return s;
}

Movie* createMovie(int id, const char* name, const char* genre) {
    Movie* m = (Movie*)malloc(sizeof(Movie));
    if (!m) return NULL;
    m->id = id; strcpy(m->name, name); strcpy(m->genre, genre);
    m->shows = NULL; m->next = NULL;
    return m;
}

void freeSeat(Seat* s) { if (!s) return; freeSeat(s->next); free(s); }
void freeRow(Row* r) { if (!r) return; freeRow(r->next); freeSeat(r->seats); free(r); }
void freeShow(Show* s) { if (!s) return; freeShow(s->next); queueClear(s); freeRow(s->seatLayout); free(s); }
void freeMovie(Movie* m) { if (!m) return; freeMovie(m->next); freeShow(m->shows); free(m); }

void queuePush(Show* show, const char* userName) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (!newNode) return;
    strcpy(newNode->userName, userName);
    newNode->next = NULL;
    if (!show->waitingListRear) {
        show->waitingListFront = show->waitingListRear = newNode;
    } else {
        show->waitingListRear->next = newNode;
        show->waitingListRear = newNode;
    }
}

int queuePop(Show* show, char* outUserName) {
    if (!show->waitingListFront) return 0;
    QueueNode* temp = show->waitingListFront;
    strcpy(outUserName, temp->userName);
    show->waitingListFront = temp->next;
    if (!show->waitingListFront) show->waitingListRear = NULL;
    free(temp);
    return 1;
}

int queueIsEmpty(Show* show) { return show->waitingListFront == NULL; }
void queueClear(Show* show) { char dummy[MAX_NAME_LEN]; while (queuePop(show, dummy)); }

void stackInit(Stack* s) { s->top = NULL; }
void stackPush(Stack* s, Action act) {
    Action* newNode = (Action*)malloc(sizeof(Action));
    if (!newNode) return;
    *newNode = act;
    newNode->next = s->top;
    s->top = newNode;
}
int stackPop(Stack* s, Action* outAct) {
    if (!s->top) return 0;
    Action* temp = s->top;
    *outAct = *temp;
    s->top = temp->next;
    free(temp);
    return 1;
}
int stackIsEmpty(Stack* s) { return s->top == NULL; }
void stackClear(Stack* s) { Action dummy; while (stackPop(s, &dummy)); }

void addMovie(Movie** head, Stack* undo, Stack* redo) {
    int id; char name[MAX_NAME_LEN], genre[MAX_NAME_LEN];
    printf("Enter Movie ID: ");
    scanf("%d", &id); clearInputBuffer();
    printf("Enter Movie Name: ");
    fgets(name, MAX_NAME_LEN, stdin); name[strcspn(name, "\n")] = '\0';
    printf("Enter Genre: ");
    fgets(genre, MAX_NAME_LEN, stdin); genre[strcspn(genre, "\n")] = '\0';
    Movie* newMovie = createMovie(id, name, genre);
    if (!newMovie) { printf("Memory allocation failed.\n"); return; }
    if (!*head) *head = newMovie;
    else {
        Movie* temp = *head;
        while (temp->next) temp = temp->next;
        temp->next = newMovie;
    }
    printf("Movie added successfully!\n");
}

void deleteMovie(Movie** head, Stack* undo, Stack* redo) {
    int id; printf("Enter Movie ID to delete: "); scanf("%d", &id); clearInputBuffer();
    if (!*head) { printf("No movies to delete.\n"); return; }
    Movie* prev = NULL; Movie* curr = *head;
    while (curr && curr->id != id) { prev = curr; curr = curr->next; }
    if (!curr) { printf("Movie not found.\n"); return; }
    if (prev) prev->next = curr->next; else *head = curr->next;
    freeMovie(curr);
    printf("Movie deleted successfully.\n");
}

void addShow(Movie* head, Stack* undo, Stack* redo) {
    char movieName[MAX_NAME_LEN];
    printf("Enter Movie Name: ");
    clearInputBuffer();
    fgets(movieName, MAX_NAME_LEN, stdin);
    movieName[strcspn(movieName, "\n")] = '\0';
    Movie* movie = head;
    while (movie && strcmp(movie->name, movieName) != 0) movie = movie->next;
    if (!movie) { printf("Movie not found!\n"); return; }

    char time[MAX_TIME_LEN];
    double price;
    int rows, cols;
    printf("Enter Show Time (e.g., 10:00 AM): ");
    fgets(time, MAX_TIME_LEN, stdin);
    time[strcspn(time, "\n")] = '\0';
    printf("Enter Ticket Price: ");
    scanf("%lf", &price);
    clearInputBuffer();   // important!
    printf("Enter Number of Rows: ");
    scanf("%d", &rows);
    clearInputBuffer();
    printf("Enter Number of Columns: ");
    scanf("%d", &cols);
    clearInputBuffer();

    if (rows <= 0 || cols <= 0) {
        printf("Error: rows and columns must be positive.\n");
        return;
    }

    Show* newShow = createShow(time, price, rows, cols);
    if (!newShow) { printf("Memory allocation failed.\n"); return; }
    if (!movie->shows) movie->shows = newShow;
    else {
        Show* temp = movie->shows;
        while (temp->next) temp = temp->next;
        temp->next = newShow;
    }
    printf("Show added successfully!\n");
}

int compareMovieNames(const void* a, const void* b) {
    Movie* ma = *(Movie**)a; Movie* mb = *(Movie**)b;
    return strcmp(ma->name, mb->name);
}

void updateSortedMovies(Movie* head, Movie*** sorted, int* count) {
    int n = 0; Movie* temp = head;
    while (temp) { n++; temp = temp->next; }
    *count = n;
    if (*sorted) free(*sorted);
    if (n == 0) { *sorted = NULL; return; }
    *sorted = (Movie**)malloc(n * sizeof(Movie*));
    if (!*sorted) { *count = 0; return; }
    temp = head;
    for (int i = 0; i < n; ++i) { (*sorted)[i] = temp; temp = temp->next; }
    qsort(*sorted, n, sizeof(Movie*), compareMovieNames);
}

Movie* binarySearchMovie(Movie** sorted, int count, const char* name) {
    int left = 0, right = count - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(sorted[mid]->name, name);
        if (cmp == 0) return sorted[mid];
        else if (cmp < 0) left = mid + 1;
        else right = mid - 1;
    }
    return NULL;
}

Show* findShowByTime(Movie* movie, const char* time) {
    Show* curr = movie->shows;
    while (curr) {
        if (strcmp(curr->time, time) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

Movie* findMovieById(Movie* head, int id) {
    while (head) {
        if (head->id == id) return head;
        head = head->next;
    }
    return NULL;
}

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void listMovies(Movie* head, Movie** sorted, int sortedCount) {
    if (sortedCount == 0) { printf("No movies available.\n"); return; }
    printf("\nMovies (sorted by name):\n");
    for (int i = 0; i < sortedCount; ++i) {
        printf("%d. %s (ID: %d, Genre: %s)\n", i+1, sorted[i]->name, sorted[i]->id, sorted[i]->genre);
    }
}

void searchMovie(Movie** sorted, int sortedCount) {
    char name[MAX_NAME_LEN];
    printf("Enter movie name: ");
    clearInputBuffer();
    fgets(name, MAX_NAME_LEN, stdin); name[strcspn(name, "\n")] = '\0';
    Movie* m = binarySearchMovie(sorted, sortedCount, name);
    if (m) {
        printf("Movie found!\nID: %d\nName: %s\nGenre: %s\n", m->id, m->name, m->genre);
        printf("Shows:\n");
        Show* s = m->shows;
        if (!s) printf("  No shows scheduled.\n");
        while (s) {
            printf("  %s - Price: %.2f\n", s->time, s->price);
            s = s->next;
        }
    } else printf("Movie not found.\n");
}

void bookTicket(Movie* head, Movie** sorted, int sortedCount, Stack* undo, Stack* redo) {
    char movieName[MAX_NAME_LEN], userName[MAX_NAME_LEN], showTime[MAX_TIME_LEN];
    int row, col;
    printf("Enter your name: ");
    clearInputBuffer();
    fgets(userName, MAX_NAME_LEN, stdin); userName[strcspn(userName, "\n")] = '\0';
    printf("Enter Movie Name: ");
    fgets(movieName, MAX_NAME_LEN, stdin); movieName[strcspn(movieName, "\n")] = '\0';
    Movie* movie = binarySearchMovie(sorted, sortedCount, movieName);
    if (!movie) { printf("Movie not found!\n"); return; }

    printf("Available Shows:\n");
    Show* showPtr = movie->shows;
    int idx = 1;
    while (showPtr) {
        printf("%d. %s (Price: %.2f)\n", idx++, showPtr->time, showPtr->price);
        showPtr = showPtr->next;
    }
    printf("Enter Show Time: ");
    fgets(showTime, MAX_TIME_LEN, stdin); showTime[strcspn(showTime, "\n")] = '\0';
    Show* show = findShowByTime(movie, showTime);
    if (!show) { printf("Show not found!\n"); return; }

    printf("Seat Map:\n");
    Row* r = show->seatLayout;
    while (r) {
        printf("Row %d: ", r->rowNum);
        Seat* s = r->seats;
        while (s) {
            if (s->isBooked) printf("[X] ");
            else printf("[ ] ");
            s = s->next;
        }
        printf("\n");
        r = r->next;
    }

    printf("Enter row and column to book: ");
    scanf("%d %d", &row, &col); clearInputBuffer();
    if (row < 1 || col < 1) { printf("Invalid seat coordinates.\n"); return; }

    Row* rowPtr = show->seatLayout;
    while (rowPtr && rowPtr->rowNum != row) rowPtr = rowPtr->next;
    if (!rowPtr) { printf("Invalid row number.\n"); return; }
    Seat* seatPtr = rowPtr->seats;
    while (seatPtr && seatPtr->col != col) seatPtr = seatPtr->next;
    if (!seatPtr) { printf("Invalid column number.\n"); return; }

    if (seatPtr->isBooked) { printf("Seat already booked!\n"); return; }

    if (show->bookedCount == show->totalSeats) {
        queuePush(show, userName);
        int pos = 0; QueueNode* q = show->waitingListFront;
        while (q) { pos++; q = q->next; }
        printf("Show is full. You have been added to the waiting list. Position: %d\n", pos);
        return;
    }

    seatPtr->isBooked = 1;
    strcpy(seatPtr->bookedBy, userName);
    show->bookedCount++;

    Action act; act.type = 1; act.movieId = movie->id; strcpy(act.showTime, show->time);
    act.row = row; act.col = col; strcpy(act.userName, userName);
    stackPush(undo, act); stackClear(redo);
    printf("Seat booked successfully!\n");
}

void cancelTicket(Movie* head, Movie** sorted, int sortedCount, Stack* undo, Stack* redo) {
    char movieName[MAX_NAME_LEN], userName[MAX_NAME_LEN], showTime[MAX_TIME_LEN];
    int row, col;
    printf("Enter your name: ");
    clearInputBuffer();
    fgets(userName, MAX_NAME_LEN, stdin); userName[strcspn(userName, "\n")] = '\0';
    printf("Enter Movie Name: ");
    fgets(movieName, MAX_NAME_LEN, stdin); movieName[strcspn(movieName, "\n")] = '\0';
    Movie* movie = binarySearchMovie(sorted, sortedCount, movieName);
    if (!movie) { printf("Movie not found!\n"); return; }

    printf("Shows:\n");
    Show* showPtr = movie->shows;
    while (showPtr) { printf("- %s\n", showPtr->time); showPtr = showPtr->next; }
    printf("Enter Show Time: ");
    fgets(showTime, MAX_TIME_LEN, stdin); showTime[strcspn(showTime, "\n")] = '\0';
    Show* show = findShowByTime(movie, showTime);
    if (!show) { printf("Show not found!\n"); return; }

    printf("Enter row and column of the seat to cancel: ");
    scanf("%d %d", &row, &col); clearInputBuffer();

    Row* rowPtr = show->seatLayout;
    while (rowPtr && rowPtr->rowNum != row) rowPtr = rowPtr->next;
    if (!rowPtr) { printf("Invalid row.\n"); return; }
    Seat* seatPtr = rowPtr->seats;
    while (seatPtr && seatPtr->col != col) seatPtr = seatPtr->next;
    if (!seatPtr) { printf("Invalid column.\n"); return; }

    if (!seatPtr->isBooked || strcmp(seatPtr->bookedBy, userName) != 0) {
        printf("Either seat is not booked or you didn't book it.\n");
        return;
    }

    seatPtr->isBooked = 0; seatPtr->bookedBy[0] = '\0'; show->bookedCount--;
    Action act; act.type = 2; act.movieId = movie->id; strcpy(act.showTime, show->time);
    act.row = row; act.col = col; strcpy(act.userName, userName);
    stackPush(undo, act); stackClear(redo);

    if (!queueIsEmpty(show)) {
        char nextUser[MAX_NAME_LEN]; queuePop(show, nextUser);
        seatPtr->isBooked = 1; strcpy(seatPtr->bookedBy, nextUser); show->bookedCount++;
        printf("Seat assigned to %s from waiting list.\n", nextUser);
    }
    printf("Seat cancelled successfully!\n");
}

void undo(Stack* undo, Stack* redo, Movie* head) {
    Action act;
    if (!stackPop(undo, &act)) { printf("Nothing to undo.\n"); return; }
    Movie* movie = findMovieById(head, act.movieId);
    if (!movie) { printf("Error: Movie not found for undo.\n"); return; }
    Show* show = findShowByTime(movie, act.showTime);
    if (!show) { printf("Error: Show not found for undo.\n"); return; }
    Row* rowPtr = show->seatLayout;
    while (rowPtr && rowPtr->rowNum != act.row) rowPtr = rowPtr->next;
    if (!rowPtr) { printf("Error: Row not found for undo.\n"); return; }
    Seat* seatPtr = rowPtr->seats;
    while (seatPtr && seatPtr->col != act.col) seatPtr = seatPtr->next;
    if (!seatPtr) { printf("Error: Seat not found for undo.\n"); return; }

    if (act.type == 1) {
        if (seatPtr->isBooked) {
            seatPtr->isBooked = 0; seatPtr->bookedBy[0] = '\0'; show->bookedCount--;
            printf("Undo: Booking cancelled.\n");
        } else printf("Error: Seat was not booked, cannot undo.\n");
    } else {
        if (!seatPtr->isBooked) {
            seatPtr->isBooked = 1; strcpy(seatPtr->bookedBy, act.userName); show->bookedCount++;
            printf("Undo: Cancellation reverted.\n");
        } else printf("Error: Seat was already booked, cannot undo cancellation.\n");
    }
    stackPush(redo, act);
}

void redo(Stack* undo, Stack* redo, Movie* head) {
    Action act;
    if (!stackPop(redo, &act)) { printf("Nothing to redo.\n"); return; }
    Movie* movie = findMovieById(head, act.movieId);
    if (!movie) { printf("Error: Movie not found for redo.\n"); return; }
    Show* show = findShowByTime(movie, act.showTime);
    if (!show) { printf("Error: Show not found for redo.\n"); return; }
    Row* rowPtr = show->seatLayout;
    while (rowPtr && rowPtr->rowNum != act.row) rowPtr = rowPtr->next;
    if (!rowPtr) { printf("Error: Row not found for redo.\n"); return; }
    Seat* seatPtr = rowPtr->seats;
    while (seatPtr && seatPtr->col != act.col) seatPtr = seatPtr->next;
    if (!seatPtr) { printf("Error: Seat not found for redo.\n"); return; }

    if (act.type == 1) {
        if (!seatPtr->isBooked) {
            seatPtr->isBooked = 1; strcpy(seatPtr->bookedBy, act.userName); show->bookedCount++;
            printf("Redo: Booking reapplied.\n");
        } else printf("Seat already booked, cannot redo.\n");
    } else {
        if (seatPtr->isBooked) {
            seatPtr->isBooked = 0; seatPtr->bookedBy[0] = '\0'; show->bookedCount--;
            printf("Redo: Cancellation reapplied.\n");
        } else printf("Seat already free, cannot redo cancellation.\n");
    }
    stackPush(undo, act);
}

void viewSeatMap(Movie* head, Movie** sorted, int sortedCount) {
    char movieName[MAX_NAME_LEN], showTime[MAX_TIME_LEN];
    printf("Enter Movie Name: ");
    clearInputBuffer();
    fgets(movieName, MAX_NAME_LEN, stdin); movieName[strcspn(movieName, "\n")] = '\0';
    Movie* movie = binarySearchMovie(sorted, sortedCount, movieName);
    if (!movie) { printf("Movie not found!\n"); return; }
    printf("Shows:\n");
    Show* s = movie->shows;
    while (s) { printf("- %s\n", s->time); s = s->next; }
    printf("Enter Show Time: ");
    fgets(showTime, MAX_TIME_LEN, stdin); showTime[strcspn(showTime, "\n")] = '\0';
    Show* show = findShowByTime(movie, showTime);
    if (!show) { printf("Show not found.\n"); return; }
    printf("Seat Map:\n");
    Row* r = show->seatLayout;
    while (r) {
        printf("Row %d: ", r->rowNum);
        Seat* seat = r->seats;
        while (seat) {
            if (seat->isBooked) printf("[X] ");
            else printf("[ ] ");
            seat = seat->next;
        }
        printf("\n");
        r = r->next;
    }
}

void showReports(Movie* head) {
    if (!head) { printf("No data available.\n"); return; }
    Movie* popularMovie = NULL; int maxBookings = -1; double totalRevenue = 0.0;
    Movie* curr = head;
    while (curr) {
        int movieBookings = 0;
        Show* show = curr->shows;
        while (show) {
            movieBookings += show->bookedCount;
            totalRevenue += show->bookedCount * show->price;
            show = show->next;
        }
        if (movieBookings > maxBookings) {
            maxBookings = movieBookings;
            popularMovie = curr;
        }
        curr = curr->next;
    }
    printf("\n=== Reports ===\n");
    if (popularMovie) printf("Most Popular Movie: %s (Total Bookings: %d)\n", popularMovie->name, maxBookings);
    printf("Total Revenue: $%.2f\n", totalRevenue);
}

int main() {
    Movie* head = NULL;
    Stack undoStack, redoStack;
    stackInit(&undoStack);
    stackInit(&redoStack);
    Movie** sortedMovies = NULL;
    int sortedCount = 0;

    int choice;
    do {
        printf("\n================== CineFlex ==================\n");
        printf("1. Admin Mode\n");
        printf("2. User Mode\n");
        printf("3. Reports\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        clearInputBuffer();

        switch (choice) {
            case 1: {
                int adminChoice;
                do {
                    printf("\n--- Admin Menu ---\n");
                    printf("1. Add Movie\n");
                    printf("2. Delete Movie\n");
                    printf("3. Add Show to Movie\n");
                    printf("4. Back to Main\n");
                    printf("Enter choice: ");
                    scanf("%d", &adminChoice);
                    clearInputBuffer();
                    switch (adminChoice) {
                        case 1: addMovie(&head, &undoStack, &redoStack); updateSortedMovies(head, &sortedMovies, &sortedCount); break;
                        case 2: deleteMovie(&head, &undoStack, &redoStack); updateSortedMovies(head, &sortedMovies, &sortedCount); break;
                        case 3: addShow(head, &undoStack, &redoStack); break;
                        case 4: break;
                        default: printf("Invalid choice.\n");
                    }
                } while (adminChoice != 4);
                break;
            }
            case 2: {
                int userChoice;
                do {
                    printf("\n--- User Menu ---\n");
                    printf("1. List All Movies\n");
                    printf("2. Search Movie by Name\n");
                    printf("3. Book Ticket\n");
                    printf("4. Cancel Ticket\n");
                    printf("5. Undo Last Action\n");
                    printf("6. Redo\n");
                    printf("7. View Seat Map\n");
                    printf("8. Back to Main\n");
                    printf("Enter choice: ");
                    scanf("%d", &userChoice);
                    clearInputBuffer();
                    switch (userChoice) {
                        case 1: listMovies(head, sortedMovies, sortedCount); break;
                        case 2: searchMovie(sortedMovies, sortedCount); break;
                        case 3: bookTicket(head, sortedMovies, sortedCount, &undoStack, &redoStack); break;
                        case 4: cancelTicket(head, sortedMovies, sortedCount, &undoStack, &redoStack); break;
                        case 5: undo(&undoStack, &redoStack, head); break;
                        case 6: redo(&undoStack, &redoStack, head); break;
                        case 7: viewSeatMap(head, sortedMovies, sortedCount); break;
                        case 8: break;
                        default: printf("Invalid choice.\n");
                    }
                } while (userChoice != 8);
                break;
            }
            case 3: showReports(head); break;
            case 4: printf("Thank you for using CineFlex. Goodbye!\n"); break;
            default: printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 4);

    freeMovie(head);
    free(sortedMovies);
    stackClear(&undoStack);
    stackClear(&redoStack);
    return 0;
}
