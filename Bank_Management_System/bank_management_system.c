#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

// Disable echo for Unix-like systems
void set_echo(int enable)
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);

    if (!enable)
        tty.c_lflag &= ~(ECHO);
    else
        tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}
#endif

#define SESSION_TIMEOUT 300 // Session timeout in seconds (5 minutes)

int login_failed_attempts = 0;
char failed_login_username[20] = "";
int withdrawal_count = 0;
time_t first_withdrawal_time = 0;
float daily_withdrawal_total = 0;
int current_day = -1;

// ======== STRUCTS WITH TYPEDEF ========
typedef struct
{
    char username[20];
    char password[20];
    char role[10];
    char status[10]; // "active" or "suspended"
} User;

typedef struct
{
    int day;
    int month;
    int year;
} Date;

typedef struct
{
    int acc_no;
    char username[20]; // links to User
    char name[20];
    Date date_of_birth;
    char address[45];
    char aadhar_no[13];
    char email_id[30];
    char phone_no[11];
    char occupation[30];
    int income; // changed from int[10] to int
    float balance;
    char acc_type[10]; // "Savings" Account
} BankAccount;

typedef struct
{
    int transaction_id;
    int acc_no;         // Account whose statement this belongs to
    int related_acc_no; // Other account involved
    char type;          // O = Opening Balance, D = Deposit, W = Withdraw, T = Transfer
    char direction[10]; // DEBIT / CREDIT
    float amount;
    float balance_after; // Balance after transaction
    char timestamp[30];
} Transaction;

typedef struct
{
    int loan_id;
    char username[20];
    float amount;
    char status[10]; // "pending", "approved", "rejected"
} Loan;

// ======== UTILITIES ========
void get_timestamp(char *buffer)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, 25, "%Y-%m-%d %H:%M", tm_info);
}

int generate_transaction_id()
{
    FILE *fp = fopen("transactions.dat", "rb");
    Transaction t;
    int transaction_id, exists;

    do
    {
        exists = 0;
        transaction_id = rand() % 9000 + 1000;

        if (fp)
        {
            rewind(fp);
            while (fread(&t, sizeof(t), 1, fp))
            {
                if (t.transaction_id == transaction_id)
                {
                    exists = 1;
                    break;
                }
            }
        }
    } while (exists);

    if (fp)
        fclose(fp);
    return transaction_id;
}

void record_transaction(int acc_no,
                        int related_acc_no,
                        char type,
                        const char *direction,
                        float amount,
                        float balance_after)
{
    Transaction tr;
    FILE *fp = fopen("transactions.dat", "ab");

    if (!fp)
    {
        printf("Error opening transactions file.\n");
        return;
    }

    // Generate transaction ID
    tr.transaction_id = generate_transaction_id();

    // Account whose statement this transaction belongs to
    tr.acc_no = acc_no;

    // Other account involved in transaction
    // For deposit/withdrawal/opening, use -1
    tr.related_acc_no = related_acc_no;

    // Transaction type
    tr.type = type;

    // DEBIT or CREDIT
    strcpy(tr.direction, direction);

    // Transaction amount
    tr.amount = amount;

    // Balance after transaction
    tr.balance_after = balance_after;

    // Timestamp
    get_timestamp(tr.timestamp);

    fwrite(&tr, sizeof(Transaction), 1, fp);

    fclose(fp);
}

void log_action(const char *role, const char *owner, const char *action)
{
    FILE *log = fopen("system_logs.txt", "a");

    if (!log)
    {
        perror("LOG ERROR");
        return;
    }

    char timestamp[25];
    get_timestamp(timestamp);

    int written = fprintf(log, "[%s] [%s] [%s] %s\n", timestamp, role, owner, action);

    fflush(log);
    fclose(log);
}

// ======== PASSWORD MASKING =======
void read_password(char *password, int max_len)
{
    int i = 0;
    char ch;

#if defined(_WIN32) || defined(_WIN64)
    while (1)
    {
        ch = _getch();
        if (ch == '\r' || ch == '\n')
        {
            password[i] = '\0';
            printf("\n");
            break;
        }
        else if (ch == '\b')
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
        }
        else if (i < max_len - 1)
        {
            password[i++] = ch;
            printf("*");
        }
    }
#else
    set_echo(0); // disable echo

    while (1)
    {
        ch = getchar();
        if (ch == '\n' || ch == '\r')
        {
            password[i] = '\0';
            printf("\n");
            break;
        }
        else if (ch == 127 || ch == '\b')
        { // handle backspace
            if (i > 0)
            {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
        }
        else if (i < max_len - 1)
        {
            password[i++] = ch;
            printf("*");
            fflush(stdout);
        }
    }

    set_echo(1); // enable echo back
#endif
}

int generate_account_number()
{
    FILE *fp = fopen("accounts.dat", "rb");
    BankAccount acc;
    int acc_no, exists;

    do
    {
        exists = 0;
        acc_no = rand() % 9000 + 1000;

        if (fp)
        {
            rewind(fp);
            while (fread(&acc, sizeof(acc), 1, fp))
            {
                if (acc.acc_no == acc_no)
                {
                    exists = 1;
                    break;
                }
            }
        }
    } while (exists);

    if (fp)
        fclose(fp);
    return acc_no;
}

int generate_loan_id()
{
    FILE *fp = fopen("loans.dat", "rb");
    Loan l;
    int loan_id, exists;

    do
    {
        exists = 0;
        loan_id = rand() % 9000 + 1000;

        if (fp)
        {
            rewind(fp);
            while (fread(&l, sizeof(l), 1, fp))
            {
                if (l.loan_id == loan_id)
                {
                    exists = 1;
                    break;
                }
            }
        }
    } while (exists);

    if (fp)
        fclose(fp);
    return loan_id;
}

void fraud_alert(const char *username, const char *message)
{
    FILE *fp = fopen("fraud_alert_logs.txt", "a");

    if (!fp)
    {
        printf("Error opening fraud log file.\n");
        return;
    }

    char timestamp[25];
    get_timestamp(timestamp);

    fprintf(fp, "[%s] USER: %s | FRAUD ALERT: %s\n", timestamp, username, message);

    fclose(fp);
}

// ======== AUTH SYSTEM ========
int register_user()
{
    User u;
    FILE *fp = fopen("users.dat", "ab+"); // open for reading and appending
    if (!fp)
    {
        // If file doesn't exist, create it
        fp = fopen("users.dat", "wb+");
        if (!fp)
        {
            printf("Error opening users file.\n");
            return 0;
        }
    }

    printf("Choose username: ");
    scanf("%19s", u.username);

    // Check if username already exists
    User temp;
    rewind(fp); // go to the start of the file
    while (fread(&temp, sizeof(User), 1, fp))
    {
        if (strcmp(temp.username, u.username) == 0)
        {
            printf("Username '%s' already exists. Please choose another.\n", u.username);
            fclose(fp);
            return 0; // registration failed
        }
    }

    /* password validation */
    while (1)
    {
        int has_digit = 0;
        int has_special = 0;
        int len = 0;

        printf("Choose password (8-12 chars, at least 1 number, at least 1 special char): ");
        read_password(u.password, 20);

        len = strlen(u.password);

        for (int i = 0; i < len; i++)
        {
            if (isdigit(u.password[i]))
                has_digit = 1;

            else if (!isalnum(u.password[i]))
                has_special = 1;
        }

        if (len < 8)
        {
            printf("Password must be at least 8 characters.\n");
        }
        else if (len > 12)
        {
            printf("Password must not exceed 12 characters.\n");
        }
        else if (!has_digit)
        {
            printf("Password must contain at least one number.\n");
        }
        else if (!has_special)
        {
            printf("Password must contain at least one special character.\n");
        }
        else
        {
            break;
        }
    }

    strcpy(u.role, "USER");
    strcpy(u.status, "active");

    // Append the new user
    fseek(fp, 0, SEEK_END);
    fwrite(&u, sizeof(User), 1, fp);

    log_action("USER", u.username, "User registered");

    fclose(fp);

    printf("User registered successfully!\n");
    return 1;
}

