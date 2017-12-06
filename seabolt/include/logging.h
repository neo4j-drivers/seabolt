//
// Created by technige on 06/12/17.
//

#ifndef SEABOLT_LOGGING
#define SEABOLT_LOGGING

#define log(message, args...) fprintf(stderr, "\x1B[36m"); fprintf(stderr, message, ##args); fprintf(stderr, "\x1B[0m\n");
//#define log(message, args...)

#define log_error(message, args...) fprintf(stderr, "\x1B[33m"); fprintf(stderr, message, ##args); fprintf(stderr, "\x1B[0m\n");


#endif // SEABOLT_LOGGING
