#include "input.h"

struct ChangePassContext {
    const char* login;
    const char* new_pass;
};

struct DeleteUserContext {
    const char* target;
};
//���������� �����
static int safe_file_update(const char* filename, int (*processor)(FILE*, FILE*, void*), void* context) {
    char temp_filename[MAX_PATH];
    char* last_backslash = strrchr(filename, '\\');
    char* filename_only = (last_backslash != NULL) ? last_backslash + 1 : (char*)filename;

    // ��������� ��� ���������� ����� � ������� ����������
    sprintf_s(temp_filename, MAX_PATH, "temp_%s", filename_only);

    // �������� ���� �� ������ � ������� ����������
    if (_access(".", 02) != 0) {
        printf("������: ��� ���� �� ������ � ������� ����������!\n");
        return 0;
    }

    FILE* src = NULL, * dst = NULL;
    int result = 0;

    if (fopen_s(&src, filename, "r") != 0) {
        printf("������ �������� ����� %s\n", filename);
        return 0;
    }

    if (fopen_s(&dst, temp_filename, "w") != 0) {
        printf("������ �������� ���������� ����� %s. ��������� ����� �������.\n", temp_filename);
        fclose(src);
        return 0;
    }

    if (!processor(src, dst, context)) {
        printf("������ ��������� �����. ��������, ������������ �� ������.\n");
        fclose(src);
        fclose(dst);
        remove(temp_filename);
        return 0;
    }

    fclose(src);
    fclose(dst);

    if (remove(filename) != 0) {
        printf("������ �������� ��������� ����� %s. ��������� ����� �������.\n", filename);
        remove(temp_filename);
        return 0;
    }

    if (rename(temp_filename, filename) != 0) {
        printf("������ �������������� ���������� ����� � %s. ��������� ����� �������.\n", filename);
        remove(temp_filename);
        return 0;
    }

    return 1;
}
//����  ������������ � ���������
static int change_pass_processor(FILE* src, FILE* dst, void* context) {
    struct ChangePassContext* ctx = (struct ChangePassContext*)context;
    UserRecord user;
    int updated = 0;

    while (fscanf_s(src, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(user.login, ctx->login) == 0) {
            strcpy_s(user.password, _countof(user.password), ctx->new_pass);
            user.cache_sum = cache_function(user.login, user.password);
            updated = 1;
        }
        fprintf(dst, "%s %s %u\n", user.login, user.password, user.cache_sum);
    }

    return updated;
}

static int delete_user_processor(FILE* src, FILE* dst, void* context) {
    struct DeleteUserContext* ctx = (struct DeleteUserContext*)context;
    UserRecord user;
    int found = 0;

    while (fscanf_s(src, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(user.login, ctx->target) == 0) {
            found = 1;
        }
        else {
            fprintf(dst, "%s %s %u\n", user.login, user.password, user.cache_sum);
        }
    }

    return found;
}