int login(char *username, char *role, int isAdminLogin)
{
    User u;
    char input_user[20], input_pass[20];
    FILE *fp = fopen("users.dat", "rb");

    if (!fp)
    {
        printf("No user found. Please register first.\n");
        return 0;
    }

    printf("Username: ");
    scanf("%19s", input_user);

    printf("Password: ");
    read_password(input_pass, 20);

    while (fread(&u, sizeof(User), 1, fp))
    {
        u.username[sizeof(u.username) - 1] = '\0';
        u.password[sizeof(u.password) - 1] = '\0';
        u.role[sizeof(u.role) - 1] = '\0';
        u.status[sizeof(u.status) - 1] = '\0';

        if (strcmp(u.username, input_user) == 0 &&
            strcmp(u.password, input_pass) == 0)
        {
            if (!isAdminLogin && _stricmp(u.role, "admin") == 0)
            {
                printf("Admin cannot login through User Login.\n");
                fclose(fp);
                return 0;
            }

            if (strcmp(u.status, "suspended") == 0)
            {
                printf("Your Account is suspended.\n");
                fclose(fp);
                return 0;
            }

            /* Successful login: reset consecutive failed attempts */
            login_failed_attempts = 0;
            failed_login_username[0] = '\0';

            strcpy(username, u.username);
            strcpy(role, u.role);

            printf("Login successful!! Username: %s, Role: %s\n",
                   username, role);

            if (_stricmp(role, "admin") == 0)
                log_action("ADMIN", username, "Login successful");
            else
                log_action("USER", username, "Login successful");

            fclose(fp);
            return 1;
        }
    }

    fclose(fp);

    /* Count failures only for consecutive attempts with the same username */
    if (strcmp(failed_login_username, input_user) == 0)
    {
        login_failed_attempts++;
    }
    else
    {
        strcpy(failed_login_username, input_user);
        login_failed_attempts = 1;
    }

    if (login_failed_attempts >= 3)
    {
        printf("\n=========================================\n");
        printf("FRAUD DETECTION ALERT\n");
        printf("Three consecutive failed login attempts\n");
        printf("detected for username: %s\n", input_user);
        printf("=========================================\n");

        char fraud_msg[150];
        snprintf(fraud_msg, sizeof(fraud_msg),
                 "Multiple (3) failed login attempts for username: %s",
                 input_user);

        fraud_alert(input_user, fraud_msg);

        /* Reset so another alert requires three new consecutive failures */
        login_failed_attempts = 0;
        failed_login_username[0] = '\0';
    }

    if (strcmp(input_user, "admin") == 0)
        log_action("ADMIN", input_user, "Login failed");
    else
        log_action("USER", input_user, "Login failed");

    printf("Invalid credentials.\n");
    return 0;
}

// ======== ADMIN ACCOUNT CREATION ========
void create_admin_account()
{
    FILE *fp = fopen("users.dat", "rb+");
    User u;
    int found = 0;

    if (fp)
    {
        while (fread(&u, sizeof(User), 1, fp))
        {
            if (strcmp(u.username, "admin") == 0)
            {
                found = 1;
                // Reset password and ensure correct role and status
                strcpy(u.password, "admin123");
                strncpy(u.role, "admin", sizeof(u.role) - 1);
                u.role[sizeof(u.role) - 1] = '\0';
                strncpy(u.status, "active", sizeof(u.status) - 1);
                u.status[sizeof(u.status) - 1] = '\0';

                fseek(fp, -sizeof(User), SEEK_CUR);
                fwrite(&u, sizeof(User), 1, fp);
                fclose(fp);
                printf("Admin account has been reset successfully.\n");
                return;
            }
        }
        fclose(fp);
    }

    // If not found, create fresh admin
    fp = fopen("users.dat", "ab");
    if (!fp)
    {
        printf("Error creating admin account.\n");
        return;
    }

    strcpy(u.username, "admin");
    strcpy(u.password, "admin123");
    strncpy(u.role, "admin", sizeof(u.role) - 1);
    u.role[sizeof(u.role) - 1] = '\0';
    strncpy(u.status, "active", sizeof(u.status) - 1);
    u.status[sizeof(u.status) - 1] = '\0';

    fwrite(&u, sizeof(User), 1, fp);
    fclose(fp);
    printf("Admin account created successfully.\n");
}

