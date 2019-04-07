//
//  chatCommon.h
//  char Client
//
//  Created by Ilya Yelagov on 13/03/2019.
//  Copyright © 2019 Ilya Yelagov. All rights reserved.
//

#ifndef chatCommon_h
#define chatCommon_h

#define LENGTH_NAME 33
#define LENGTH_MSG 129
#define MSG_LEN 128
#define LENGTH_SEND 259
#define PAUSE 1
#define TRUE 1
#define FALSE 0
#define bool int

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void FixString (char* arr, int length) {    //treating /n as end char
    int i;
    for (i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}


#endif /* chatCommon_h */
