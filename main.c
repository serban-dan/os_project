#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

#define MAX_NAME_LEN 52
#define MAX_CAT_LEN 20
#define MAX_DESC_LEN 112

typedef struct Record {
    int id; //4 bytes
    char inspector[MAX_NAME_LEN]; //52 bytes
    float latitude; //4 bytes
    float longitude; //4 bytes
    char category[MAX_CAT_LEN]; //20 bytes
    int severity; //4 bytes
    time_t timestamp; //8 bytes
    char description[MAX_DESC_LEN]; //112 bytes
} Record;

enum Operation {
    add,
    list,
    view,
    remove_report,
    update_threshold,
    filter,
    op_invalid
};

//FUNCTION PROTOTYPES
void manage_symlink(const char* district);
void check_dangling_symlink(const char* link_name);
void view_function(const char* district, const char* role, const char* user, const char* target_id_string);
void remove_report_function(const char* district, const char* role, const char* user, const char* target_id_string);
void update_threshold_function(const char* district, const char* role, const char* user, const char* threshold_str);
void filter_function(const char* district, const char* role, const char* user, int argc, char** argv);
int parse_condition(const char* input, char* field, char* op, char* value);
int match_condition(Record* r, const char* field, const char* op, const char* value);
static int compare_numeric(long long rec_val, long long cond_val, const char* op);
static int compare_string(const char* rec_val, const char* cond_val, const char* op);