// ======== BANKING OPERATIONS ========
void create_account(const char *admin, int is_admin)
{
    BankAccount acc;
    FILE *fp_acc, *fp_user, *fp_trans;
    User u;
    char username[20];
    int exists = 0, registered = 0;

    printf("Enter the username of the customer: ");
    scanf("%19s", username);

    // Step 1: Verify if user is registered
    fp_user = fopen("users.dat", "rb");
    if (!fp_user)
    {
        printf("No registered users found. Please register first.\n");
        log_action("ADMIN", admin, "Account creation failed - no users.dat found");
        return;
    }
    while (fread(&u, sizeof(User), 1, fp_user))
    {
        if (strcmp(u.username, username) == 0)
        {
            registered = 1;
            break;
        }
    }
    fclose(fp_user);

    if (!registered)
    {
        printf("This username is not registered. Ask the user to register first.\n");
        return;
    }

    // Step 2: Check if account already exists
    fp_acc = fopen("accounts.dat", "rb");
    if (fp_acc)
    {
        while (fread(&acc, sizeof(BankAccount), 1, fp_acc))
        {
            if (strcmp(acc.username, username) == 0)
            {
                exists = 1;
                break;
            }
        }
        fclose(fp_acc);
    }

    if (exists)
    {
        printf("This user already has a Savings Account. Only one account per user is allowed.\n");
        return;
    }

    // Step 3: Create new Savings Account
    fp_acc = fopen("accounts.dat", "ab");
    if (!fp_acc)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    acc.acc_no = generate_account_number();
    strcpy(acc.username, username);
    strcpy(acc.acc_type, "Savings"); // always Savings

    getchar(); // clear buffer
    printf("Enter Account Holder Name: ");
    fgets(acc.name, sizeof(acc.name), stdin);
    acc.name[strcspn(acc.name, "\n")] = 0;

    // DOB validation
    int valid_dob = 0;

    time_t now = time(NULL);
    struct tm *current = localtime(&now);

    do
    {
        printf("Enter DOB (DD M YYYY Format): ");
        scanf("%d %d %d",
              &acc.date_of_birth.day,
              &acc.date_of_birth.month,
              &acc.date_of_birth.year);

        int d = acc.date_of_birth.day;
        int m = acc.date_of_birth.month;
        int y = acc.date_of_birth.year;

        // Basic validation
        if (y < 1900 || m < 1 || m > 12 || d < 1 || d > 31)
        {
            printf("Invalid date of birth.\n");
            continue;
        }

        // Days in each month
        int days_in_month[] =
            {
                31, 28, 31, 30, 31, 30,
                31, 31, 30, 31, 30, 31};

        // Leap year check
        if ((y % 400 == 0) || (y % 4 == 0 && y % 100 != 0))
        {
            days_in_month[1] = 29;
        }

        if (d > days_in_month[m - 1])
        {
            printf("Invalid date of birth.\n");
            continue;
        }

        // Future date check
        if (y > current->tm_year + 1900 ||
            (y == current->tm_year + 1900 &&
             m > current->tm_mon + 1) ||
            (y == current->tm_year + 1900 &&
             m == current->tm_mon + 1 &&
             d > current->tm_mday))
        {
            printf("Date of birth cannot be in the future.\n");
            continue;
        }

        // Calculate age
        int age = (current->tm_year + 1900) - y;

        if (m > current->tm_mon + 1 ||
            (m == current->tm_mon + 1 &&
             d > current->tm_mday))
        {
            age--;
        }

        if (age < 18)
        {
            printf("Customer must be at least 18 years old.\n");
            continue;
        }

        valid_dob = 1;

    } while (!valid_dob);

    getchar();
    printf("Enter Address: ");
    fgets(acc.address, sizeof(acc.address), stdin);
    acc.address[strcspn(acc.address, "\n")] = 0;

    // Aadhaar Number validation and duplicate check
    do
    {
        printf("Enter Aadhar number (12 digits): ");
        scanf("%12s", acc.aadhar_no);

        int valid = 1;
        int duplicate = 0;

        // Must contain exactly 12 digits
        if (strlen(acc.aadhar_no) != 12)
        {
            valid = 0;
        }
        else
        {
            for (int i = 0; i < 12; i++)
            {
                if (!isdigit((unsigned char)acc.aadhar_no[i]))
                {
                    valid = 0;
                    break;
                }
            }
        }

        if (!valid)
        {
            printf("Invalid Aadhaar number. Enter exactly 12 digits.\n");
            continue;
        }

        // Check whether Aadhaar number already exists
        FILE *check_fp = fopen("accounts.dat", "rb");

        if (check_fp != NULL)
        {
            BankAccount existing;

            while (fread(&existing, sizeof(BankAccount), 1, check_fp))
            {
                if (strcmp(existing.aadhar_no, acc.aadhar_no) == 0)
                {
                    duplicate = 1;
                    break;
                }
            }

            fclose(check_fp);
        }

        if (duplicate)
        {
            printf("Error: This Aadhaar number is already registered.\n");

            fclose(fp_acc);
            return;
        }
        break;
    } while (1);

    // Phone Number validation and duplicate check
    do
    {
        printf("Enter Phone number (10 digits): ");
        scanf("%10s", acc.phone_no);

        int valid = 1;
        int duplicate = 0;

        // Must contain exactly 10 digits
        if (strlen(acc.phone_no) != 10)
        {
            valid = 0;
        }
        // First digit must be 6, 7, 8 or 9
        else if (acc.phone_no[0] != '6' &&
                 acc.phone_no[0] != '7' &&
                 acc.phone_no[0] != '8' &&
                 acc.phone_no[0] != '9')
        {
            valid = 0;
        }
        // Check that all characters are digits
        else
        {
            for (int i = 0; i < 10; i++)
            {
                if (!isdigit((unsigned char)acc.phone_no[i]))
                {
                    valid = 0;
                    break;
                }
            }
        }

        if (!valid)
        {
            printf("Invalid phone number.\n");
            printf("Phone number must be exactly 10 digits and start with 6, 7, 8 or 9.\n");
            continue;
        }

        // Check whether phone number already exists
        FILE *check_fp = fopen("accounts.dat", "rb");

        if (check_fp != NULL)
        {
            BankAccount existing;

            while (fread(&existing, sizeof(BankAccount), 1, check_fp))
            {
                if (strcmp(existing.phone_no, acc.phone_no) == 0)
                {
                    duplicate = 1;
                    break;
                }
            }

            fclose(check_fp);
        }

        if (duplicate)
        {
            printf("Error: This phone number is already registered.\n");
            fclose(fp_acc);
            return;
        }
        break;
    } while (1);

    getchar(); // Clear newline before the next input

    // Email validation and duplicate check
    do
    {
        int valid = 1;
        int duplicate = 0;

        printf("Enter Email ID: ");
        fgets(acc.email_id, sizeof(acc.email_id), stdin);

        // Remove trailing newline
        acc.email_id[strcspn(acc.email_id, "\n")] = '\0';

        // Basic email format validation
        if (!strchr(acc.email_id, '@') ||
            !strchr(acc.email_id, '.') ||
            strchr(acc.email_id, '@') > strrchr(acc.email_id, '.') ||
            strchr(acc.email_id, '@') == acc.email_id ||
            strchr(acc.email_id, '.') == NULL ||
            strlen(acc.email_id) == 0)
        {
            valid = 0;
        }

        if (!valid)
        {
            printf("Invalid Email! Enter a valid email address (e.g., user@gmail.com).\n");
            continue;
        }

        // Check whether email already exists
        FILE *check_fp = fopen("accounts.dat", "rb");

        if (check_fp != NULL)
        {
            BankAccount existing;

            while (fread(&existing, sizeof(BankAccount), 1, check_fp))
            {
                if (strcmp(existing.email_id, acc.email_id) == 0)
                {
                    duplicate = 1;
                    break;
                }
            }

            fclose(check_fp);
        }

        if (duplicate)
        {
            printf("Error: This Email ID is already registered.\n");

            fclose(fp_acc);
            return;
        }
        break;
    } while (1);

    printf("Enter Occupation: ");
    fgets(acc.occupation, sizeof(acc.occupation), stdin);
    acc.occupation[strcspn(acc.occupation, "\n")] = 0;

    printf("Enter Income in Rs: ");
    scanf("%d", &acc.income);

    // Balance validation
    do
    {
        printf("Enter initial balance (minimum 1000 Rs required): ");
        scanf("%f", &acc.balance);
        if (acc.balance < 1000)
        {
            printf("Minimum balance should be Rs 1000.\n");
        }
    } while (acc.balance < 1000);

    fwrite(&acc, sizeof(BankAccount), 1, fp_acc);

    fclose(fp_acc);

    // --- Add initial transaction ---
    fp_trans = fopen("transactions.dat", "ab");
    if (fp_trans)
    {
        Transaction tr;
        tr.acc_no = acc.acc_no;
        tr.type = 'O';
        tr.amount = acc.balance;

        // Timestamp
        time_t t = time(NULL);
        struct tm tm_info = *localtime(&t);
        strftime(tr.timestamp, sizeof(tr.timestamp), "%Y-%m-%d %H:%M", &tm_info);

        fwrite(&tr, sizeof(Transaction), 1, fp_trans);
        fclose(fp_trans);
    }
    else
    {
        printf("Could not open transactions file.\n");
    }

    // Log
    char action[150];
    sprintf(action, "Account created for %s | Acc No: %d | Initial Balance: %.2f", username, acc.acc_no, acc.balance);
    log_action("ADMIN", admin, action);

    record_transaction(acc.acc_no, 0, 'O', "CREDIT", acc.balance, acc.balance);

    printf("Savings Account created successfully for user: %s, Account Number: %d\n",
           username, acc.acc_no);
}

