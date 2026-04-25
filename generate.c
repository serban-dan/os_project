#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MAX_NAME_LEN 52
#define MAX_CAT_LEN 20
#define MAX_DESC_LEN 112

typedef struct Record {
    int id;
    char inspector[MAX_NAME_LEN];
    float latitude;
    float longitude;
    char category[MAX_CAT_LEN];
    int severity;
    time_t timestamp;
    char description[MAX_DESC_LEN];
} Record;

void fill_record(Record *r, int id, const char *inspector, float lat, float lon,
                 const char *cat, int sev, const char *desc) {
    memset(r, 0, sizeof(Record));
    r->id = id;
    strncpy(r->inspector, inspector, MAX_NAME_LEN - 1);
    r->latitude = lat;
    r->longitude = lon;
    strncpy(r->category, cat, MAX_CAT_LEN - 1);
    r->severity = sev;
    r->timestamp = time(NULL) - (id * 3600); // Fake different timestamps (1 hour apart)
    strncpy(r->description, desc, MAX_DESC_LEN - 1);
}

int main() {
    int fd = open("reports.dat", O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) { perror("Error"); return 1; }

    Record r;
    fill_record(&r, 0, "alice", 45.7532, 21.2284, "road", 2, "Deep pothole in the right lane.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 1, "bob", 45.7610, 21.2155, "flooding", 3, "Water main break. Street flooded.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 2, "charlie", -12.345, 0.0000, "other", 1, "Graffiti on the stop sign.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 3, "diana", 99.999, -180.00, "lighting", 2, "Traffic light completely out at intersection.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 4, "eve", 45.1111, 21.2222, "road", 1, "Slight crack forming on the sidewalk.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 5, "frank", 45.7500, 21.2500, "flooding", 2, "Clogged drain causing massive puddles.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 6, "grace", 45.7532, 21.2284, "road", 3, "Bridge expansion joint looks severely damaged.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 7, "alice", 45.8888, 21.8888, "lighting", 1, "Streetlight flickering continuously.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 8, "bob", 0.0000, 0.0000, "other", 3, "Downed power line resting on a parked car.");
    write(fd, &r, sizeof(Record));

    fill_record(&r, 9, "charlie", 45.7777, 21.3333, "road", 2, "Sinkhole opening up near the curb.");
    write(fd, &r, sizeof(Record));

    close(fd);
    printf("Successfully generated reports.dat with 10 records (Total size: %zu bytes)\n", 10 * sizeof(Record));
    return 0;
}