//PERMISSIONS
void mode_to_string(mode_t mode, char* str) {
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

int check_permission(const char* filepath, const char* role, int require_read, int require_write) {
    struct stat st = { 0 };

    if (stat(filepath, &st) == -1) {
        perror("Error checking file permissions");
        return 0;
    }

    if (strcmp(role, "manager") == 0) {
        if (require_read && !(st.st_mode & S_IRUSR)) {
            fprintf(stderr, "Access Denied: Manager lacks read permission on %s\n", filepath);
            return 0;
        }
        if (require_write && !(st.st_mode & S_IWUSR)) {
            fprintf(stderr, "Access Denied: Manager lacks write permission on %s\n", filepath);
            return 0;
        }
        return 1;
    }
    else

        if (strcmp(role, "inspector") == 0) {
            if (require_read && !(st.st_mode & S_IRGRP)) {
                fprintf(stderr, "Access Denied: Inspector lacks read permission on %s\n", filepath);
                return 0;
            }
            if (require_write && !(st.st_mode & S_IWGRP)) {
                fprintf(stderr, "Access Denied: Inspector lacks write permission on %s\n", filepath);
                return 0;
            }
            return 1;
        }

    return 0;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void print_usage(const char* action) {
    printf("Usage:\n");
    if (action) {
        printf("  city_manager --role <role> --user <user_name> %s\n", action);
    }
    else {
        printf("  city_manager --role <role> --user <user_name> --<operation> <district_id>\n");
    }
}

//DIRECTORY
int district_exists(const char* district) {
    struct stat st = { 0 };
    if (stat(district, &st) == 0 && S_ISDIR(st.st_mode)) {
        //exists
        return 1;
    }

    return 0;
}

int directory_creation(const char* district) {
    struct stat st = { 0 };
    if (stat(district, &st) == -1) {
        if (mkdir(district, 0750) == -1) {
            perror("Failed to create directory");
            return -1;
        }

        if (chmod(district, 0750) == -1) {
            perror("Failed to set permissions");
            return -1;
        }
    }
    return 0;
}

//LOGS
void log_operation(const char* district, const char* role, const char* user, const char* action) {
    char filepath[256];
    char buffer[512];

    snprintf(filepath, sizeof(filepath), "%s/logged_district", district);

    struct stat st = { 0 };
    if (stat(filepath, &st) == 0) {
        if (!check_permission(filepath, role, 0, 1)) {
            printf("Warning: Unable to log operation due to insufficient permissions on %s\n", filepath);
            return;
        }
    }

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    chmod(filepath, 0644);

    if (fd == -1) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    int len = snprintf(buffer, sizeof(buffer), "[%ld] Role: %s | User: %s | Action: %s\n", (long)now, role, user, action);
    if (len > 0) {
        if (write(fd, buffer, len) == -1) {
            perror("Failed to write to log file");
        }
    }
    close(fd);
}

//ADD
void add_function(const char* district, const char* role, const char* inspector_name) {

    if (!district_exists(district)) {
        //create dir
        if (directory_creation(district) == -1) {
            fprintf(stderr, "Fatal: Could not initialize directory.\n");
            _exit(1);
        }

        //init ditrict.cfg
        char config_path[256];
        snprintf(config_path, sizeof(config_path), "%s/district.cfg", district);

        int cfg_fd = open(config_path, O_WRONLY | O_CREAT | O_EXCL, 0640);
        if (cfg_fd != -1) {
            write(cfg_fd, "1\n", 2); //1 is default threshhold value
            close(cfg_fd);
            chmod(config_path, 0640);
        }
        else {
            perror("Warning: Failed to create default district.cfg");
        }


    }
    //create symlink
    manage_symlink(district);

    //open reports.dat
    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);

    int report_fd = open(reports_path, O_RDWR | O_CREAT | O_APPEND, 0664);
    if (report_fd == -1) {
        perror("Fatal: Could not open reports.dat");
        _exit(1);
    }
    chmod(reports_path, 0664);

    //create record
    Record new_record;
    memset(&new_record, 0, sizeof(Record));

    int next_id = 0;
    struct stat st = { 0 };
    if (stat(reports_path, &st) == 0) {
        if (!check_permission(reports_path, role, 0, 1)) {
            _exit(1);
        }

        if (st.st_size > 0) {
            Record last_record;
            lseek(report_fd, -sizeof(Record), SEEK_END);
            read(report_fd, &last_record, sizeof(Record));
            next_id = last_record.id + 1;
        }
    }

    new_record.id = next_id;
    strncpy(new_record.inspector, inspector_name, MAX_NAME_LEN - 1);
    new_record.inspector[MAX_NAME_LEN - 1] = '\0';

    while (1) {
        printf("X: ");
        if (scanf("%f", &new_record.latitude) == 1) {
            clear_input_buffer();
            break;
        }
        printf("Invalid input. Please enter a valid number.\n");
        clear_input_buffer();
    }

    while (1) {
        printf("Y: ");
        if (scanf("%f", &new_record.longitude) == 1) {
            clear_input_buffer();
            break;
        }
        printf("Invalid input. Please enter a valid number.\n");
        clear_input_buffer();
    }

    while (1) {
        printf("Category (road/lighting/flooding/other): ");
        scanf(" %19s", new_record.category);
        clear_input_buffer();

        if (strcmp(new_record.category, "road") == 0 ||
            strcmp(new_record.category, "lighting") == 0 ||
            strcmp(new_record.category, "flooding") == 0 ||
            strcmp(new_record.category, "other") == 0) {
            break;
        }
        printf("Invalid category. Must be one of the specified options.\n");
    }

    while (1) {
        printf("Severity level (1/2/3): ");
        if (scanf("%i", &new_record.severity) == 1 &&
            (new_record.severity >= 1 && new_record.severity <= 3)) {
            clear_input_buffer();
            break;
        }
        printf("Invalid severity. Please enter 1, 2, or 3.\n");
        clear_input_buffer();
    }

    new_record.timestamp = time(NULL);

    while (1) {
        printf("Description: ");
        if (scanf(" %111[^\n]", new_record.description) == 1) {
            clear_input_buffer();
            break;
        }
        printf("Description cannot be empty. Please enter details.\n");
        clear_input_buffer();
    }

    if (write(report_fd, &new_record, sizeof(Record)) == -1) {
        perror("Error: Could not write to reports.dat");
        close(report_fd);
        _exit(1);
    }
    close(report_fd);
    log_operation(district, role, inspector_name, "add");
    printf("Report ID %d successfully added!\n", new_record.id);
}

//LIST
void list_function(const char* district, const char* role, const char* user) {

    if (!district_exists(district)) {
        fprintf(stderr, "Error: District '%s' does not exist.\n", district);
        return;
    }

    //symlink warning
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    check_dangling_symlink(link_name);


    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);
    check_dangling_symlink(reports_path);

    struct stat st = { 0 };
    if (stat(reports_path, &st) == -1) {
        perror("Error: Could not stat reports.dat");
        return;
    }

    if (!check_permission(reports_path, role, 1, 0)) {
        return;
    }

    char perms[11];
    mode_to_string(st.st_mode, perms);

    char time_str[64];
    struct tm* tm_info = localtime(&st.st_mtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("\n=== District: %s ===\n", district);
    printf("File Info: %s | Size: %ld bytes | Last Modified: %s\n", perms, (long)st.st_size, time_str);
    printf("\n-----------------------------------------------------------\n");

    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening reports.dat for reading");
        return;
    }

    Record current_record;
    int count = 0;

    while (read(fd, &current_record, sizeof(Record)) == sizeof(Record)) {
        printf("[ID: %d] %s | Sev: %d | Cat: %s | GPS: (%.4f,%.4f)\n",
            current_record.id,
            current_record.inspector,
            current_record.severity,
            current_record.category,
            current_record.latitude,
            current_record.longitude);
        printf("Description: %s\n", current_record.description);
        printf("-----\n");
        count++;
    }

    if (count == 0) {
        printf("No reports found in this district.\n");
    }
    printf("Total records: %d\n", count);
    printf("==========================\n\n");

    close(fd);
    log_operation(district, role, user, "list");
}