void edit_account()
{
    FILE *fp;
    FILE *check_fp;
    BankAccount acc, existing;
    char username[20];
    char temp[100];
    int found = 0;

    printf("Enter username of the customer to edit account: ");
    scanf("%19s", username);
    getchar();

    fp = fopen("accounts.dat", "rb+");
    if (!fp)
    {
        printf("No accounts found.\n");
        return;
    }

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (strcmp(acc.username, username) == 0)
        {
            found = 1;

            printf("\n--- Current Account Details ---\n");
            printf("Account No: %d\n", acc.acc_no);
            printf("Name: %s\n", acc.name);
            printf("DOB: %02d-%02d-%04d\n",
                   acc.date_of_birth.day,
                   acc.date_of_birth.month,
                   acc.date_of_birth.year);
            printf("Address: %s\n", acc.address);
            printf("Aadhar: %s\n", acc.aadhar_no);
            printf("Phone: %s\n", acc.phone_no);
            printf("Email: %s\n", acc.email_id);
            printf("Occupation: %s\n", acc.occupation);
            printf("Income: %d\n", acc.income);
            printf("Balance: %.2f\n", acc.balance);

            printf("\n--- Edit Account Details ---\n");
            printf("Press Enter to keep the existing value.\n");

            /* Address */
            printf("\nEnter new Address: ");
            fgets(temp, sizeof(temp), stdin);
            temp[strcspn(temp, "\n")] = '\0';

            if (strlen(temp) > 0)
            {
                strcpy(acc.address, temp);
            }

            /* Aadhaar number */
            while (1)
            {
                int valid = 1;

                printf("Enter new Aadhaar number (12 digits): ");
                fgets(temp, sizeof(temp), stdin);
                temp[strcspn(temp, "\n")] = '\0';

                if (strlen(temp) == 0)
                    break;

                if (strlen(temp) != 12)
                {
                    valid = 0;
                }
                else
                {
                    for (int i = 0; i < 12; i++)
                    {
                        if (!isdigit((unsigned char)temp[i]))
                        {
                            valid = 0;
                            break;
                        }
                    }
                }

                if (!valid)
                {
                    printf("Invalid Aadhaar! Enter exactly 12 digits.\n");
                    continue;
                }

                check_fp = fopen("accounts.dat", "rb");
                if (!check_fp)
                {
                    printf("Error checking existing accounts.\n");
                    fclose(fp);
                    return;
                }

                int duplicate = 0;

                while (fread(&existing, sizeof(BankAccount), 1, check_fp))
                {
                    if (strcmp(existing.username, username) != 0 &&
                        strcmp(existing.aadhar_no, temp) == 0)
                    {
                        duplicate = 1;
                        break;
                    }
                }

                fclose(check_fp);

                if (duplicate)
                {
                    printf("Error: This Aadhaar number is already registered!\n");
                    fclose(fp);
                    return;
                }

                strcpy(acc.aadhar_no, temp);
                break;
            }

            /* Phone number */
            while (1)
            {
                int valid = 1;

                printf("Enter new Phone number (10 digits): ");
                fgets(temp, sizeof(temp), stdin);
                temp[strcspn(temp, "\n")] = '\0';

                if (strlen(temp) == 0)
                    break;

                if (strlen(temp) != 10)
                {
                    valid = 0;
                }
                else if (temp[0] < '6' || temp[0] > '9')
                {
                    valid = 0;
                }
                else
                {
                    for (int i = 0; i < 10; i++)
                    {
                        if (!isdigit((unsigned char)temp[i]))
                        {
                            valid = 0;
                            break;
                        }
                    }
                }

                if (!valid)
                {
                    printf("Invalid phone number!\n");
                    printf("Enter 10 digits starting with 6, 7, 8 or 9.\n");
                    continue;
                }

                check_fp = fopen("accounts.dat", "rb");
                if (!check_fp)
                {
                    printf("Error checking existing accounts.\n");
                    fclose(fp);
                    return;
                }

                int duplicate = 0;

                while (fread(&existing, sizeof(BankAccount), 1, check_fp))
                {
                    if (strcmp(existing.username, username) != 0 &&
                        strcmp(existing.phone_no, temp) == 0)
                    {
                        duplicate = 1;
                        break;
                    }
                }

                fclose(check_fp);

                if (duplicate)
                {
                    printf("Error: This phone number is already registered!\n");
                    fclose(fp);
                    return;
                }

                strcpy(acc.phone_no, temp);
                break;
            }

            /* Email ID */
            while (1)
            {
                printf("Enter new Email ID: ");
                fgets(temp, sizeof(temp), stdin);
                temp[strcspn(temp, "\n")] = '\0';

                if (strlen(temp) == 0)
                    break;

                char *at = strchr(temp, '@');
                char *dot = at ? strchr(at + 1, '.') : NULL;

                if (at == NULL || at == temp ||
                    dot == NULL || dot == at + 1 ||
                    dot[1] == '\0' ||
                    strchr(at + 1, '@') != NULL)
                {
                    printf("Invalid email format! Please try again.\n");
                    continue;
                }

                check_fp = fopen("accounts.dat", "rb");
                if (!check_fp)
                {
                    printf("Error checking existing accounts.\n");
                    fclose(fp);
                    return;
                }

                int duplicate = 0;

                while (fread(&existing, sizeof(BankAccount), 1, check_fp))
                {
                    if (strcmp(existing.username, username) != 0 &&
                        strcmp(existing.email_id, temp) == 0)
                    {
                        duplicate = 1;
                        break;
                    }
                }

                fclose(check_fp);

                if (duplicate)
                {
                    printf("Error: This email ID is already registered!\n");
                    fclose(fp);
                    return;
                }

                strcpy(acc.email_id, temp);
                break;
            }

            /* Occupation */
            printf("Enter new Occupation: ");
            fgets(temp, sizeof(temp), stdin);
            temp[strcspn(temp, "\n")] = '\0';

            if (strlen(temp) > 0)
            {
                strcpy(acc.occupation, temp);
            }

            /* Income */
            printf("Enter new Income: ");
            fgets(temp, sizeof(temp), stdin);
            temp[strcspn(temp, "\n")] = '\0';

            if (strlen(temp) > 0)
            {
                char *endptr;
                long new_income = strtol(temp, &endptr, 10);

                if (endptr != temp &&
                    *endptr == '\0' &&
                    new_income > 0 &&
                    new_income <= 2147483647L)
                {
                    acc.income = (int)new_income;
                }
                else
                {
                    printf("Invalid income. Keeping the old income.\n");
                }
            }

            /* Update account record */
            if (fseek(fp, -(long)sizeof(BankAccount), SEEK_CUR) != 0)
            {
                printf("Error locating account record.\n");
                fclose(fp);
                return;
            }

            if (fwrite(&acc, sizeof(BankAccount), 1, fp) != 1)
            {
                printf("Error updating account.\n");
                fclose(fp);
                return;
            }

            fflush(fp);
            fclose(fp);

            log_action("ADMIN", username, "Account details updated");
            printf("\nAccount updated successfully for user: %s\n", username);
            return;
        }
    }

    fclose(fp);

    if (!found)
    {
        printf("No account found for username: %s\n", username);
    }
}

void delete_account(const char *owner, int is_admin)
{
    int acc_no;
    BankAccount acc;
    FILE *fp, *temp;
    int found = 0;
    char confirm;

    printf("Enter Account number to delete: ");
    scanf("%d", &acc_no);

    fp = fopen("accounts.dat", "rb");
    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    temp = fopen("temp.dat", "wb");
    if (!temp)
    {
        printf("Error opening temporary file.\n");
        fclose(fp);
        return;
    }

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == acc_no && (is_admin || strcmp(acc.username, owner) == 0))
        {
            found = 1;
            printf("\nAccount Found:\n");
            printf("Account No: %d\n", acc.acc_no);
            printf("Holder Name: %s\n", acc.name);
            printf("Balance: %.2f\n", acc.balance);

            printf("\nAre you sure you want to DELETE this account? (Y/N): ");
            scanf(" %c", &confirm);

            if (confirm == 'Y' || confirm == 'y')
            {
                printf("Account %d deleted successfully.\n", acc.acc_no);
                record_transaction(
                    acc.acc_no, // Account being deleted
                    -1,         // No related account
                    'X',        // X = Account Deleted
                    "DEBIT",    // Direction
                    0.0f,       // No money involved
                    acc.balance // Balance before deletion
                );
                continue; // Skip writing this record
            }
            else
            {
                printf("Deletion cancelled.\n");
            }
        }
        fwrite(&acc, sizeof(BankAccount), 1, temp);
    }

    fclose(fp);
    fclose(temp);

    if (remove("accounts.dat") != 0 || rename("temp.dat", "accounts.dat") != 0)
    {
        printf("Error updating file.\n");
    }

    if (!found)
    {
        printf("Account is not found\n");
    }
}

void view_all_accounts()
{
    FILE *fp;
    BankAccount acc;
    int count = 0;

    fp = fopen("accounts.dat", "rb");
    if (!fp)
    {
        printf("Error opening accounts file or no accounts exist.\n");
        return;
    }

    printf("\n======================= All Accounts ==============================\n");
    printf("%-10s %-20s %-10s %-15s %-10s\n",
           "Acc No", "Account Holder Name", "Type", "Phone", "Balance");
    printf("--------------------------------------------------------------------\n");

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        printf("%-10d %-20s %-10s %-15s %-10.2f\n",
               acc.acc_no, acc.name, acc.acc_type, acc.phone_no, acc.balance);
        count++;
    }

    if (count == 0)
    {
        printf("No accounts found.\n");
    }

    printf("=====================================================================\n");
    fclose(fp);
}