void register_user(const char* filename) {
    char login[MaxLen], password[MaxLen], confirm_password[MaxLen];
    UserRecord new_user;

    printf("\n--- ����������� ---\n");
    printf("(��� ������ ������� '/cancel' � ����� ����)\n");

    // ���� ������
    do {
        printf("�����: ");
        if (scanf_s("%255s", login, (unsigned)_countof(login)) != 1) {
            printf("������ ����� ������\n");
            while (getchar() != '\n'); // ������� ������
            continue;
        }

        if (strcmp(login, "/cancel") == 0) {
            printf("����������� ��������.\n");
            return;
        }

        if (!is_valid_input(login, 0)) {
            printf("����� ����� ��������� ������ ����� � �����\n");
            continue;
        }

        if (is_login_exist(login, filename)) {
            printf("����� ��� ����������\n");
            continue;
        }
        break;
    } while (1);

    // ���� ������
    do {
        printf("������ (��� 'gen' ��� ���������): ");
        if (scanf_s("%255s", password, (unsigned)_countof(password)) != 1) {
            printf("������ ����� ������\n");
            while (getchar() != '\n');
            continue;
        }

        if (strcmp(password, "/cancel") == 0) {
            printf("����������� ��������.\n");
            return;
        }

        if (strcmp(password, "gen") == 0) {
            generate_strong_password(password, 12);
            printf("��������������� ������: %s\n", password);
            break;
        }

        if (!is_valid_input(password, 1)) {
            printf("������ �������� ������������ �������\n");
            continue;
        }

        evaluate_password_strength(password);

        if (!is_password_strong(password)) {
            continue;
        }

        // ������������� ������
        do {
            printf("����������� ������: ");
            if (scanf_s("%255s", confirm_password, (unsigned)_countof(confirm_password)) != 1) {
                printf("������ ����� �������������\n");
                while (getchar() != '\n');
                continue;
            }

            if (strcmp(confirm_password, "/cancel") == 0) {
                printf("����������� ��������.\n");
                return;
            }

            if (strcmp(password, confirm_password) != 0) {
                printf("������ �� ���������\n");
            }
        } while (strcmp(password, confirm_password) != 0);

        break;
    } while (1);

    // ���������� ������������ (��������� �������������� �����)
    FILE* file = NULL;
    errno_t err;

    // ��������� ������� ��� ����������
    err = fopen_s(&file, filename, "a");
    if (err != 0 || file == NULL) {
        // ���� �� ����������, ������� ������� ����� ����
        err = fopen_s(&file, filename, "w");
        if (err != 0 || file == NULL) {
            printf("������ ��������/�������� �����. ���: %d\n", err);
            perror("��������� ������");
            return;
        }
    }

    // �������� ���� ������� � �����
    if (_access(filename, 02) != 0) { // �������� �� ������
        printf("��� ���� �� ������ � ����!\n");
        fclose(file);
        return;
    }

    strcpy_s(new_user.login, _countof(new_user.login), login);
    strcpy_s(new_user.password, _countof(new_user.password), password);
    new_user.cache_sum = cache_function(login, password);

    if (fprintf(file, "%s %s %u\n", new_user.login, new_user.password, new_user.cache_sum) < 0) {
        printf("������ ������ � ����!\n");
    }
    else {
        printf("����������� ������ �������!\n");
    }

    fclose(file);
}
void admin_change_password(const char* filename) {
    char target_login[MaxLen];
    char new_password[MaxLen];
    char confirm_password[MaxLen];

    printf("\n--- ����� ������ ---\n");
    printf("����� ������������: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    if (strcmp(target_login, "admin") == 0) {
        printf("��� ����� ����� ������ � ���� \n");
        return;
    }

    if (!is_login_exist(target_login, filename)) {
        printf("������������ �� ������!\n");
        return;
    }

    do {
        printf("����� ������ (��� 'gen'): ");
        scanf_s("%255s", new_password, (unsigned)_countof(new_password));

        if (strcmp(new_password, "gen") == 0) {
            generate_strong_password(new_password, 12);
            printf("��������������� ������: %s\n", new_password);
            evaluate_password_strength(new_password);
            break;
        }

        if (!is_valid_input(new_password, 1)) {
            printf("������ �������� ������������ �������\n");
            continue;
        }

        evaluate_password_strength(new_password);

        if (!is_password_strong(new_password)) {
            printf("������ ������� ������!\n");
            continue;
        }

        printf("����������� ������: ");
        scanf_s("%255s", confirm_password, (unsigned)_countof(confirm_password));
    } while (strcmp(new_password, confirm_password) != 0);

    struct ChangePassContext ctx = { target_login, new_password };
    if (safe_file_update(filename, change_pass_processor, &ctx)) {
        printf("������ �������!\n");
    }
    else {
        printf("������!\n");
    }
}

