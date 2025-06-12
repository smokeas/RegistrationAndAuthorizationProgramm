#include "input.h"

struct ChangePassContext {
    const char* login;
    const char* new_pass;
};

struct DeleteUserContext {
    const char* target;
};
//обновление файла
static int safe_file_update(const char* filename, int (*processor)(FILE*, FILE*, void*), void* context) {
    char temp_filename[MAX_PATH];
    char* last_backslash = strrchr(filename, '\\');
    char* filename_only = (last_backslash != NULL) ? last_backslash + 1 : (char*)filename;

    // Формируем имя временного файла в текущей директории
    sprintf_s(temp_filename, MAX_PATH, "temp_%s", filename_only);

    // Проверка прав на запись в текущую директорию
    if (_access(".", 02) != 0) {
        printf("Ошибка: нет прав на запись в текущую директорию!\n");
        return 0;
    }

    FILE* src = NULL, * dst = NULL;
    int result = 0;

    if (fopen_s(&src, filename, "r") != 0) {
        printf("Ошибка открытия файла %s\n", filename);
        return 0;
    }

    if (fopen_s(&dst, temp_filename, "w") != 0) {
        printf("Ошибка создания временного файла %s. Проверьте права доступа.\n", temp_filename);
        fclose(src);
        return 0;
    }

    if (!processor(src, dst, context)) {
        printf("Ошибка обработки файла. Возможно, пользователь не найден.\n");
        fclose(src);
        fclose(dst);
        remove(temp_filename);
        return 0;
    }

    fclose(src);
    fclose(dst);

    if (remove(filename) != 0) {
        printf("Ошибка удаления исходного файла %s. Проверьте права доступа.\n", filename);
        remove(temp_filename);
        return 0;
    }

    if (rename(temp_filename, filename) != 0) {
        printf("Ошибка переименования временного файла в %s. Проверьте права доступа.\n", filename);
        remove(temp_filename);
        return 0;
    }

    return 1;
}
//ищем  пользователя и обноаляем
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

    printf("\n--- Регистрация ---\n");
    printf("(Для отмены введите '/cancel' в любое поле)\n");

    // Ввод логина
    do {
        printf("Логин: ");
        if (scanf_s("%255s", login, (unsigned)_countof(login)) != 1) {
            printf("Ошибка ввода логина\n");
            while (getchar() != '\n'); // Очистка буфера
            continue;
        }

        if (strcmp(login, "/cancel") == 0) {
            printf("Регистрация отменена.\n");
            return;
        }

        if (!is_valid_input(login, 0)) {
            printf("Логин может содержать только буквы и цифры\n");
            continue;
        }

        if (is_login_exist(login, filename)) {
            printf("Логин уже существует\n");
            continue;
        }
        break;
    } while (1);

    // Ввод пароля
    do {
        printf("Пароль (или 'gen' для генерации): ");
        if (scanf_s("%255s", password, (unsigned)_countof(password)) != 1) {
            printf("Ошибка ввода пароля\n");
            while (getchar() != '\n');
            continue;
        }

        if (strcmp(password, "/cancel") == 0) {
            printf("Регистрация отменена.\n");
            return;
        }

        if (strcmp(password, "gen") == 0) {
            generate_strong_password(password, 12);
            printf("Сгенерированный пароль: %s\n", password);
            break;
        }

        if (!is_valid_input(password, 1)) {
            printf("Пароль содержит недопустимые символы\n");
            continue;
        }

        evaluate_password_strength(password);

        if (!is_password_strong(password)) {
            continue;
        }

        // Подтверждение пароля
        do {
            printf("Подтвердите пароль: ");
            if (scanf_s("%255s", confirm_password, (unsigned)_countof(confirm_password)) != 1) {
                printf("Ошибка ввода подтверждения\n");
                while (getchar() != '\n');
                continue;
            }

            if (strcmp(confirm_password, "/cancel") == 0) {
                printf("Регистрация отменена.\n");
                return;
            }

            if (strcmp(password, confirm_password) != 0) {
                printf("Пароли не совпадают\n");
            }
        } while (strcmp(password, confirm_password) != 0);

        break;
    } while (1);

    // Сохранение пользователя (полностью переработанная часть)
    FILE* file = NULL;
    errno_t err;

    // Попробуем открыть для добавления
    err = fopen_s(&file, filename, "a");
    if (err != 0 || file == NULL) {
        // Если не получилось, пробуем создать новый файл
        err = fopen_s(&file, filename, "w");
        if (err != 0 || file == NULL) {
            printf("Ошибка открытия/создания файла. Код: %d\n", err);
            perror("Подробная ошибка");
            return;
        }
    }

    // Проверка прав доступа к файлу
    if (_access(filename, 02) != 0) { // Проверка на запись
        printf("Нет прав на запись в файл!\n");
        fclose(file);
        return;
    }

    strcpy_s(new_user.login, _countof(new_user.login), login);
    strcpy_s(new_user.password, _countof(new_user.password), password);
    new_user.cache_sum = cache_function(login, password);

    if (fprintf(file, "%s %s %u\n", new_user.login, new_user.password, new_user.cache_sum) < 0) {
        printf("Ошибка записи в файл!\n");
    }
    else {
        printf("Регистрация прошла успешно!\n");
    }

    fclose(file);
}
void admin_change_password(const char* filename) {
    char target_login[MaxLen];
    char new_password[MaxLen];
    char confirm_password[MaxLen];

    printf("\n--- Смена пароля ---\n");
    printf("Логин пользователя: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    if (strcmp(target_login, "admin") == 0) {
        printf("для смены нужно менять в коде \n");
        return;
    }

    if (!is_login_exist(target_login, filename)) {
        printf("Пользователь не найден!\n");
        return;
    }

    do {
        printf("Новый пароль (или 'gen'): ");
        scanf_s("%255s", new_password, (unsigned)_countof(new_password));

        if (strcmp(new_password, "gen") == 0) {
            generate_strong_password(new_password, 12);
            printf("Сгенерированный пароль: %s\n", new_password);
            evaluate_password_strength(new_password);
            break;
        }

        if (!is_valid_input(new_password, 1)) {
            printf("Пароль содержит недопустимые символы\n");
            continue;
        }

        evaluate_password_strength(new_password);

        if (!is_password_strong(new_password)) {
            printf("Пароль слишком слабый!\n");
            continue;
        }

        printf("Подтвердите пароль: ");
        scanf_s("%255s", confirm_password, (unsigned)_countof(confirm_password));
    } while (strcmp(new_password, confirm_password) != 0);

    struct ChangePassContext ctx = { target_login, new_password };
    if (safe_file_update(filename, change_pass_processor, &ctx)) {
        printf("Пароль изменен!\n");
    }
    else {
        printf("Ошибка!\n");
    }
}

