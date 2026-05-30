#include "BankManager.h"
#include "BankUI.h"

int main() {
    BankManager backend;
    backend.boot_up_scanner();

    BankUI terminal(&backend);

    terminal.show_main_menu();

    return 0;
}