//VIEW
void view_function(const char* district, const char* role, const char* user, const char* target_id_string) {
    int target_id = atoi(target_id_string);

    if (!district_exists(district)) {
        fprintf(stderr, "Error: District '%s' does not exist.\n", district);
        return;
    }

26    //symlink warning
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    check_dangling_symlink(link_name);

    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);
    check_dangling_symlink(reports_path);

    if (!check_permission(reports_path, role, 1, 0)) {
        return;
    }

    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening reports.dat");
        return;
    }

    Record current_record;
    int found = 0;

    while (read(fd, &current_record, sizeof(Record)) == sizeof(Record)) {
        if (current_record.id == target_id) {
            printf("\n=== Full Report Details (ID: %d) ===\n", current_record.id);
            printf("Inspector: %s\n", current_record.inspector);

            char time_str[64];
            struct tm* tm_info = localtime(&current_record.timestamp);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("Date/Time: %s\n", time_str);

            printf("GPS Coordinates: %.4f, %.4f\n", current_record.latitude, current_record.longitude);
            printf("Category: %s\n", current_record.category);
            printf("Severity: %d\n", current_record.severity);
            printf("Description:\n  %s\n", current_record.description);
            printf("==============================================\n\n");

            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Error: Report with ID %d not found in district '%s'.\n", target_id, district);
    }

    close(fd);
    log_operation(district, role, user, "view");
}

//REMOVE
void remove_report_function(const char* district, const char* role, const char* user, const char* target_id_string) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Access Denied: Only managers can remove reports.\n");
        return;
    }

    int target_id = atoi(target_id_string);

    if (!district_exists(district)) {
        fprintf(stderr, "Error: District '%s' does not exist.\n", district);
        return;
    }

    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);

    if (!check_permission(reports_path, role, 1, 1)) {
        return;
    }

    int fd = open(reports_path, O_RDWR);
    if (fd == -1) {
        perror("Error opening reports.dat");
        return;
    }

    Record current_record;
    int found_index = -1;
    int current_index = 0;

    while (read(fd, &current_record, sizeof(Record)) == sizeof(Record)) {
        if (current_record.id == target_id) {
            found_index = current_index;
            break;
        }
        current_index++;
    }

    if (found_index == -1) {
        printf("Error: Report with ID %d not found in district '%s'.\n", target_id, district);
        close(fd);
        return;
    }

    off_t read_pos = (found_index + 1) * sizeof(Record);
    off_t write_pos = found_index * sizeof(Record);

    Record temp_record;
    while (1) {
        lseek(fd, read_pos, SEEK_SET);
        if (read(fd, &temp_record, sizeof(Record)) != sizeof(Record)) {
            break;
        }

        lseek(fd, write_pos, SEEK_SET);
        if (write(fd, &temp_record, sizeof(Record)) == -1) {
            perror("Error writing to reports.dat during removal");
            close(fd);
            return;
        }

        read_pos += sizeof(Record);
        write_pos += sizeof(Record);
    }

    struct stat st = { 0 };
    fstat(fd, &st);

    if (ftruncate(fd, st.st_size - sizeof(Record)) == -1) {
        perror("Error truncating file");
    }
    else {
        printf("Report with ID %d successfully removed from district '%s'.\n", target_id, district);
    }

    close(fd);
    log_operation(district, role, user, "remove_report");
}

//UPDATE THRESHOLD
void update_threshold_function(const char* district, const char* role, const char* user, const char* threshold_str) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Access Denied: Only managers can update the threshold.\n");
        return;
    }
    int new_threshold = atoi(threshold_str);
    if (new_threshold < 1) {
        fprintf(stderr, "Invalid threshold value. Must be a positive integer.\n");
        return;
    }

    if (!district_exists(district)) {
        fprintf(stderr, "Error: District '%s' does not exist.\n", district);
        return;
    }

    char config_path[256];
    snprintf(config_path, sizeof(config_path), "%s/district.cfg", district);

    struct stat st = { 0 };
    if (stat(config_path, &st) == -1) {
        perror("Error: Could not stat district.cfg");
        return;
    }

    if ((st.st_mode & 0777) != 0640) {
        fprintf(stderr, "Error: district.cfg has incorrect permissions. Expected 0640.\n");
        return;
    }

    if (!check_permission(config_path, role, 1, 1)) {
        return;
    }

    int fd = open(config_path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Error opening district.cfg");
        return;
    }

    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", new_threshold);

    if (write(fd, buffer, len) == -1) {
        perror("Error writing new threshold to district.cfg");
    }
    else {
        printf("Threshold successfully updated to %d for district '%s'.\n", new_threshold, district);
    }

    close(fd);
    log_operation(district, role, user, "update_threshold");
}

