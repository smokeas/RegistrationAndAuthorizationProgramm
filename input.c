#include "input.h"

// Функция для установки цвета текста в консоли
void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

int getInputInt(int min, int max) {
    int num;
    while (1) {
        if (scanf_s("%d", &num) != 1 || getchar() != '\n') {
            printf("Ошибка ввода! Введите число от %d до %d: ", min, max);
            while (getchar() != '\n') {}
        }
        else if (num < min || num > max) {
            printf("Число должно быть от %d до %d: ", min, max);
        }
        else {
            return num;
        }
    }
}

int is_valid_input(const char* str, int is_password) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (is_password) {
            if (!isprint((unsigned char)str[i])) {
                return 0;
            }
        }
        else {
            if (!isalnum((unsigned char)str[i])) {
                return 0;
            }
        }
    }
    return 1;
}

int is_password_strong(const char* password) {
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;
    size_t length = strlen(password);

    if (length < 8) return 0;

    for (size_t i = 0; password[i] != '\0'; i++) {
        if (isupper(password[i])) has_upper = 1;
        else if (islower(password[i])) has_lower = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else if (ispunct(password[i])) has_special = 1;
    }

    return (has_upper && has_lower && has_digit && has_special);
}
void evaluate_password_strength(const char* password) {
    // Проверка на  нулевой указатель
    if (password == NULL) {
        printf("Ошибка: передан NULL указатель!\n");
        return;
    }
    //инициализация переменных для прверки
    int has_upper = 0, has_lower = 0, has_digit = 0, has_special = 0;//заглавные строчнык цифры символы
    size_t length = strlen(password);//длинна 
    int complexity = 0;//оценка сложности
    int transitions = 0; // Счетчик переходов между типами символов
    int prev_char_type = -1; // Тип предыдущего символа

    // Типы символов
    enum CharType {
        UPPER = 0,
        LOWER = 1,
        DIGIT = 2,
        SPECIAL = 3
    };

    // Проверяем наличие разных типов символов и считаем переходы
    for (size_t i = 0; i < length; i++) {
        int current_char_type = -1;//тип текущего символа (пока неизвестен)

        if (isupper((unsigned char)password[i])) {// заглавная
            has_upper = 1;
            current_char_type = UPPER;
        }
        else if (islower((unsigned char)password[i])) {//строчные
            has_lower = 1;
            current_char_type = LOWER;
        }
        else if (isdigit((unsigned char)password[i])) {//цифры
            has_digit = 1;
            current_char_type = DIGIT;
        }
        else if (ispunct((unsigned char)password[i])) {//спецсимволы
            has_special = 1;
            current_char_type = SPECIAL;
        }

        // Считаем переходы между типами символов
        if (prev_char_type != -1 && current_char_type != prev_char_type) {
            transitions++;//переходы
        }
        prev_char_type = current_char_type;
    }

    // Рассчитываем базовую сложность на основе длины
    if (length < 5) {
        complexity = 1;
    }
    else if (length < 8) {
        complexity = 3;
    }
    else if (length < 12) {
        complexity = 5;
    }
    else if (length < 16) {
        complexity = 7;
    }
    else {
        complexity = 9;
    }

    // Бонусы за наличие разных типов символов
    if (has_upper) complexity += 2;
    if (has_lower) complexity += 2;
    if (has_digit) complexity += 2;
    if (has_special) complexity += 3;

    // Анализ переходов
    float transition_ratio = (length > 0) ? (float)transitions / length : 0.0f;//отношение числа переходов к длине пароля

    if (transition_ratio > 0.5f) {
        // Высокое соотношение переходов - хороший признак
        complexity += 4;
    }
    else if (transition_ratio > 0.3f) {
        complexity += 2;
    }
    else if (transition_ratio < 0.1f) {
        // Очень мало переходов - плохой признак
        complexity -= 3;
    }

    // Проверка на последовательности (qwerty, 12345 и т.д.)
    int sequence_score = 0;
    for (size_t i = 2; i < length; i++) {
        // Проверка возрастающих последовательностей
        if (password[i] == password[i - 1] + 1 && password[i - 1] == password[i - 2] + 1) {
            sequence_score -= 2;
        }
        // Проверка повторяющихся символов
        if (password[i] == password[i - 1] &&
            password[i - 1] == password[i - 2]) {
            sequence_score -= 3;
        }
    }
    complexity += sequence_score;

    // Проверка на использование общих слов
    const char* common_words[] = {
        "password", "qwerty", "123456", "admin", "welcome",
        "login", "letmein", "master", "sunshine", "football" ,"ytrewq" , "Seleznev" ,"asdfghjkl","zxcvbnm"
    };

    for (size_t i = 0; i < sizeof(common_words) / sizeof(common_words[0]); i++) {
        if (strstr(password, common_words[i]) != NULL) {
            complexity -= 5;
            break;
        }
    }

    // Проверка на использование дат (1900-2099)
    for (size_t i = 0; i < length - 3; i++) {//пкароверить все возможные последовательности из 4 символов 
        if (isdigit(password[i]) && isdigit(password[i + 1]) && // проверка на цифру
            isdigit(password[i + 2]) && isdigit(password[i + 3])) {//проверка следующих трёх символов
            int year = atoi(&password[i]);
            if (year >= 1800 && year <= 2199) {
                complexity -= 3;
                break;
            }
        }
    }

    // Ограничиваем сложность 
    if (complexity < 1) complexity = 1;
    if (complexity > 20) complexity = 20;

    // Выводим подробный анализ
    printf("\n--- Подробный анализ пароля ---\n");
    printf("Длина: %zu символов\n", length);
    printf("Переходы между типами символов: %d (%.1f%%)\n",
        transitions, transition_ratio * 100);
    printf("Содержит: %s%s%s%s\n",
        has_upper ? "заглавные буквы, " : "",
        has_lower ? "строчные буквы, " : "",
        has_digit ? "цифры, " : "",
        has_special ? "спецсимволы" : "");

    if (sequence_score < 0) {
        printf("Обнаружены слабые последовательности символов\n");
    }

    printf("\nОБЩАЯ ОЦЕНКА СЛОЖНОСТИ: ");
    if (complexity < 5) {
        setConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        printf("ОЧЕНЬ СЛАБЫЙ (%d/20)\n", complexity);
        printf("Пароль может быть взломан мгновенно\n");
    }
    else if (complexity < 8) {
        setConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN);
        printf("СЛАБЫЙ (%d/20)\n", complexity);
        printf("Пароль уязвим для атак по словарю\n");
    }
    else if (complexity < 12) {
        setConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("СРЕДНИЙ (%d/20)\n", complexity);
        printf("Пароль приемлем, но может быть улучшен\n");
    }
    else if (complexity < 16) {
        setConsoleColor(FOREGROUND_GREEN);
        printf("СИЛЬНЫЙ (%d/20)\n", complexity);
        printf("Хороший пароль, стойкий к большинству атак\n");
    }
    else {
        setConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("ОЧЕНЬ СИЛЬНЫЙ (%d/20)\n", complexity);
        printf("Отличный пароль! Высокая стойкость к взлому\n");
    }
    setConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    // Рекомендации по улучшению
    printf("\nРЕКОМЕНДАЦИИ:\n");
    if (length < 12) {
        printf("- Увеличьте длину до 12+ символов (сейчас %zu)\n", length);
    }
    if (transition_ratio < 0.3f) {
        printf("- Чаще чередуйте разные типы символов (текущее чередование: %.1f%%)\n",
            transition_ratio * 100);
    }
    if (!has_upper || !has_lower || !has_digit || !has_special) {
        printf("- Добавьте в пароль: ");
        if (!has_upper) printf("заглавные буквы, ");
        if (!has_lower) printf("строчные буквы, ");
        if (!has_digit) printf("цифры, ");
        if (!has_special) printf("спецсимволы");
        printf("\n");
    }
    if (sequence_score < 0) {
        printf("- Избегайте последовательностей и повторяющихся символов\n");
    }
    printf("\n");
}
uint32_t cache_function(const char* login, const char* password) {//предназначен для 32-битных целых 
    uint32_t hash = 0x811C9DC5; // начальное значениеииз лгоритма FNA -1a
    const uint32_t prime = 0x01000193;//число для перемешивания битов
    
    for (const char* p = login; *p; p++) {// Перебирает каждый символ логина до \0
        hash ^= (uint32_t)(*p);//^= (XOR): Побитовое исключающее ИЛИ. Обновляет hash значением текущего символа
        //Пример: hash = 0x811C9DC5, *p = 'A' (0x41) → hash = 0x811C9DC5 ^ 0x41 = 0x811C9D84
        hash *= prime;//Умножение на простое число для увеличения энтропии(мера бесспорядка)
        hash += (hash << 13) | (hash >> 19);//сдвиги
    }

    hash ^= 0xDEADBEEF;// добавление соли
    hash *= prime;//еще усложняем

    for (const char* p = password; *p; p++) {
        hash ^= (uint32_t)(*p);
        hash *= prime;
        hash = (hash << 17) | (hash >> 15);
    }

    hash ^= hash >> 16;
    hash *= 0x85EBCA6B;
    hash ^= hash >> 13;
    hash *= 0xC2B2AE35;
    hash ^= hash >> 16;

    return hash;
}

