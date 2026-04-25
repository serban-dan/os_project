#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define MAX_NAME_LEN 52
#define MAX_CAT_LEN 20
#define MAX_DESC_LEN 112

typedef struct Record{
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
    filter
};

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
    struct stat st = {0};
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

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
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
        }
        else {
            perror("Warning: Failed to create default district.cfg");
        }
    }

    //open reports.dat
    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);

    int report_fd = open(reports_path, O_RDWR | O_CREAT | O_APPEND, 0664);
    if (report_fd == -1) {
        perror("Fatal: Could not open reports.dat");
        _exit(1);
    }

    printf("add %s\n",district);
    //create record
    Record new_record;
    memset(&new_record, 0, sizeof(Record));

    int next_id = 0;
    struct stat st = {0};
    if (stat(reports_path, &st) == 0 && st.st_size > 0){
        Record last_record;
        lseek(report_fd, -sizeof(Record), SEEK_END);
        read(report_fd, &last_record, sizeof(Record));
        next_id = last_record.id + 1;
    }

    new_record.id = next_id;
    strcpy(new_record.inspector,inspector_name);

    printf("X: ");
    scanf("%f",&new_record.latitude);

    printf("Y: ");
    scanf("%f",&new_record.longitude);

    printf("Category (road/lighting/flooding/other): ");
    scanf(" %[^\n]",new_record.category);

    printf("Severity level (1/2/3): ");
    scanf("%i",&new_record.severity);

    new_record.timestamp = time(NULL);

    printf("Description: ");
    scanf(" %[^\n]",new_record.description);

    write(report_fd, &new_record,sizeof(Record));
    close(report_fd);
    log_operation(district,role,inspector_name,"add");
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
    else op = -1;

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
        printf("To do list\n");
        break;
    }
    case view: {
        if (argc != 8) {
            print_usage("--view <district_id> <report_id>");
            return 1;
        }
        printf("To do view\n");
        break;
    }
    case remove_report: {
        if (argc != 8) {
            print_usage("--remove_report <district_id> <report_id>");
            return 1;
        }
        printf("To do remove\n");
        break;
    }
    case update_threshold: {
        if (argc != 8) {
            print_usage("--update_threshold <district_id> <value>");
            return 1;
        }
        printf("To do update\n");
        break;
    }
    case filter: {
        if (argc < 8) {
            print_usage("--filter <district_id> <condition>");
            return 1;
        }
        printf("To do filter\n");
        break;
    }
    default:
        printf("Operation %s does not exist.\n", ops);
        return 1;
    }

    return 0;
}
