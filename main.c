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
    if (argc < 5) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--role") != 0) {
        print_usage();
        return 1;
    }

    const char *role = argv[2];

    if (strcmp(argv[3], "--user") != 0) {
        print_usage();
        return 1;
    }

    const char *user = argv[4];
    const char *ops = argv[5];
    printf("op = %s\n",ops);
    
    enum Operation op;
    if(strcmp(ops,"--add") == 0){
        enum Operation op = add;
    }else if(strcmp(ops,"--list") == 0){
        enum Operation op = list;
    }else if(strcmp(ops,"--view") == 0){
        enum Operation op = view;
    }else if(strcmp(ops,"--remove_report") == 0){
        enum Operation op = remove_report;
    }else if(strcmp(ops,"--update_threshold") == 0){
        enum Operation op = update_threshold;
    }else if(strcmp(ops,"--filter") == 0){
        enum Operation op = filter;
    }else op = -1;

    printf("enum = %i\n", op);

    switch (op) {
        case add:
            const char *district_id = argv[6];
            printf("Add to %s.\n",district_id);
            break;
        case list:
            printf("To do list");
            break;
        case view:
            printf("To do view");
            break;
        case remove_report:
            printf("To do remove");
            break;
        case update_threshold:
            printf("To do update");
            break;
        case filter:
            printf("To do filter");
            break;
        default:
            printf("Operation %s does not exist.\n",ops);
    }

    return 0;
}