#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef enum {
    UNCLASSIFIED = 0, 
    CONFIDENTIAL = 1,
    SECRET       = 2,
    TOP_SECRET   = 3,
    LEVEL_INVALID = -1
} level_t;

level_t parse_level(const char *s){ 
    //파일 이름 파싱 -> 레벨 연결 
    //get_user_clearance에서 쓰는용

    if (strcmp(s, "UNCLASSIFIED")==0) return UNCLASSIFIED;
    else if (strcmp(s, "CONFIDENTIAL")==0) return CONFIDENTIAL;
    else if (strcmp(s, "SECRET")==0) return SECRET;
    else if (strcmp(s, "TOP_SECRET")==0) return TOP_SECRET;
    else return LEVEL_INVALID;
    
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
    if (setegid(getgid())!=0){
        perror("setegid error");
        exit(1);
    }
    if (seteuid(getuid())!=0){
        perror("seteuid error");
        exit(1);
    }
}

//유저의 보안 등급 파악
level_t get_user_clearance(const char *username){
    FILE *fp = fopen("mac.policy", "r");
    if (fp == NULL){
        return LEVEL_INVALID;
    }

    char line[256];
    level_t result = LEVEL_INVALID;

    while (fgets(line, sizeof(line), fp)!=NULL){ //문서 끝까지 루프
        line[strcspn(line, "\r\n")] = '\0'; //개행문자 제거

        char *name = strtok(line, ":");
        char *level_str = strtok(NULL, ":");

        if (name == NULL || level_str == NULL) continue;

        if (strcmp(name, username) == 0){
            result = parse_level(level_str);
            break;
        }
    }

    return result;
}

level_t get_file_clearance(const char *filename){

    if (strcmp(filename, "secret.data")==0){
        return SECRET;
    }
    else if (strcmp(filename, "top_secret.data") == 0){
        return TOP_SECRET;
    }
    else if (strcmp(filename, "unclassified.data")==0){
        return UNCLASSIFIED;
    }
    else if (strcmp(filename, "confidential.data")==0){
        return CONFIDENTIAL;
    } 
    else {
        return LEVEL_INVALID;
    }
}

int do_read(const char*filename){
    FILE *fp = fopen(filename, "r");
    if (fp == NULL){
        perror("fopen");
        return -1;
    }

    char buf[1024];
    size_t n;
    while ((n=fread(buf, 1, sizeof(buf), fp))>0){
        fwrite(buf, 1, n, stdout);
    } 
    fclose(fp);
    return 0;
}

int do_write(const char*filename, const char*data){
    FILE *fp = fopen(filename, "a"); //w로 열면 기존 내용 덮어씀! 반드시 append (뒤에 추가))
    if (fp == NULL){
        perror("fopen");
        return -1;
    }

    fprintf(fp, "%s\n", data);
    fclose(fp);
    return 0;
}

void logging(const char*username, const char*command, const char*filename){
    umask(0027); //0666 & -0027 -> 0640 보장
    char logpath[256];
    snprintf(logpath, sizeof(logpath), "%s.log", username);

    int fd = open(logpath, O_WRONLY|O_CREAT|O_APPEND, 0640);
    if (fd < 0) { //파일 없으면 에러 문자 출력
        perror("open log");
        exit(1);
    }

    dprintf(fd, "%s %s\n", command, filename); //한줄 기록

    close(fd);
}

int main(int argc, char *argv[]){ //argc : 인자개수, argv : 인자배열
    if (argc < 3){ //인자 부족하면 반환
        return 1;
    }

    const char *command = argv[1];
    const char *filename = argv[2];
    const char *data = NULL;

    if (strcmp(command, "read")==0){
        if (argc!=3){ //read이면 인자가 3개여야함, 아니면 오류
            return 1;
        }
    } 
    else if (strcmp(command, "write") == 0){
        if (argc!=4){ //write이면 인자가 4개여야함, 아니면 오류
            return 1;
        }
        data = argv[3];
    }
    else{
        return 1; //아예 오류
    }

    //현재 username 얻기, 실패시 리턴
    const char *username = get_current_username();
    if (username == NULL){
        return 1;
    }

    //유저 보안 등급 조회
    level_t user_level = get_user_clearance(username);
    //파일 보안 등급 조회
    level_t file_level = get_file_clearance(filename);
    
    //접근여부 판단
    if (user_level == LEVEL_INVALID){
        printf("ACCESS DENIED\n"); // invalid 한 유저는 권한 비교 전에 아예 차단
    }
    else if (strcmp(command, "read") == 0){
        if (user_level >= file_level){
            do_read(filename);
        }
        else{
            printf("ACCESS DENIED\n");
        }
    }
    else if (strcmp(command, "write") == 0){
        if (user_level <= file_level){
            do_write(filename, data);
        }
        else {
            printf("ACCESS DENIED\n");
        }
    }

    //권한 드롭
    drop_privilege();

    //로그 기록
    logging(username, command, filename);

    return 0;
}