//FILTER
//AI created functions

/**
 * Parses a condition string formatted as "field:operator:value"
 * * @param input The input string to parse.
 * @param field Buffer to store the extracted field name.
 * @param op    Buffer to store the extracted operator.
 * @param value Buffer to store the extracted value.
 * @return      1 on success, 0 on failure (e.g., malformed string or null pointers).
 */
int parse_condition(const char* input, char* field, char* op, char* value) {
    // Null pointer safety check
    if (!input || !field || !op || !value) {
        return 0;
    }

    // Find the first colon
    const char* first_colon = strchr(input, ':');
    if (!first_colon) {
        return 0; // Missing first colon
    }

    // Find the second colon, searching from the character after the first colon
    const char* second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) {
        return 0; // Missing second colon
    }

    // Calculate lengths of the field and operator substrings
    size_t field_len = first_colon - input;
    size_t op_len = second_colon - (first_colon + 1);

    // Extract the field and explicitly null-terminate
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    // Extract the operator and explicitly null-terminate
    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    // Extract the value (copies everything from the second colon to the null terminator)
    strcpy(value, second_colon + 1);
    return 1;

}

/**
 * Helper function to evaluate numeric comparisons.
 * Casts values to long long to safely handle both int (severity) and time_t (timestamp).
 */

static int compare_numeric(long long rec_val, long long cond_val, const char* op) {

    if (strcmp(op, "==") == 0) return rec_val == cond_val;
    if (strcmp(op, "!=") == 0) return rec_val != cond_val;
    if (strcmp(op, "<") == 0) return rec_val < cond_val;
    if (strcmp(op, "<=") == 0) return rec_val <= cond_val;
    if (strcmp(op, ">") == 0) return rec_val > cond_val;
    if (strcmp(op, ">=") == 0) return rec_val >= cond_val;
    return 0; // Unknown operator

}


/**
 * Helper function to evaluate string comparisons.
 * Uses standard lexicographical evaluation via strcmp.
 */

static int compare_string(const char* rec_val, const char* cond_val, const char* op) {

    int cmp = strcmp(rec_val, cond_val);

    if (strcmp(op, "==") == 0) return cmp == 0;
    if (strcmp(op, "!=") == 0) return cmp != 0;
    if (strcmp(op, "<") == 0) return cmp < 0;
    if (strcmp(op, "<=") == 0) return cmp <= 0;
    if (strcmp(op, ">") == 0) return cmp > 0;
    if (strcmp(op, ">=") == 0) return cmp >= 0;
    return 0; // Unknown operator
}

/**
 * Evaluates if a record matches a given condition.
 * * @param r     Pointer to the Record.
 * @param field The field to evaluate (severity, category, inspector, timestamp).
 * @param op    The comparison operator (==, !=, <, <=, >, >=).
 * @param value The value to compare against, represented as a string.
 * @return      1 if the record satisfies the condition, 0 otherwise.
 */

int match_condition(Record* r, const char* field, const char* op, const char* value) {
    if (!r || !field || !op || !value) {
        return 0; // Safety guard
    }

    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        return compare_numeric((long long)r->severity, (long long)val, op);
    }
    else if (strcmp(field, "timestamp") == 0) {
        // Using strtoll for safe conversion to 64-bit integers often used by time_t
        long long val = strtoll(value, NULL, 10);
        return compare_numeric((long long)r->timestamp, val, op);
    }
    else if (strcmp(field, "category") == 0) {
        return compare_string(r->category, value, op);
    }
    else if (strcmp(field, "inspector") == 0) {
        return compare_string(r->inspector, value, op);
    }
    // Unsupported field
    return 0;

}
//End of AI created functions