void view_transactions(const char *username, int is_admin)
{
    Transaction tr;
    BankAccount acc;

    FILE *trans_fp = fopen("transactions.dat", "rb");
    FILE *acc_fp = fopen("accounts.dat", "rb");

    int acc_no = -1;
    int found_account = 0;
    int count = 0;

    if (!trans_fp || !acc_fp)
    {
        printf("Error opening data files.\n");

        if (trans_fp)
            fclose(trans_fp);

        if (acc_fp)
            fclose(acc_fp);

        return;
    }

    // =====================================================
    // ADMIN
    // =====================================================

    if (is_admin)
    {
        printf("Enter account number to view transactions: ");
        scanf("%d", &acc_no);

        // Verify account exists
        while (fread(&acc, sizeof(BankAccount), 1, acc_fp))
        {
            if (acc.acc_no == acc_no)
            {
                found_account = 1;
                break;
            }
        }

        if (!found_account)
        {
            printf("Account number %d not found.\n", acc_no);

            fclose(trans_fp);
            fclose(acc_fp);
            return;
        }
    }

    // =====================================================
    // USER
    // =====================================================

    else
    {
        while (fread(&acc, sizeof(BankAccount), 1, acc_fp))
        {
            if (strcmp(acc.username, username) == 0)
            {
                acc_no = acc.acc_no;
                found_account = 1;
                break;
            }
        }

        if (!found_account)
        {
            printf("No bank account found for this user.\n");

            fclose(trans_fp);
            fclose(acc_fp);
            return;
        }
    }

    fclose(acc_fp);

    // =====================================================
    // DISPLAY HEADER
    // =====================================================

    printf("\n");
    printf("================================================================================================================\n");
    printf("                              ACCOUNT TRANSACTION STATEMENT\n");
    printf("================================================================================================================\n");

    printf("Account Number : %d\n", acc_no);

    if (!is_admin)
        printf("Username       : %s\n", username);

    printf("----------------------------------------------------------------------------------------------------------------\n");

    printf("%-20s %-18s %-12s %-12s %-15s\n",
           "Date & Time",
           "Transaction",
           "Debit",
           "Credit",
           "Balance");

    printf("----------------------------------------------------------------------------------------------------------------\n");

    // =====================================================
    // READ ONLY THIS ACCOUNT'S TRANSACTIONS
    // =====================================================

    while (fread(&tr, sizeof(Transaction), 1, trans_fp))
    {
        // IMPORTANT:
        // Only transactions belonging to this account
        if (tr.acc_no != acc_no)
            continue;

        char transaction_name[30];

        // Transaction type
        if (tr.type == 'O')
        {
            strcpy(transaction_name, "Opening Balance");
        }
        else if (tr.type == 'D')
        {
            strcpy(transaction_name, "Deposit");
        }
        else if (tr.type == 'W')
        {
            strcpy(transaction_name, "Withdrawal");
        }
        else if (tr.type == 'T')
        {
            if (strcmp(tr.direction, "DEBIT") == 0)
                strcpy(transaction_name, "Transfer Sent");
            else
                strcpy(transaction_name, "Transfer Received");
        }
        else if (tr.type == 'X')
        {
            strcpy(transaction_name, "Account Deleted");
        }
        else
        {
            strcpy(transaction_name, "Unknown");
        }

        // =================================================
        // DEBIT TRANSACTION
        // =================================================

        if (strcmp(tr.direction, "DEBIT") == 0)
        {
            printf("%-20s %-18s %-12.2f %-12s %-15.2f\n",
                   tr.timestamp,
                   transaction_name,
                   tr.amount,
                   "-",
                   tr.balance_after);
        }

        // =================================================
        // CREDIT TRANSACTION
        // =================================================

        else if (strcmp(tr.direction, "CREDIT") == 0)
        {
            printf("%-20s %-18s %-12s %-12.2f %-15.2f\n",
                   tr.timestamp,
                   transaction_name,
                   "-",
                   tr.amount,
                   tr.balance_after);
        }

        count++;
    }

    fclose(trans_fp);

    // =====================================================
    // NO TRANSACTIONS
    // =====================================================

    if (count == 0)
    {
        printf("No transactions found for Account %d.\n", acc_no);
    }

    printf("================================================================================================================\n");
}


void print_statement(const char *username, int is_admin)
{
    Transaction tr;
    BankAccount acc;

    FILE *acc_fp = fopen("accounts.dat", "rb");
    FILE *trans_fp = fopen("transactions.dat", "rb");

    int acc_no = -1;
    int found_account = 0;
    int count = 0;

    char actual_username[20] = "";

    if (!acc_fp || !trans_fp)
    {
        printf("Error opening data files.\n");

        if (acc_fp)
            fclose(acc_fp);

        if (trans_fp)
            fclose(trans_fp);

        return;
    }

    // =====================================================
    // ADMIN / USER ACCOUNT SELECTION
    // =====================================================

    if (is_admin)
    {
        printf("Enter account number to print statement: ");
        scanf("%d", &acc_no);

        while (fread(&acc, sizeof(BankAccount), 1, acc_fp))
        {
            if (acc.acc_no == acc_no)
            {
                strcpy(actual_username, acc.username);
                found_account = 1;
                break;
            }
        }
    }
    else
    {
        while (fread(&acc, sizeof(BankAccount), 1, acc_fp))
        {
            if (strcmp(acc.username, username) == 0)
            {
                acc_no = acc.acc_no;
                strcpy(actual_username, acc.username);
                found_account = 1;
                break;
            }
        }
    }

    fclose(acc_fp);

    if (!found_account)
    {
        printf("Account not found.\n");
        fclose(trans_fp);
        return;
    }

    // =====================================================
    // CREATE FOLDER
    // =====================================================

#if defined(_WIN32) || defined(_WIN64)
    _mkdir("bank_statements");
#else
    mkdir("bank_statements", 0777);
#endif

    // =====================================================
    // CREATE STATEMENT FILE
    // =====================================================

    FILE *out_fp;
    char filename[60];

    snprintf(filename, sizeof(filename),
             "bank_statements/%s_statement.txt",
             actual_username);

    out_fp = fopen(filename, "w");

    if (!out_fp)
    {
        printf("Error creating statement file.\n");
        fclose(trans_fp);
        return;
    }

    // =====================================================
    // DISPLAY HEADER
    // =====================================================

    printf("\n");
    printf("================================================================================================================\n");
    printf("                                      MINI_C_BANK STATEMENT\n");
    printf("================================================================================================================\n");

    printf("Account Number : %d\n", acc_no);
    printf("Username       : %s\n", actual_username);

    printf("----------------------------------------------------------------------------------------------------------------\n");

    printf("%-20s %-18s %-12s %-12s %-15s\n",
           "Date & Time",
           "Transaction",
           "Debit",
           "Credit",
           "Balance");

    printf("----------------------------------------------------------------------------------------------------------------\n");

    // Write header to file
    fprintf(out_fp, "================================================================================================================\n");
    fprintf(out_fp, "                                      MINI_C_BANK STATEMENT\n");
    fprintf(out_fp, "================================================================================================================\n");

    fprintf(out_fp, "Account Number : %d\n", acc_no);
    fprintf(out_fp, "Username       : %s\n", actual_username);

    fprintf(out_fp, "----------------------------------------------------------------------------------------------------------------\n");

    fprintf(out_fp, "%-20s %-18s %-12s %-12s %-15s\n",
            "Date & Time",
            "Transaction",
            "Debit",
            "Credit",
            "Balance");

    fprintf(out_fp, "----------------------------------------------------------------------------------------------------------------\n");

    // =====================================================
    // READ ACCOUNT TRANSACTIONS
    // =====================================================

    while (fread(&tr, sizeof(Transaction), 1, trans_fp))
    {
        if (tr.acc_no != acc_no)
            continue;

        char transaction_name[30];

        // Transaction type
        if (tr.type == 'O')
        {
            strcpy(transaction_name, "Opening Balance");
        }
        else if (tr.type == 'D')
        {
            strcpy(transaction_name, "Deposit");
        }
        else if (tr.type == 'W')
        {
            strcpy(transaction_name, "Withdrawal");
        }
        else if (tr.type == 'T')
        {
            if (strcmp(tr.direction, "DEBIT") == 0)
                strcpy(transaction_name, "Transfer Sent");
            else
                strcpy(transaction_name, "Transfer Received");
        }
        else if (tr.type == 'L')
        {
            strcpy(transaction_name, "Loan Payment");
        }
        else if (tr.type == 'X')
        {
            strcpy(transaction_name, "Account Deleted");
        }
        else
        {
            strcpy(transaction_name, "Unknown");
        }

        // =================================================
        // DEBIT TRANSACTION
        // =================================================

        if (strcmp(tr.direction, "DEBIT") == 0)
        {
            printf("%-20s %-18s %-12.2f %-12s %-15.2f\n",
                   tr.timestamp,
                   transaction_name,
                   tr.amount,
                   "-",
                   tr.balance_after);

            fprintf(out_fp, "%-20s %-18s %-12.2f %-12s %-15.2f\n",
                    tr.timestamp,
                    transaction_name,
                    tr.amount,
                    "-",
                    tr.balance_after);
        }

        // =================================================
        // CREDIT TRANSACTION
        // =================================================

        else if (strcmp(tr.direction, "CREDIT") == 0)
        {
            printf("%-20s %-18s %-12s %-12.2f %-15.2f\n",
                   tr.timestamp,
                   transaction_name,
                   "-",
                   tr.amount,
                   tr.balance_after);

            fprintf(out_fp, "%-20s %-18s %-12s %-12.2f %-15.2f\n",
                    tr.timestamp,
                    transaction_name,
                    "-",
                    tr.amount,
                    tr.balance_after);
        }

        count++;
    }

    fclose(trans_fp);

    // =====================================================
    // END OF STATEMENT
    // =====================================================

    printf("================================================================================================================\n");

    fprintf(out_fp, "================================================================================================================\n");

    fclose(out_fp);

    if (count == 0)
    {
        printf("No transactions found for account %d.\n", acc_no);
        remove(filename);
    }
    else
    {
        printf("\nBank Statement saved successfully: %s\n", filename);
    }
}