int is_login_exist(const char* login, const char* filename) {
    FILE* file;
    if (fopen_s(&file, filename, "r") != 0) {
        return 0;
    }

    UserRecord user;
    int found = 0;

    while (fscanf_s(file, "%255s %255s %u",
        user.login, (unsigned)_countof(user.login),
        user.password, (unsigned)_countof(user.password),
        &user.cache_sum) == 3) {
        if (strcmp(login, user.login) == 0) {
            found = 1;
            break;
        }
    }

    fclose(file);
    return found;
}

void print_menu() {
    system("cls");//очистка экрана
    printf("СИСТЕМА АУТЕНТИФИКАЦИИ\n\n");
    printf("1. Вход\n");
    printf("2. Администрирование\n");
    printf("3. Регистрация\n");
    printf("4. Выход\n\n");
    printf("Выбор: ");
}

void generate_strong_password(char* password, size_t length) {
    const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    const char digits[] = "0123456789";
    const char specials[] = "!@#$%^&*";

    password[0] = uppercase[rand() % 26];
    password[1] = lowercase[rand() % 26];
    password[2] = digits[rand() % 10];
    password[3] = specials[rand() % 8];

    const char all_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";

    for (size_t i = 4; i < length; i++) {
        password[i] = all_chars[rand() % (sizeof(all_chars) - 1)];
    }

    for (size_t i = 0; i < length; i++) {
        size_t j = rand() % length;
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }

    password[length] = '\0';
}