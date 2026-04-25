#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

enum Operation{
    add,
    list,
    view,
    remove_report,
    update_threshold,
    filter
};

void mode_to_string(mode_t mode, char *str) {
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

static void print_usage(const char *action) {
    printf("Usage:\n");
    if (action) {
        printf("  city_manager --role <role> --user <user_name> %s\n", action);
    } else {
        printf("  city_manager --role <role> --user <user_name> --<operation> <district_id>\n");
    }
}

int district_exists(const char *district) {
    struct stat st = {0};
    if(stat(district, &st) == 0 && S_ISDIR(st.st_mode)){
        return 1;
    }

    return 0;
}

int directory_creation(const char *district) {
    struct stat st = {0};
    if(stat(district, &st) == -1){
        if(mkdir(district, 0750) == -1){
            perror("Failed to create directory");
            return -1;
        }

        if(chmod(district, 0750) == -1){
            perror("Failed to set permissions");
            return -1;
        }
        return 0;
    }else {
        if(!S_ISDIR(st.st_mode)){
            fprintf(stderr, "%s exists but is not a directory.\n", district);
            return -1;
        }
        return 0;
    }
}

void log_operation(const char *district,const char *role, const char *user, const char *action){
    char filepath[256];
    char buffer[512];

    snprintf(filepath, sizeof(filepath), "%s/", district);
}

int main(int argc, char **argv) {
    if (argc < 7) {
        print_usage(NULL);
        return 1;
    }

    if (strcmp(argv[1], "--role") != 0) {
        print_usage(NULL);
        return 1;
    }

    if(strcmp(argv[2],"inspector") != 0 && strcmp(argv[2],"manager") != 0){
        printf("Role %s does not exist.\n",argv[2]);
        return 1;
    }

    const char *role = argv[2];

    if (strcmp(argv[3], "--user") != 0) {
        print_usage(NULL);
        return 1;
    }

    const char *user = argv[4];
    const char *ops = argv[5];
    const char *district_id = argv[6];
    
    enum Operation op;
    if(strcmp(ops,"--add") == 0){
        op = add;
    }else if(strcmp(ops,"--list") == 0){
        op = list;
    }else if(strcmp(ops,"--view") == 0){
        op = view;
    }else if(strcmp(ops,"--remove_report") == 0){
        op = remove_report;
    }else if(strcmp(ops,"--update_threshold") == 0){
        op = update_threshold;
    }else if(strcmp(ops,"--filter") == 0){
        op = filter;
    }else op = -1;

    switch (op) {
        case add: {
            printf("Add to %s.\n",district_id);
            if(argc != 7){
                print_usage("--add <district_id>");
                return 1;
            }
            break;
        }
        case list:{
            if(argc != 7){
                print_usage("--list <district_id>");
                return 1;
            }
            printf("To do list\n");
            break;
        }
        case view:{
            if(argc != 8){
                print_usage("--view <district_id> <report_id>");
                return 1;
            }
            printf("To do view\n");
            break;
        }
        case remove_report:{
            if(argc != 8){
                print_usage("--remove_report <district_id> <report_id>");
                return 1;
            }
            printf("To do remove\n");
            break;
        }
        case update_threshold:{
            if(argc != 8){
                print_usage("--update_threshold <district_id> <value>");
                return 1;
            }
            printf("To do update\n");
            break;
        }
        case filter:{
            if(argc != 8){
                print_usage("--filter <district_id> <condition>");
                return 1;
            }
            printf("To do filter\n");
            break;
        }
        default:
            printf("Operation %s does not exist.\n",ops);
            return 1;
    }

    return 0;
}