void search_account()
{
    int acc_no;
    BankAccount acc;
    FILE *fp = fopen("accounts.dat", "rb");
    int found = 0;

    if (!fp)
    {
        printf("No accounts found.\n");
        return;
    }

    printf("Enter account number to search: ");
    scanf("%d", &acc_no);

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == acc_no)
        {
            printf("Account Number: %d\nName: %s\nBalance: %.2f\n", acc.acc_no, acc.name, acc.balance);
            found = 1;
            break;
        }
    }
    if (!found)
        printf("Account not found.\n");
    fclose(fp);
}

// User functions
void display_user_account(const char *owner)
{
    FILE *fp = fopen("accounts.dat", "rb");
    BankAccount acc;
    int found = 0;

    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (strcmp(acc.username, owner) == 0)
        {
            printf("\n-- Your Bank Account Details --\n");
            printf("----------------------------------------\n");
            printf("Account No   : %d\n", acc.acc_no);
            printf("Account Type : %s\n", acc.acc_type);
            printf("Holder Name  : %s\n", acc.name);
            printf("DOB          : %02d-%02d-%04d\n", acc.date_of_birth.day, acc.date_of_birth.month, acc.date_of_birth.year);
            printf("Address      : %s\n", acc.address);
            printf("Aadhar No    : %s\n", acc.aadhar_no);
            printf("Email ID     : %s\n", acc.email_id);
            printf("Phone No     : %s\n", acc.phone_no);
            printf("Occupation   : %s\n", acc.occupation);
            printf("Income       : %d\n", acc.income);
            printf("Balance      : %.2f\n", acc.balance);
            printf("----------------------------------------\n");
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("You don't have any account yet.\n");
    }

    fclose(fp);
}

void view_account_balance(const char *username)
{
    BankAccount acc;
    FILE *fp = fopen("accounts.dat", "rb");
    int found = 0;

    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }
    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (strcmp(acc.username, username) == 0)
        {
            printf("-----------------------------------------\n");
            printf("Account No   : %d\n", acc.acc_no);
            printf("Balance      : %.2f\n", acc.balance);
            printf("-----------------------------------------\n");
            found = 1;
        }
    }

    if (!found)
    {
        printf("No account found for this user.\n");
    }

    fclose(fp);
}

void deposit(const char *owner, int is_admin)
{

    int acc_no, found = 0;
    float amount;
    BankAccount acc;
    FILE *fp = fopen("accounts.dat", "rb+");

    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    printf("Enter account number: ");
    scanf("%d", &acc_no);

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == acc_no && (is_admin || strcmp(owner, acc.username) == 0))
        {
            printf("Amount to deposit: ");
            scanf("%f", &amount);
            if (amount <= 0)
            {
                printf("Invalid amount.\n");
                fclose(fp);
                return;
            }
            acc.balance += amount;
            fseek(fp, -sizeof(BankAccount), SEEK_CUR);
            fwrite(&acc, sizeof(BankAccount), 1, fp);

            fflush(fp);
            fclose(fp);

            record_transaction(
                acc.acc_no,
                -1,
                'D',
                "CREDIT",
                amount,
                acc.balance);

            // Log
            char action[100];
            sprintf(action, "Deposit | Acc: %d | Amount: %.2f", acc.acc_no, amount);

            log_action("USER", owner, action);

            printf("Deposit successful.\n");
            printf("Now, Your Balance is: Rs %f\n", acc.balance);

            found = 1;
            break;
        }
    }
    if (!found)
        printf("Enter your account number correctly\n");
    fclose(fp);
}

void withdraw(const char *owner, int is_admin)
{
    int acc_no, found = 0;
    float amount;
    BankAccount acc;
    FILE *fp = fopen("accounts.dat", "rb+");

    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    printf("Enter account number: ");
    scanf("%d", &acc_no);

    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == acc_no &&
            (is_admin || strcmp(owner, acc.username) == 0))
        {
            printf("Amount to withdraw: ");
            scanf("%f", &amount);

            /* validate first */
            if (amount <= 0)
            {
                printf("Invalid amount.\n");
                fclose(fp);
                return;
            }

            /* rapid transaction check */
            time_t now = time(NULL);

            if (first_withdrawal_time == 0 ||
                difftime(now, first_withdrawal_time) > 60)
            {
                first_withdrawal_time = now;
                withdrawal_count = 1;
            }
            else
            {
                withdrawal_count++;
            }

            if (withdrawal_count > 5)
            {
                printf("\n=========================================\n");
                printf("FRAUD DETECTION ALERT\n");
                printf("More than 5 withdrawals within 1 minute!\n");
                printf("Please ignore this alert, if it was you.\n");
                printf("=========================================\n");

                char fraud_msg[100];
                sprintf(fraud_msg, "Rapid Transactions detected within 1 minute | Acc.No: %d", acc.acc_no);
                fraud_alert(owner, fraud_msg);
            }

            /* daily limit check */
            struct tm *tm_info = localtime(&now);
            int today = tm_info->tm_yday;

            if (current_day != today)
            {
                current_day = today;
                daily_withdrawal_total = 0;
            }

            daily_withdrawal_total += amount;

            if (daily_withdrawal_total > 500000)
            {
                printf("\n=========================================\n");
                printf("FRAUD DETECTION ALERT\n");
                printf("Daily withdrawal exceeds Rs 5,00,000\n");
                printf("Please ignore this alert, if it was you.\n");
                printf("=========================================\n");

                char fraud_msg[100];
                sprintf(fraud_msg, "Daily withdrawal limit exceeded (Rs %.2f)", daily_withdrawal_total);
                fraud_alert(owner, fraud_msg);
            }

            /* actual withdrawal */
            if (acc.balance >= amount)
            {
                acc.balance -= amount;

                fseek(fp, -sizeof(BankAccount), SEEK_CUR);
                fwrite(&acc, sizeof(BankAccount), 1, fp);

                record_transaction(
                    acc.acc_no,
                    -1,
                    'W',
                    "DEBIT",
                    amount,
                    acc.balance);

                printf("Withdrawal successful.\n");
                printf("Now, Your Balance is: Rs %.2f\n", acc.balance);

                /* large withdrawal */
                if (amount >= 500000)
                {
                    printf("\n==============================================\n");
                    printf("FRAUD DETECTION ALERT\n");
                    printf("Large amount withdrawal exceeds Rs 5,00,000\n");
                    printf("Please ignore this alert, if it was you.\n");
                    printf("================================================\n");

                    char fraud_msg[150];
                    sprintf(fraud_msg, "Large withdrawal detected | Acc No: %d | Amount: %.2f",
                            acc.acc_no, amount);
                    fraud_alert(acc.username, fraud_msg);
                }

                /* system log */
                char action[100];
                sprintf(action, "Withdraw | Acc: %d | Amount: %.2f", acc.acc_no, amount);
                log_action("USER", owner, action);
            }
            else
            {
                printf("Insufficient funds.\n");
            }

            found = 1;
            break;
        }
    }

    if (!found)
        printf("Enter your account number correctly\n");

    fclose(fp);
}

