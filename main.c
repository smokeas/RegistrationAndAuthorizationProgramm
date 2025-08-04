#include "input.h"

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "RUS");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    srand(time(NULL));
//ГОЙДА ГОЙДА ГОАЙД
    /ЫЩАТЫЩВА
    SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
       SDPFKSIPKFISPDKF
    qerfwe]\'G;R[EG.F;CVAR. /
// WRF WE[. FRPW F
        F E';F, WE;F, WE.F '; 
     /// WERF WE FWEF WEF
    if (argc < 2) {
        printf("Èñïîëüçîâàíèå: %s users.txt\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    if (_access(filename, 0) != 0) {
        FILE* file;
        if (fopen_s(&file, filename, "w") == 0) {
            fprintf(file, "admin admin %u\n", cache_function("admin", "admin"));
            fclose(file);
        }
    }

    int choice;
    do {
        print_menu();
        choice = getInputInt(1, 4);

        switch (choice) {
        case 1: user_login(filename); break;
        case 2: admin_panel(filename); break;
        case 3: register_user(filename); break;
        case 4: printf("\nÄî ñâèäàíèÿ.\n"); break;
        }
        if (choice != 4) system("pause");
    } while (choice != 4);

    return 0;
}