void filter_function(const char* district, const char* role, const char* user, int argc, char** argv) {
    if (!district_exists(district)) {
        fprintf(stderr, "Error: District '%s' does not exist.\n", district);
        return;
    }

    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);

    if (!check_permission(reports_path, role, 1, 0)) {
        return;
    }

    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening reports.dat");
        return;
    }

    Record current_record;
    int count = 0;

    printf("\n=== Filtered Reports for District: %s ===\n", district);

    while (read(fd, &current_record, sizeof(Record)) == sizeof(Record)) {
        int is_match = 1;

        for (int i = 7; i < argc; i++) {
            char field[32] = { 0 }, op[4] = { 0 }, value[128] = { 0 };
            if (!parse_condition(argv[i], field, op, value)) {
                fprintf(stderr, "Error: Malformed condition '%s'\n", argv[i]);
                close(fd);
                return;
            }

            if (!match_condition(&current_record, field, op, value)) {
                is_match = 0;
                break;
            }
        }

        if (is_match) {
            printf("[ID: %d] %s | Sev: %d | Cat: %s | GPS: (%.4f,%.4f)\n",
                current_record.id,
                current_record.inspector,
                current_record.severity,
                current_record.category,
                current_record.latitude,
                current_record.longitude);
            printf("Description: %s\n", current_record.description);
            printf("-----\n");
            count++;
        }
    }

    if (count == 0) {
        printf("No reports match the specified conditions.\n");
    }
    else {
        printf("Total matching records: %d\n", count);
    }
    printf("==========================\n\n");

    close(fd);
    log_operation(district, role, user, "filter");
}

//SYMLINK
void manage_symlink(const char* district) {
    char link_name[256];
    char target_path[256];

    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    snprintf(target_path, sizeof(target_path), "%s/reports.dat", district);

    struct stat st = { 0 };

    if (lstat(link_name, &st) == -1) {
        if (symlink(target_path, link_name) == -1) {
            perror("Error creating symlink");
        }
        else {
            printf("Symlink '%s' created pointing to '%s'\n", link_name, target_path);
        }
    }
}

void check_dangling_symlink(const char* link_name) {
    struct stat link_st = { 0 };
    struct stat target_st = { 0 };

    if (lstat(link_name, &link_st) == 0) {
        if (S_ISLNK(link_st.st_mode)) {
            if (stat(link_name, &target_st) == -1) {
                if (errno == ENOENT) {
                    fprintf(stderr, "Warning: The symlink '%s' is dangling (its target file is missing).", link_name);
                    unlink(link_name);
                }
                else {
                    perror("Warning: Error accessing symlink target");
                }
            }
        }
    }
}

//MAIN
int main(int argc, char** argv) {
    if (argc < 7) {
        print_usage(NULL);
        return 1;
    }

    if (strcmp(argv[1], "--role") != 0) {
        print_usage(NULL);
        return 1;
    }

    if (strcmp(argv[2], "inspector") != 0 && strcmp(argv[2], "manager") != 0) {
        printf("Role %s does not exist.\n", argv[2]);
        return 1;
    }

    const char* role = argv[2];

    if (strcmp(argv[3], "--user") != 0) {
        print_usage(NULL);
        return 1;
    }

    const char* user = argv[4];
    const char* ops = argv[5];
    const char* district_id = argv[6];

    enum Operation op;
    if (strcmp(ops, "--add") == 0) {
        op = add;
    }
    else if (strcmp(ops, "--list") == 0) {
        op = list;
    }
    else if (strcmp(ops, "--view") == 0) {
        op = view;
    }
    else if (strcmp(ops, "--remove_report") == 0) {
        op = remove_report;
    }
    else if (strcmp(ops, "--update_threshold") == 0) {
        op = update_threshold;
    }
    else if (strcmp(ops, "--filter") == 0) {
        op = filter;
    }
    else op = op_invalid;

    switch (op) {
    case add: {
        if (argc < 7) {
            print_usage("--add <district_id>");
            return 1;
        }
        add_function(district_id, role, user);
        break;
    }
    case list: {
        if (argc != 7) {
            print_usage("--list <district_id>");
            return 1;
        }
        list_function(district_id, role, user);
        break;
    }
    case view: {
        if (argc != 8) {
            print_usage("--view <district_id> <report_id>");
            return 1;
        }
        view_function(district_id, role, user, argv[7]);
        break;
    }
    case remove_report: {
        if (argc != 8) {
            print_usage("--remove_report <district_id> <report_id>");
            return 1;
        }
        remove_report_function(district_id, role, user, argv[7]);
        break;
    }
    case update_threshold: {
        if (argc != 8) {
            print_usage("--update_threshold <district_id> <value>");
            return 1;
        }
        update_threshold_function(district_id, role, user, argv[7]);
        break;
    }
    case filter: {
        if (argc < 8) {
            print_usage("--filter <district_id> <condition>");
            return 1;
        }
        filter_function(district_id, role, user, argc, argv);
        break;
    }
    default:
        printf("Operation %s does not exist.\n", ops);
        return 1;
    }

    return 0;
}
