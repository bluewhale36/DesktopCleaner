#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096
#define MAX_RULES 50
#define CONFIG_FILE "rules.cfg"

// --- [파일 입출력 및 규칙 관리] ---------------------------------

// rules.cfg 파일에 규칙 저장
int save_rules_to_file();
// rule_list 배열에 규칙 추가
void add_rule(const char* extension, const char* folder_name);
// 기본 규칙 설정
void set_default_rules();
// 프로그램 시작 시 규칙 초기화
void init_rules();

// --- [파일 처리 함수] ---------------------------------

// 파일 복사 진입점
int copy_file(const char* src_name, const char* dest_name);
// 파일 복사 작업 수행
int perform_copy(FILE* source_file, FILE* dest_file);
// 파일 이동/삭제 및 폴더 생성 작업 수행.
// base_dest_path : 정리된 폴더가 위치할 폴더 경로. ex) C:\Users\user\Desktop\Desktop_Clean\result
int move_and_organize(const char* filename, const char* base_dest_path, const char* category);
// 동적 디렉토리 생성 작업 수행
int dynamic_mkdir(const char* path);
// 파일 확장자 추출
const char* get_extension(const char* filename);
// 폴더 내 파일 탐색 및 정리 진입점
void clean_directory(const char* dest_path);

// 설정 메뉴 UI
void settings_menu();


// ANSI 방식의 콘솔 클리어 함수
void clear_console()
{
    printf("\033[H\033[2J");
}

typedef struct
{
    char ext[20];
    char folder[50];
} Rule;

Rule rule_list[MAX_RULES];
size_t rule_count = 0;

// 프로그램 시작 경로 저장 변수
char program_start_path[256];

int main() {
    char source_path[256];
    char dest_path[256];
    int choice;

    // 프로그램 시작 위치 저장
    _getcwd(program_start_path, sizeof(program_start_path));

    // 규칙 로드
    init_rules();

    while (1)
    {
        // 메인 메뉴 진입 시 항상 시작 위치로 복귀
        _chdir(program_start_path);

        printf("\n============================================\n");
        printf(" Desktop Cleaner v5.0 (Auto-Save)\n");
        printf("============================================\n");
        printf(" 1. 정리 시작하기\n");
        printf(" 2. 설정 (파일 저장됨)\n");
        printf(" 3. 종료\n");
        printf("============================================\n");
        printf(" 메뉴 선택 >> ");

        if (scanf("%d", &choice) != 1)
        {
            while (getchar() != '\n');
            choice = 0;
        }
        while (getchar() != '\n');

        if (choice == 3)
        {
            clear_console();
            printf("\n============================================\n");
            printf(" 프로그램을 종료합니다.\n");
            printf("============================================\n");
            break;
        }
        else if (choice == 2)
        {
            clear_console();
            settings_menu();
        }
        else if (choice == 1)
        {
            clear_console();

            printf("\n [1/2] 정리할 폴더 경로(원본) 입력 >> ");
            fgets(source_path, sizeof(source_path), stdin);
            source_path[strcspn(source_path, "\n")] = 0;    // fgets() 에서 입력된 개행문자 제거

            printf("\n [2/2] 파일들을 보낼 폴더 경로(목적지) 입력 >> ");
            fgets(dest_path, sizeof(dest_path), stdin);
            dest_path[strcspn(dest_path, "\n")] = 0;        // fgets() 에서 입력된 개행문자 제거

            if (strlen(dest_path) == 0) strcpy(dest_path, source_path);

            // 원본 경로로 이동하여 작업 수행
            if (_chdir(source_path) == 0)
            {
                clean_directory(dest_path);

                // 정리 후 원래 실행 경로로 복귀
                _chdir(program_start_path);
            }
            else
            {
                printf("\n 경로 오류: %s\n", source_path);
            }
        }
        else
        {
            clear_console();
            printf(" 잘못된 입력입니다.\n");
        }
    }
    return 0;
}

int save_rules_to_file()
{
    FILE* fp = fopen(CONFIG_FILE, "w");
    if (fp == NULL)
    {
        return 0;
    }

    for (int i = 0; i < rule_count; i++)
    {
        fprintf(fp, "%s %s\n", rule_list[i].ext, rule_list[i].folder);
    }
    fclose(fp);
    return 1;
}

void add_rule(const char* extension, const char* folder_name)
{
    if (rule_count >= MAX_RULES)
    {
        printf(" 규칙이 가득 찼습니다.\n");
        return;
    }
    strcpy(rule_list[rule_count].ext, extension);
    strcpy(rule_list[rule_count].folder, folder_name);
    rule_count++;
}

void set_default_rules()
{
    add_rule("jpg", "Images");
    add_rule("png", "Images");
    add_rule("txt", "Texts");
    add_rule("hwp", "Documents");
    add_rule("pdf", "Documents");
    add_rule("zip", "Archives");
    add_rule("mp3", "Musics");
    add_rule("mp4", "Videos");
}

