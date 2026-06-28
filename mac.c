#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

typedef enum {
    UNCLASSIFIED = 0, 
    CONFIDENTIAL = 1,
    SECRET       = 2,
    TOP_SECRET   = 3,
    LEVEL_INVALID = -1
} level_t;

level_t parse_level(const char *s){

}

//현재 실행중인 유저의 username 얻기
const char *get_current_username(void){
    uid_t uid = getuid(); //유저의 real uid 얻기
    struct passwd *pw = getpwuid(uid); //uid로 passwd 파일의 entry 조회
    if (pw == NULL){
        return NULL; //조회 실패시
    }
    return pw->pw_name; //pw에서 username 문자열 반환
}

void drop_privilege(void){
    uid_t e_uid = geteuid(); //유저의 effective uid 얻기
    if (setegid(getgid())!=0){
        perrror("setegid error");
        exit(1);
    }
    if (seteuid(getuid())!=0){
        perrror("seteuid error");
        exit(1);
    }
}

int main(int argc, char *argv[]){ //argc : 인자개수, argv : 인자배열
    if (argc < 3){ //인자 부족하면 반환
        return 1;
    }

    const char *command = argv[1];
    const char *filename = argv[2];

    if (strcmp(command, "read")==0){
        if (argv!=3){ //read이면 인자가 3개여야함, 아니면 오류
            return 1;
        }
    } 
    else if (strcmp(command, "write") == 0){
        if (argv!=4){ //write이면 인자가 4개여야함, 아니면 오류
            return 1;
        }
        const char *data = argv[3];
    }
    else{
        return 1; //아예 오류
    }

    //현재 username 얻기, 실패시 리턴
    const char *username = get_current_username();
    if (username == NULL){
        return 1;
    }

    drop_privilege();

    





    return 0;
}
