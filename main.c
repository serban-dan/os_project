#include <stdio.h>
#include <string.h>

enum Operation{
    add,
    list,
    view,
    remove_report,
    update_threshold,
    filter
};

static void print_usage() {
    printf("Usage:\n");
    printf("  city_manager --role <role> --user <user_name> --<operation>\n");
}

int main(int argc, char **argv) {
    if (argc < 6) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--role") != 0) {
        print_usage();
        return 1;
    }

    if(strcmp(argv[2],"inspector") != 0 && strcmp(argv[2],"manager") != 0){
        printf("Role %s does not exist.\n",argv[2]);
        return 1;
    }

    const char *role = argv[2];

    if (strcmp(argv[3], "--user") != 0) {
        print_usage();
        return 1;
    }

    const char *user = argv[4];
    const char *ops = argv[5];
    
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
            const char *district_id = argv[6];
            printf("Add to %s.\n",district_id);
            break;
        }
        case list:
            printf("To do list\n");
            break;
        case view:
            printf("To do view\n");
            break;
        case remove_report:
            printf("To do remove\n");
            break;
        case update_threshold:
            printf("To do update\n");
            break;
        case filter:
            printf("To do filter\n");
            break;
        default:
            printf("Operation %s does not exist.\n",ops);
            return 1;
    }

    return 0;
}