void init_rules()
{
    FILE* fp = fopen(CONFIG_FILE, "r");

    // 파일이 없으면 기본값 생성 후 저장
    if (fp == NULL)
    {
        printf(" 설정 파일이 없어 기본값을 생성합니다.\n");
        set_default_rules();
        save_rules_to_file();
        return;
    }

    // 파일이 있으면 읽어오기
    char load_ext[20];
    char load_folder[50];
    rule_count = 0;

    while (fscanf(fp, "%s %s", load_ext, load_folder) != EOF)
    {
        if (rule_count >= MAX_RULES) break;
        add_rule(load_ext, load_folder);
    }
    fclose(fp);
    printf(" 설정 파일에서 %zu개의 규칙을 불러왔습니다.\n", rule_count);
}


int copy_file(const char* src_name, const char* dest_name)
{
    FILE* src = fopen(src_name, "rb");
    if (src == NULL)
    {
        return 0;
    }

    FILE* dest = fopen(dest_name, "wb");
    if (dest == NULL)
    {
        fclose(src);
        return 0;
    }

    int copy_result = perform_copy(src, dest);

    fclose(src);
    fclose(dest);
    return copy_result;
}

int perform_copy(FILE* source_file, FILE* dest_file)
{
    char buffer[BUFFER_SIZE];
    size_t read_count;

    // 파일 끝까지 반복
    while (!feof(source_file))
    {
        // 읽기
        read_count = fread(buffer, 1, BUFFER_SIZE, source_file);

        if (ferror(source_file))
        {
            return 0; // 실패
        }

        // 쓰기
        if (read_count > 0)
        {
            fwrite(buffer, 1, read_count, dest_file);

            if (ferror(dest_file))
            {
                return 0; // 실패
            }
        }
    }
    return 1; // 성공
}

int dynamic_mkdir(const char* path)
{
    char temp_path[512];
    char* p = NULL;
    size_t len;

    // 원본 경로를 복사하여 조작
    snprintf(temp_path, sizeof(temp_path), "%s", path);
    len = strlen(temp_path);

    // 경로 끝에 \ 또는 / 가 있다면 제거
    if (temp_path[len - 1] == '\\' || temp_path[len - 1] == '/')
    {
        temp_path[len - 1] = '\0';
    }

    // 문자열을 처음부터 훑으며 \ 또는 / 를 만날 때마다 폴더 확인 및 생성
    // p = temp_path + 1 부터 시작: 드라이브 명(C:\)이나 루트 경로 회피
    for (p = temp_path + 1; *p; p++)
    {
        if (*p == '\\' || *p == '/')
        {
            char tmp = *p;
            *p = '\0';  // 문자열 임시로 끊음

            // 끊어진 문자열까지의 경로가 존재하지 않을 경우(_access 반환값이 -1일 경우) 생성
            if (_access(temp_path, 0) != 0)
            {
                // 생성 실패 시(_mkdir 반환값이 -1일 경우) -1 반환
                if (_mkdir(temp_path) != 0)
                {
                    return -1;
                }
            }

            *p = tmp;   // 끊은 문자열 복구
        }
    }

    // 마지막 최종 경로 생성 확인
    if (_access(temp_path, 0) != 0)
    {
        return _mkdir(temp_path);
    }
    return 0;
}

int move_and_organize(const char* filename, const char* base_dest_path, const char* category)
{
    char final_folder[512]; // 복사본이 위치할 폴더 경로. ex) C:\Users\user\Desktop\Desktop_Clean\result\Documents
    char final_path[512];   // 복사본의 최종 경로. ex) C:\Users\user\Desktop\Desktop_Clean\result\Documents\copied_file.docx

    sprintf(final_folder, "%s\\%s", base_dest_path, category);

    // 동적 디렉토리 생성 실패 시 즉시 중단
    if (dynamic_mkdir(final_folder) != 0)
    {
        printf(" [실패] 디렉토리 생성 오류: %s\n", final_folder);
        return -2;  // 디렉토리 생성 오류
    }

    sprintf(final_path, "%s\\%s", final_folder, filename);

    if (copy_file(filename, final_path))
    {
        if (remove(filename) == 0)
        {
            printf(" [이동] %s -> [%s]\n", filename, category);
            return 0;   // 성공
        }
        else
        {
            printf(" [경고] 원본 삭제 실패: %s\n", filename);
            return 1;   // 파일 복사 성공, 원본 삭제 실패
        }
    }
    else
    {
        printf(" [실패] 복사 오류: %s\n", filename);
        return -1;  // 복사 실패
    }
}

const char* get_extension(const char* filename)
{
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1; // . 뒤의 확장자 첫글자 반환 (포인터 +1)
}