void delete_user(const char* filename) {
    char target_login[MaxLen];
    printf("\nЛогин для удаления: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    if (strcmp(target_login, "admin") == 0) {
        printf("Нельзя удалить администратора!\n");
        return;
    }

    struct DeleteUserContext ctx = { .target = target_login };
    if (safe_file_update(filename, delete_user_processor, &ctx)) {
        printf("Пользователь %s удален!\n", target_login);
    }
    else {
        printf("Ошибка или пользователь не найден!\n");
    }
}
//вход
void user_login(const char* filename) {
    char login[MaxLen], password[MaxLen];
    int attempt = 0;
    const int max_attempts = 3;

    while (1) {
        printf("\n--- Вход ---\n");
        printf("Логин : ");
        scanf_s("%255s", login, (unsigned)_countof(login));
        if (strcmp(login, "admin") == 0) {
            printf("Вход под админом разрешён только через панель администратора!\n");
            return;
        }


        if (strcmp(login, "/reg") == 0) {
            register_user(filename);
            attempt = 0;
            continue;
        }

        printf("Пароль: ");
        scanf_s("%255s", password, (unsigned)_countof(password));

        FILE* file;
        UserRecord user;
        int found = 0;

        if (fopen_s(&file, filename, "r") != 0) {
            printf("Ошибка открытия файла пользователей!\n");
            return;
        }

        while (fscanf_s(file, "%255s %255s %u",
            user.login, (unsigned)_countof(user.login),
            user.password, (unsigned)_countof(user.password),
            &user.cache_sum) == 3) {
            if (strcmp(login, user.login) == 0 &&
                cache_function(login, password) == user.cache_sum) {
                printf("\nВход выполнен успешно!\n");
                found = 1;
                break;
            }
        }
        fclose(file);

        if (found) {
            break;
        }

        attempt++;
        printf("\nНеверный логин или пароль!\n");

        if (attempt < max_attempts) {
            printf("Нет аккаунта? Введите '/reg' в поле логина для регистрации.\n");
            printf("1. Попробовать снова\n2. Вернуться в меню\nВыбор: ");
            int choice = getInputInt(1, 2);
            if (choice == 2) {
                printf("Возврат в меню...\n");
                return;
            }
        }
        else {
            printf("Превышено максимальное количество попыток (%d).\n", max_attempts);
            system("pause");
            break;
        }
    }
}

void admin_panel(const char* filename) {
    char login[MaxLen], password[MaxLen];
    printf("\n--- Админ-вход ---\n");
    printf("Логин: ");
    scanf_s("%255s", login, (unsigned)_countof(login));
    printf("Пароль: ");
    scanf_s("%255s", password, (unsigned)_countof(password));

    if (strcmp(login, "admin") != 0 || cache_function("admin", password) != cache_function("admin", "admin")) {
        printf("Доступ запрещен!\n");
        return;
    }

    int choice;
    do {
        printf("\n--- Админ-панель ---\n");
        printf("1. Список пользователей\n2. Показать пароль\n3. Показать кэш\n");
        printf("4. Удалить пользователя\n5. Сменить пароль\n6. Выход\nВыбор: ");
        choice = getInputInt(1, 6);

        switch (choice) {
        case 1: print_all_users(filename); break;
        case 2: show_user_password(filename); break;
        case 3: show_user_cache(filename); break;
        case 4: delete_user(filename); break;
        case 5: admin_change_password(filename); break;
        case 6: printf("Выход...\n"); break;
        }
    } while (choice != 6);
}

void print_all_users(const char* filename) {
    FILE* file;
    UserRecord user;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("Ошибка открытия файла!\n");
        return;
    }

    printf("\n--- Список пользователей ---\n");
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
    printf("\nВведите логин: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    FILE* file;
    UserRecord user;
    int found = 0;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("Ошибка открытия файла!\n");
        return;
    }

    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(target_login, user.login) == 0) {
            printf("Логин: %s\nПароль: %s\n", user.login, user.password);
            found = 1;
            break;
        }
    }

    fclose(file);
    if (!found) printf("Пользователь не найден!\n");
}

void show_user_cache(const char* filename) {
    char target_login[MaxLen];
    printf("\nВведите логин: ");
    scanf_s("%255s", target_login, (unsigned)_countof(target_login));

    FILE* file;
    UserRecord user;
    int found = 0;

    if (fopen_s(&file, filename, "r") != 0) {
        printf("Ошибка открытия файла!\n");
        return;
    }

    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(target_login, user.login) == 0) {
            printf("Логин: %s\nКэш: %u\n", user.login, user.cache_sum);
            found = 1;
            break;
        }
    }

    fclose(file);
    if (!found) printf("Пользователь не найден!\n");
}