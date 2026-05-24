#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>


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

typedef struct {
    char name[MAX_NAME_LEN];
    int total_score;
} inspector_score;

#define MAX_INSPECTORS 100

int  main(const int argc, const char** argv){
    if(argc != 2){
        fprintf(stderr, "Usage: ./scorer <district_id>\n");
        return 1;
    }

    const char* district = argv[1];
    char reports_path[256];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district);

    int fd = open(reports_path, O_RDONLY);
    if(fd == -1){
        printf("District '%s' does not exist or has no reports.\n", district);
        return 1;
    }

    inspector_score scores[MAX_INSPECTORS];
    int inspector_count = 0;

    Record current_record;

    while(read(fd, &current_record, sizeof(Record)) == sizeof(Record)){

        int found = 0;
        for(int i = 0; i < inspector_count; i++){
            if(strcmp(scores[i].name, current_record.inspector) == 0){
                scores[i].total_score += current_record.severity;
                found = 1;
                break;
            }
        }

        // If not found, add new inspector
        if(!found && inspector_count < MAX_INSPECTORS){
            strncpy(scores[inspector_count].name, current_record.inspector, MAX_NAME_LEN);
            scores[inspector_count].total_score = current_record.severity;
            inspector_count++;
        }
    }

    close(fd);

    // Print the scores
    if(inspector_count == 0){
        printf("No reports found in district '%s'.\n", district);
    }
    else{
        printf("Inspector Scores for District '%s':\n", district);
        for(int i = 0; i < inspector_count; i++){
            printf("%s: %d\n", scores[i].name, scores[i].total_score);
        }
    }
    return 0;
}