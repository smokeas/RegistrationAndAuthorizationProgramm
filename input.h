#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <windows.h>
#include <time.h>
#include <io.h>
#include <time.h>

#define MaxLen 256
#define TEMP_FILE_PREFIX "temp_"

typedef struct {
    char login[MaxLen];
    char password[MaxLen];
    uint32_t cache_sum;
} UserRecord;

// Функции ввода/вывода
int getInputInt(int min, int max);
void print_menu();

// Функции работы с пользователями
int is_valid_input(const char* str, int is_password);
int is_password_strong(const char* password);
void evaluate_password_strength(const char* password);
uint32_t cache_function(const char* login, const char* password);
int is_login_exist(const char* login, const char* filename);
void register_user(const char* filename);
void user_login(const char* filename);
void generate_strong_password(char* password, size_t length);

// Административные функции
void admin_panel(const char* filename);
void print_all_users(const char* filename);
void show_user_password(const char* filename);
void show_user_cache(const char* filename);
void delete_user(const char* filename);
void admin_change_password(const char* filename);