void transfer_money(const char *owner)
{
    int to_acc;
    float amount;
    BankAccount acc, from, to;
    FILE *fp = fopen("accounts.dat", "rb+");
    int found_from = 0, found_to = 0;

    if (!fp)
    {
        printf("Error opening accounts file.\n");
        return;
    }

    // Find the logged-in user's account
    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (strcmp(acc.username, owner) == 0)
        {
            from = acc;
            found_from = 1;
            break;
        }
    }

    if (!found_from)
    {
        printf("Your account was not found.\n");
        fclose(fp);
        return;
    }

    printf("Your Account Number: %d\n", from.acc_no);
    printf("Enter receiver's account number: ");
    scanf("%d", &to_acc);

    if (to_acc == from.acc_no)
    {
        printf("You cannot transfer money to your own account.\n");
        log_action("USER", owner, "Transfer failed - attempted self transfer");
        fclose(fp);
        return;
    }

    printf("Enter amount to transfer: ");
    scanf("%f", &amount);

    if (amount <= 0)
    {
        printf("Invalid amount.\n");
        fclose(fp);
        return;
    }

    if (from.balance < amount)
    {
        printf("Insufficient funds.\n");
        fclose(fp);
        return;
    }

    // Find receiver account
    rewind(fp);
    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == to_acc)
        {
            to = acc;
            found_to = 1;
            break;
        }
    }

    if (!found_to)
    {
        printf("Receiver account not found.\n");
        fclose(fp);
        return;
    }

    // Update balances
    from.balance -= amount;
    to.balance += amount;

    // Rewrite updates in file
    rewind(fp);
    while (fread(&acc, sizeof(BankAccount), 1, fp))
    {
        if (acc.acc_no == from.acc_no)
        {
            fseek(fp, -sizeof(BankAccount), SEEK_CUR);
            fwrite(&from, sizeof(BankAccount), 1, fp);
            fseek(fp, 0, SEEK_CUR);
        }
        else if (acc.acc_no == to.acc_no)
        {
            fseek(fp, -sizeof(BankAccount), SEEK_CUR);
            fwrite(&to, sizeof(BankAccount), 1, fp);
            fseek(fp, 0, SEEK_CUR);
        }
    }

    // Log the transactions
    record_transaction(
        from.acc_no,
        to.acc_no,
        'T',
        "DEBIT",
        amount,
        from.balance);

    record_transaction(
        to.acc_no,
        from.acc_no,
        'T',
        "CREDIT",
        amount,
        to.balance);

    // Log
    char action[100];
    sprintf(action, "Transfer | From Acc: %d -> To Acc: %d Amount: %.2f", from.acc_no, to.acc_no, amount);
    log_action("USER", owner, action);

    printf("Transfer successful! Rs %.2f transferred to A/C %d.\n", amount, to.acc_no);
    printf("Remaining Balance: Rs %.2f\n", from.balance);

    if (amount >= 1000000)
    {
        printf("\n=========================================\n");
        printf("FRAUD DETECTION ALERT\n");
        printf("LARGE MONEY TRANSFER DETECTED\n");
        printf("Please ignore this alert, if it was you.\n");
        printf("=========================================\n");

        char fraud_msg[150];
        sprintf(fraud_msg,
                "Large transfer detected | From Acc: %d -> To Acc: %d | Amount: %.2f",
                from.acc_no, to.acc_no, amount);
        fraud_alert(owner, fraud_msg);
    }

    fclose(fp);
}

void apply_loan(const char *username)
{
    Loan l;
    FILE *fp = fopen("loans.dat", "ab");

    l.loan_id = generate_loan_id();
    strcpy(l.username, username);

    do
    {
        printf("Enter loan amount (minimum Rs. 1000): ");
        scanf("%f", &l.amount);

        if (l.amount < 1000)
        {
            printf("Loan amount must be minimum Rs. 1000.\n");
        }

    } while (l.amount < 1000);

    strcpy(l.status, "pending");

    fwrite(&l, sizeof(Loan), 1, fp);

    char action[100];
    sprintf(action, "Loan applied | Amount: %.2f | Loan ID: %d", l.amount, l.loan_id);
    log_action("USER", username, action);

    fclose(fp);
    printf("Loan request submitted.\n");
}

void view_my_loan_status(const char *username)
{
    FILE *fp = fopen("loans.dat", "rb");
    Loan l;
    int found = 0;

    if (!fp)
    {
        printf("You have not applied for any loans\n");
        return;
    }

    printf("\n--- Your Loan Applications ---\n");
    printf("%-10s %-10s %-10s\n", "Loan ID", "Amount", "Status");
    printf("----------------------------------\n");

    while (fread(&l, sizeof(Loan), 1, fp))
    {
        if (strcmp(l.username, username) == 0)
        {
            printf("%-10d %-10.2f %-10s\n", l.loan_id, l.amount, l.status);
            found = 1;
        }
    }

    if (!found)
    {
        printf("You have not applied for any loans.\n");
    }

    fclose(fp);
}

void view_my_logs(const char *username)
{
    FILE *fp = fopen("system_logs.txt", "r");
    char line[300];
    int found = 0;

    if (!fp)
    {
        printf("No logs found.\n");
        return;
    }

    printf("\n===== Your Activity Logs =====\n");

    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, username) != NULL)
        {
            printf("%s", line);
            found = 1;
        }
    }

    if (!found)
    {
        printf("No logs found for user: %s\n", username);
    }

    fclose(fp);
}

void user_reset_password(const char *username)
{
    char oldpass[20];
    char newpass[20];

    User u;
    int found = 0;
    int has_digit, has_special, len;
    int ch;

    FILE *fp = fopen("users.dat", "rb+");

    if (!fp)
    {
        printf("Error opening users.dat. No users found.\n");
        return;
    }

    // Clear newline left by scanf() in the menu
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
        // Discard remaining input
    }

    while (fread(&u, sizeof(User), 1, fp))
    {
        if (strcmp(u.username, username) == 0)
        {
            found = 1;

            // Verify current password
            printf("Enter current password: ");
            read_password(oldpass, sizeof(oldpass));

            if (strcmp(u.password, oldpass) != 0)
            {
                printf("Incorrect current password.\n");
                fclose(fp);
                return;
            }

            // Validate new password
            while (1)
            {
                has_digit = 0;
                has_special = 0;

                printf("Enter new password "
                       "(8-12 chars, at least 1 number "
                       "and 1 special character): ");

                read_password(newpass, sizeof(newpass));

                len = (int)strlen(newpass);

                for (int i = 0; i < len; i++)
                {
                    unsigned char c = (unsigned char)newpass[i];

                    if (isdigit(c))
                        has_digit = 1;

                    if (!isalnum(c))
                        has_special = 1;
                }

                if (len < 8)
                {
                    printf("Password must be at least "
                           "8 characters.\n");
                }
                else if (len > 12)
                {
                    printf("Password must not exceed "
                           "12 characters.\n");
                }
                else if (!has_digit)
                {
                    printf("Password must contain at least "
                           "one number.\n");
                }
                else if (!has_special)
                {
                    printf("Password must contain at least "
                           "one special character.\n");
                }
                else if (strcmp(oldpass, newpass) == 0)
                {
                    printf("New password must be different "
                           "from the current password.\n");
                }
                else
                {
                    break;
                }
            }

            // Update the user's record in users.dat
            strcpy(u.password, newpass);

            if (fseek(fp, -(long)sizeof(User), SEEK_CUR) != 0)
            {
                printf("Error locating user record.\n");
                fclose(fp);
                return;
            }

            if (fwrite(&u, sizeof(User), 1, fp) != 1)
            {
                printf("Error updating password.\n");
                fclose(fp);
                return;
            }

            fflush(fp);
            fclose(fp);

            log_action("USER", username,
                       "Password reset successfully");

            printf("Password updated successfully!\n");
            return;
        }
    }

    fclose(fp);

    if (!found)
    {
        printf("User not found.\n");
    }
}

// Admin functions
void detect_fraud()
{
    FILE *fp = fopen("fraud_alert_logs.txt", "r");
    char line[300];

    if (!fp)
    {
        printf("No fraud alerts found.\n");
        return;
    }

    printf("\n====================================================== FRAUD ALERT REPORT ====================================================\n");

    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
    }

    printf("==============================================================================================================================\n");

    fclose(fp);
}

void suspend_or_reactivate_user()
{
    char username[20];
    char action[100];
    User u;
    int found = 0;

    FILE *fp = fopen("users.dat", "rb+");

    if (!fp)
    {
        printf("No users found.\n");
        return;
    }

    printf("Username to suspend/reactivate: ");
    scanf("%19s", username);

    while (fread(&u, sizeof(User), 1, fp) == 1)
    {
        u.username[sizeof(u.username) - 1] = '\0';
        u.role[sizeof(u.role) - 1] = '\0';
        u.status[sizeof(u.status) - 1] = '\0';

        if (strcmp(u.username, username) == 0)
        {
            found = 1;

            if (strcmp(u.role, "admin") == 0)
            {
                printf("Admin accounts cannot be modified here.\n");
                break;
            }

            if (strcmp(u.status, "active") == 0)
            {
                strcpy(u.status, "suspended");
                snprintf(action, sizeof(action),
                         "User suspended: %s", username);
            }
            else if (strcmp(u.status, "suspended") == 0)
            {
                strcpy(u.status, "active");
                snprintf(action, sizeof(action),
                         "User reactivated: %s", username);
            }
            else
            {
                printf("Invalid user status.\n");
                break;
            }

            if (fseek(fp, -(long)sizeof(User), SEEK_CUR) != 0)
            {
                printf("Error locating user record.\n");
                break;
            }

            if (fwrite(&u, sizeof(User), 1, fp) != 1)
            {
                printf("Error updating user status.\n");
                break;
            }

            fflush(fp);

            printf("User '%s' status updated to '%s'.\n",
                   username, u.status);

            log_action("ADMIN", "ADMIN", action);
            break;
        }
    }

    if (ferror(fp))
    {
        printf("Error reading users.dat.\n");
    }
    else if (!found)
    {
        printf("User not found.\n");
    }

    fclose(fp);
}