void delete_user(const char* filename) {
    char target_login[MaxLen];
    printf("\n����� ��� ��������: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    if (strcmp(target_login, "admin") == 0) {
        printf("������ ������� ��������������!\n");
        return;
    }

    struct DeleteUserContext ctx = { .target = target_login };
    if (safe_file_update(filename, delete_user_processor, &ctx)) {
        printf("������������ %s ������!\n", target_login);
    }
    else {
        printf("������ ��� ������������ �� ������!\n");
    }
}
//����
void user_login(const char* filename) {
    char login[MaxLen], password[MaxLen];
    int attempt = 0;
    const int max_attempts = 3;

    while (1) {
        printf("\n--- ���� ---\n");
        printf("����� : ");
        scanf_s("%255s", login, (unsigned)_countof(login));
        if (strcmp(login, "admin") == 0) {
            printf("���� ��� ������� �������� ������ ����� ������ ��������������!\n");
            return;
        }


        if (strcmp(login, "/reg") == 0) {
            register_user(filename);
            attempt = 0;
            continue;
        }

        printf("������: ");
        scanf_s("%255s", password, (unsigned)_countof(password));

        FILE* file;
        UserRecord user;
        int found = 0;

        if (fopen_s(&file, filename, "r") != 0) {
            printf("������ �������� ����� �������������!\n");
            return;
        }

        while (fscanf_s(file, "%255s %255s %u",
            user.login, (unsigned)_countof(user.login),
            user.password, (unsigned)_countof(user.password),
            &user.cache_sum) == 3) {
            if (strcmp(login, user.login) == 0 &&
                cache_function(login, password) == user.cache_sum) {
                printf("\n���� �������� �������!\n");
                found = 1;
                break;
            }
        }
        fclose(file);

        if (found) {
            break;
        }

        attempt++;
        printf("\n�������� ����� ��� ������!\n");

        if (attempt < max_attempts) {
            printf("��� ��������? ������� '/reg' � ���� ������ ��� �����������.\n");
            printf("1. ����������� �����\n2. ��������� � ����\n�����: ");
            int choice = getInputInt(1, 2);
            if (choice == 2) {
                printf("������� � ����...\n");
                return;
            }
        }
        else {
            printf("��������� ������������ ���������� ������� (%d).\n", max_attempts);
            system("pause");
            break;
        }
    }
}

void admin_panel(const char* filename) {
    char login[MaxLen], password[MaxLen];
    printf("\n--- �����-���� ---\n");
    printf("�����: ");
    scanf_s("%255s", login, (unsigned)_countof(login));
    printf("������: ");
    scanf_s("%255s", password, (unsigned)_countof(password));

    if (strcmp(login, "admin") != 0 || cache_function("admin", password) != cache_function("admin", "admin")) {
        printf("������ ��������!\n");
        return;
    }

    int choice;
    do {
        printf("\n--- �����-������ ---\n");
        printf("1. ������ �������������\n2. �������� ������\n3. �������� ���\n");
        printf("4. ������� ������������\n5. ������� ������\n6. �����\n�����: ");
        choice = getInputInt(1, 6);

        switch (choice) {
        case 1: print_all_users(filename); break;
        case 2: show_user_password(filename); break;
        case 3: show_user_cache(filename); break;
        case 4: delete_user(filename); break;
        case 5: admin_change_password(filename); break;
        case 6: printf("�����...\n"); break;
        }
    } while (choice != 6);
}

void print_all_users(const char* filename) {
    FILE* file;
    UserRecord user;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("������ �������� �����!\n");
        return;
    }

    printf("\n--- ������ ������������� ---\n");
    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        printf("- %s\n", user.login);
    }
    fclose(file);
}

void show_user_password(const char* filename) {
    char target_login[MaxLen];
    printf("\n������� �����: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    FILE* file;
    UserRecord user;
    int found = 0;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("������ �������� �����!\n");
        return;
    }

    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(target_login, user.login) == 0) {
            printf("�����: %s\n������: %s\n", user.login, user.password);
            found = 1;
            break;
        }
    }

    fclose(file);
    if (!found) printf("������������ �� ������!\n");
}

void show_user_cache(const char* filename) {
    char target_login[MaxLen];
    printf("\n������� �����: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    FILE* file;
    UserRecord user;
    int found = 0;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("������ �������� �����!\n");
        return;
    }

    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(target_login, user.login) == 0) {
            printf("�����: %s\n���: %u\n", user.login, user.cache_sum);
            found = 1;
            break;
        }
    }

    fclose(file);
    if (!found) printf("������������ �� ������!\n");
}