void clean_directory(const char* dest_path)
{
    struct _finddata_t file_info;   // 탐색하는 파일 메타데이터 저장 변수
    intptr_t handle;                // 검색 핸들

    // 파일 이동 성공 개수, 실패(디렉토리 생성, 파일 복사) 개수, 원본 삭제 실패 개수
    int success_count = 0, failed_count = 0, warning_count = 0;

    handle = _findfirst("*.*", &file_info);
    if (handle == -1)
    {
        printf("\n 파일이 없습니다.\n");
        return;
    }

    printf("\n [탐색 시작] 설정된 %zu개의 규칙으로 정리합니다...\n", rule_count);

    do
    {
        if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
        {
            // 현재 디렉토리 및 상위 디렉토리는 제외
            continue;
        }

        if (!(file_info.attrib & _A_SUBDIR))
        {
            // 실행 파일 및 설정 파일은 제외
            if (strcmp(file_info.name, "Desktop_Cleaner.exe") == 0) continue;
            if (strcmp(file_info.name, CONFIG_FILE) == 0) continue;

            const char* ext = get_extension(file_info.name);

            for (int i = 0; i < rule_count; i++)
            {
                if (_stricmp(ext, rule_list[i].ext) == 0)   // 대소문자 구분 없이 확장자 비교
                {
                    int move_result = move_and_organize(file_info.name, dest_path, rule_list[i].folder);
                    switch (move_result)
                    {
                    case -2:
                    case -1:
                        // 최종 경로 디렉토리 생성 실패 또는 파일 복사 실패
                        failed_count++;
                        break;
                    case 0:
                        // 파일 복사 및 원본 삭제 성공
                        success_count++;
                        break;
                    case 1:
                        // 파일 복사 성공 및 원본 삭제 실패
                        warning_count++;
                        break;
                    }
                    break;
                }
            }
        }
    } while (_findnext(handle, &file_info) == 0);

    _findclose(handle);

    printf("\n [결과]\n");
    printf(" 총 %d개의 파일 정리에 성공했습니다.\n", success_count);
    if (failed_count != 0)
    {
        printf(" 총 %d개의 파일 정리에 실패했습니다.\n", failed_count);
    }
    if (warning_count != 0)
    {
        printf(" 총 %d개 파일의 복사는 성공했으나, 원본 삭제에 실패했습니다.\n", warning_count);
    }
}

void settings_menu()
{
    int choice;
    char input_ext[20];
    char input_folder[50];

    while (1)
    {
        printf("\n============================================\n");
        printf(" 환경 설정 (현재 규칙: %zu개)\n", rule_count);
        printf(" * 변경사항은 '%s'에 자동 저장됩니다.\n", CONFIG_FILE);
        printf("============================================\n");
        for (int i = 0; i < rule_count; i++)
        {
            printf(" [%d] .%s  --->  %s 폴더\n", i + 1, rule_list[i].ext, rule_list[i].folder);
        }
        printf("--------------------------------------------\n");
        printf(" 1. 새로운 규칙 추가하기\n");
        printf(" 2. 기존 규칙의 폴더 이름 변경하기\n");
        printf(" 3. 뒤로 가기 (메인 메뉴)\n");
        printf("============================================\n");
        printf(" 선택 >> ");

        if (scanf("%d", &choice) != 1)
        {
            while (getchar() != '\n');
            continue;
        }

        if (choice == 3)
        {
            clear_console();
            return;
        }
        else if (choice == 1)
        {
            while (getchar() != '\n');
            clear_console();

            printf("\n [새 규칙 추가]\n");

            printf(" 정리할 확장자 입력 (예: ppt) >> ");
            fgets(input_ext, sizeof(input_ext), stdin);
            input_ext[strcspn(input_ext, "\n")] = 0;

            printf(" 보낼 폴더 이름 입력 (예: Univ_PPT) >> ");
            fgets(input_folder, sizeof(input_folder), stdin);
            input_folder[strcspn(input_folder, "\n")] = 0;

            add_rule(input_ext, input_folder);

            if (save_rules_to_file())
            {
                printf(" 추가 및 저장 완료.\n");
            }
            else
            {
                printf(" [오류] 설정을 추가할 수 없습니다.\n");
            }
        }
        else if (choice == 2)
        {
            int idx;
            printf("\n [규칙 수정]\n");
            printf(" 변경할 규칙의 번호를 입력하세요 >> ");
            scanf("%d", &idx);

            if (idx >= 1 && idx <= rule_count)
            {
                printf(" '.%s' 파일을 어느 폴더로 바꿀까요? >> ", rule_list[idx - 1].ext);
                scanf("%s", input_folder);

                strcpy(rule_list[idx - 1].folder, input_folder);
                if (save_rules_to_file())
                {
                    clear_console();
                    printf(" 변경 및 저장 완료.\n");
                }
                else
                {
                    clear_console();
                    printf(" [오류] 설정을 변경할 수 없습니다.\n");
                }
            }
            else
            {
                clear_console();
                printf(" 잘못된 번호입니다.\n");
            }
        }
        else
        {
            clear_console();
            printf(" 잘못된 입력입니다.\n");
        }
    }
}