void review_loans(const char *admin)
{
    Loan l;
    FILE *fp;
    int choice, loan_id, found;

    while (1)
    {
        printf("\n-- Loan Management --\n");
        printf("1. View All Loans\n");
        printf("2. Review Pending Loans\n");
        printf("0. Back to Admin Menu\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        if (choice == 0)
            break;

        if (choice == 1)
        {
            // View all loans
            fp = fopen("loans.dat", "rb");
            if (!fp)
            {
                printf("No loan records found.\n");
                continue;
            }

            printf("\n-- Loan History --\n");
            printf("%-10s %-20s %-10s %-10s\n", "Loan ID", "Username", "Amount", "Status");
            printf("-------------------------------------------------------------\n");

            while (fread(&l, sizeof(Loan), 1, fp))
            {
                printf("%-10d %-20s %-10.2f %-10s\n",
                       l.loan_id, l.username, l.amount, l.status);
            }

            fclose(fp);
        }
        else if (choice == 2)
        {
            while (1)
            {
                fp = fopen("loans.dat", "rb+");
                if (!fp)
                {
                    printf("No loan requests.\n");
                    break;
                }

                found = 0;
                printf("\n-- Pending Loan Requests --\n");
                printf("%-10s %-20s %-10s\n", "Loan ID", "Username", "Amount");
                printf("--------------------------------------------------\n");

                // Show pending loans
                while (fread(&l, sizeof(Loan), 1, fp))
                {
                    if (strcmp(l.status, "pending") == 0)
                    {
                        printf("%-10d %-20s %-10.2f\n", l.loan_id, l.username, l.amount);
                        found = 1;
                    }
                }

                if (!found)
                {
                    printf("No pending loan requests.\n");
                    fclose(fp);
                    break;
                }

                fclose(fp);

                printf("\nEnter Loan ID to review (0 to stop reviewing): ");
                scanf("%d", &loan_id);

                if (loan_id == 0)
                    break;

                fp = fopen("loans.dat", "rb+");
                if (!fp)
                {
                    printf("Error opening loans file.\n");
                    break;
                }

                while (fread(&l, sizeof(Loan), 1, fp))
                {
                    if (l.loan_id == loan_id && strcmp(l.status, "pending") == 0)
                    {
                        printf("Reviewing Loan ID %d | User: %s | Amount: %.2f\n",
                               l.loan_id, l.username, l.amount);
                        printf("1. Approve  2. Reject  3. Cancel: ");
                        scanf("%d", &choice);

                        if (choice == 1)
                        {
                            strcpy(l.status, "approved");
                        }
                        else if (choice == 2)
                        {
                            strcpy(l.status, "rejected");
                        }
                        else
                        {
                            printf("Cancelled review.\n");
                            break;
                        }

                        // Update loan
                        fseek(fp, -sizeof(Loan), SEEK_CUR);
                        fwrite(&l, sizeof(Loan), 1, fp);

                        // Log action
                        char action[150];
                        sprintf(action, "Loan ID %d %s", l.loan_id, l.status);
                        log_action(admin, "ADMIN", action);

                        printf("Loan %d %s.\n", l.loan_id, l.status);
                        break;
                    }
                }

                fclose(fp);
            }
        }
        else
        {
            printf("Invalid choice.\n");
        }
    }
}

void view_system_logs()
{
    FILE *fp = fopen("system_logs.txt", "r");
    char line[200];

    if (!fp)
    {
        printf("No logs found.\n");
        return;
    }

    printf("\n-- System Logs --\n");
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
    }

    fclose(fp);
}

void logout(const char *username, const char *role)
{
    char action[100];

    sprintf(action, "Logged out successfully");
    log_action(role, username, action);

    printf("\n========================================\n");
    printf("  Thank you for using Mini_C_Bank!\n");
    printf("  %s logged out successfully.\n", username);
    printf("========================================\n");
}

// ======== USER AND ADMIN MENUS WITH TIMEOUT ========
void user_menu(const char *username)
{
    int choice;
    time_t last_activity = time(NULL);

    do
    {
        if (difftime(time(NULL), last_activity) > SESSION_TIMEOUT)
        {
            printf("\nSession timed out due to inactivity. Logging out...\n");
            break;
        }

        printf("\n-- User Menu --\n");
        printf("1. View My Account\n");
        printf("2. View Balance\n");
        printf("3. Deposit Money\n");
        printf("4. Withdraw Money\n");
        printf("5. Transfer Money\n");
        printf("6. View My Transactions\n");
        printf("7. View My Print Statement\n");
        printf("8. Apply for Loan\n");
        printf("9. View My Loan Status\n");
        printf("10. View My Logs\n");
        printf("11. Reset your Password\n");
        printf("0. Logout\n");
        printf("Choice: ");
        last_activity = time(NULL);
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            display_user_account(username);
            break;
        case 2:
            view_account_balance(username);
            break;
        case 3:
            deposit(username, 0);
            break;
        case 4:
            withdraw(username, 0);
            break;
        case 5:
            transfer_money(username);
            break;
        case 6:
            view_transactions(username, 0);
            break;
        case 7:
            print_statement(username, 0);
            break;
        case 8:
            apply_loan(username);
            break;
        case 9:
            view_my_loan_status(username);
            break;
        case 10:
            view_my_logs(username);
            break;
        case 11:
            user_reset_password(username);
            break;
        case 0:
            logout(username, "USER");
            break;
        default:
            printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

void admin_menu(const char *username)
{
    int choice;
    time_t last_activity = time(NULL);

    do
    {
        if (difftime(time(NULL), last_activity) > SESSION_TIMEOUT)
        {
            printf("\nSession timed out due to inactivity. Logging out...\n");
            break;
        }

        printf("\n-- Admin Menu --\n");
        printf("1. Create Customer Account\n");
        printf("2. Edit Customer Account\n");
        printf("3. Delete Customer Account\n");
        printf("4. View all Accounts\n");
        printf("5. View Transactions of a Customer\n");
        printf("6. Print Statement of a Customer\n");
        printf("7. View System Logs\n");
        printf("8. Search Account\n");
        printf("9. Review Loans\n");
        printf("10. Detect Fraud\n");
        printf("11. Suspend User (or) Reactivate User\n");
        printf("0. Logout Admin\n");
        printf("Choice: ");
        scanf("%d", &choice);

        last_activity = time(NULL);

        switch (choice)
        {
        case 1:
            create_account(username, 1);
            break;
        case 2:
            edit_account();
            break;
        case 3:
            delete_account(username, 1);
            break;
        case 4:
            view_all_accounts();
            break;
        case 5:
            view_transactions(username, 1);
            break;
        case 6:
            print_statement(username, 1);
            break;
        case 7:
            view_system_logs();
            break;
        case 8:
            search_account();
            break;
        case 9:
            review_loans(username);
            break;
        case 10:
            detect_fraud();
            break;
        case 11:
            suspend_or_reactivate_user();
            break;
        case 0:
            logout(username, "ADMIN");
            break;
        default:
            printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

// ======== MAIN FUNCTION ========
int main()
{
    char username[20], role[10];
    int logged_in = 0;
    int choice;

    srand(time(NULL));
    create_admin_account(); // ensure admin exists

    while (1)
    {
        printf("\n\n***** Welcome to Mini_C_Bank - Bank Management System *****\n");
        printf("***** Mini Project in C. Major Learning in Programming ***** \n");
        printf("1. User Registration\n");
        printf("2. User Login\n");
        printf("3. Admin Login\n");
        printf("4. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            register_user();
        }
        else if (choice == 2)
        {
            logged_in = login(username, role, 0);
            if (logged_in && _stricmp(role, "admin") != 0)
            {
                printf("Welcome %s!\n", username);
                user_menu(username);
            }
        }
        else if (choice == 3)
        {
            logged_in = login(username, role, 1);
            if (logged_in && strcmp(role, "admin") == 0)
            {
                printf("Welcome Mini_C_Bank Admin!\n");
                admin_menu(username);
            }
            else
            {
                printf("Admin login failed or wrong role.\n");
            }
        }
        else if (choice == 4)
        {
            printf("Goodbye! Please visit Mini_C_Bank Again...\n");
            break;
        }
        else
        {
            printf("Invalid choice.\n");
        }
    }
    return 0;
}